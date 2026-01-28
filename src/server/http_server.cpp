#include "http_server.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

namespace diskhash {

http_server::http_server(const std::string& address, uint16_t port,
                         const std::string& db_path, size_t num_shards,
                         size_t num_threads)
    : ioc_(static_cast<int>(num_threads))
    , acceptor_(ioc_)
    , db_(db_path, num_shards)
    , num_threads_(num_threads)
{
    beast::error_code ec;

    auto endpoint = tcp::endpoint(net::ip::make_address(address), port);

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open acceptor: " + ec.message());
    }

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Failed to set reuse_address: " + ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Failed to bind: " + ec.message());
    }

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("Failed to listen: " + ec.message());
    }
}

http_server::~http_server() {
    stop();
    wait();
}

void http_server::run() {
    running_ = true;
    do_accept();

    threads_.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { ioc_.run(); });
    }
}

void http_server::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    beast::error_code ec;
    acceptor_.close(ec);
    ioc_.stop();
}

void http_server::wait() {
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads_.clear();

    db_.close();
}

void http_server::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec && running_) {
                // Launch session in a detached coroutine-like manner
                net::post(
                    socket.get_executor(),
                    [this, s = std::move(socket)]() mutable {
                        handle_session(std::move(s));
                    });
            }
            if (running_) {
                do_accept();
            }
        });
}

void http_server::handle_session(tcp::socket socket) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    while (running_) {
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);

        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            return;
        }

        auto res = handle_request(req);
        res.set(http::field::server, "diskhash-server/1.0");
        res.prepare_payload();

        http::write(socket, res, ec);
        if (ec) {
            return;
        }

        if (res.need_eof()) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

http::response<http::string_body> http_server::handle_request(
    const http::request<http::string_body>& req)
{
    auto target = std::string(req.target());

    // Parse path and query string
    auto query_pos = target.find('?');
    std::string path = (query_pos != std::string::npos)
                           ? target.substr(0, query_pos)
                           : target;

    // Health check
    if (path == "/health") {
        return handle_health();
    }

    // Keys listing
    if (path == "/keys" && req.method() == http::verb::get) {
        return handle_keys();
    }

    // GET /get?key=...
    if (path == "/get" && req.method() == http::verb::get) {
        auto key = extract_query_param(target, "key");
        if (key.empty()) {
            http::response<http::string_body> res{http::status::bad_request,
                                                   req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Missing 'key' parameter";
            return res;
        }
        return handle_get(base64url_decode(url_decode(key)));
    }

    // PUT/POST /set?key=...
    if (path == "/set" &&
        (req.method() == http::verb::put || req.method() == http::verb::post)) {
        auto key = extract_query_param(target, "key");
        if (key.empty()) {
            http::response<http::string_body> res{http::status::bad_request,
                                                   req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Missing 'key' parameter";
            return res;
        }
        return handle_set(base64url_decode(url_decode(key)), req.body());
    }

    // DELETE /delete?key=...
    if (path == "/delete" && req.method() == http::verb::delete_) {
        auto key = extract_query_param(target, "key");
        if (key.empty()) {
            http::response<http::string_body> res{http::status::bad_request,
                                                   req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Missing 'key' parameter";
            return res;
        }
        return handle_delete(base64url_decode(url_decode(key)));
    }

    // Not found
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Not Found";
    return res;
}

http::response<http::string_body> http_server::handle_get(const std::string& key) {
    auto result = db_.get(key);
    if (result) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "application/octet-stream");
        res.body() = std::move(*result);
        return res;
    }

    http::response<http::string_body> res{http::status::not_found, 11};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Key not found";
    return res;
}

http::response<http::string_body> http_server::handle_set(
    const std::string& key, const std::string& value)
{
    if (db_.set(key, value)) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/plain");
        res.body() = "OK";
        return res;
    }

    http::response<http::string_body> res{http::status::conflict, 11};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Key already exists";
    return res;
}

http::response<http::string_body> http_server::handle_delete(const std::string& key) {
    if (db_.remove(key)) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/plain");
        res.body() = "OK";
        return res;
    }

    http::response<http::string_body> res{http::status::not_found, 11};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Key not found";
    return res;
}

http::response<http::string_body> http_server::handle_keys() {
    auto all_keys = db_.keys();

    std::ostringstream oss;
    for (const auto& key : all_keys) {
        oss << base64url_encode(key) << "\n";
    }

    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "text/plain");
    res.body() = oss.str();
    return res;
}

http::response<http::string_body> http_server::handle_health() {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "text/plain");
    res.body() = "OK";
    return res;
}

std::string http_server::url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

// Base64url alphabet (RFC 4648)
static constexpr char base64url_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static constexpr std::array<int, 256> make_decode_table() {
    std::array<int, 256> table{};
    for (auto& v : table) v = -1;
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(base64url_chars[i])] = i;
    }
    return table;
}

static constexpr auto base64url_decode_table = make_decode_table();

std::string http_server::base64url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 3 / 4);

    int val = 0;
    int bits = 0;

    for (char c : str) {
        if (c == '=') break;
        int d = base64url_decode_table[static_cast<unsigned char>(c)];
        if (d < 0) continue;

        val = (val << 6) | d;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            result += static_cast<char>((val >> bits) & 0xFF);
        }
    }

    return result;
}

std::string http_server::base64url_encode(const std::string& str) {
    std::string result;
    result.reserve((str.size() + 2) / 3 * 4);

    int val = 0;
    int bits = 0;

    for (unsigned char c : str) {
        val = (val << 8) | c;
        bits += 8;

        while (bits >= 6) {
            bits -= 6;
            result += base64url_chars[(val >> bits) & 0x3F];
        }
    }

    if (bits > 0) {
        val <<= (6 - bits);
        result += base64url_chars[val & 0x3F];
    }

    // No padding for base64url

    return result;
}

std::string http_server::extract_query_param(const std::string& target,
                                              const std::string& param)
{
    auto query_pos = target.find('?');
    if (query_pos == std::string::npos) {
        return "";
    }

    std::string query = target.substr(query_pos + 1);
    std::string search = param + "=";

    size_t pos = 0;
    while (pos < query.size()) {
        auto amp = query.find('&', pos);
        std::string pair = (amp != std::string::npos)
                               ? query.substr(pos, amp - pos)
                               : query.substr(pos);

        if (pair.compare(0, search.size(), search) == 0) {
            return pair.substr(search.size());
        }

        if (amp == std::string::npos) break;
        pos = amp + 1;
    }

    return "";
}

}  // namespace diskhash
