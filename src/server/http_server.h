#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "sharded_hash_map.h"

namespace diskhash {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class http_server {
public:
    http_server(const std::string& address, uint16_t port,
                const std::string& db_path, size_t num_shards,
                size_t num_threads);

    ~http_server();

    void run();
    void stop();
    void wait();

private:
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    sharded_hash_map db_;
    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};
    size_t num_threads_;

    void do_accept();
    void handle_session(tcp::socket socket);
    http::response<http::string_body> handle_request(
        const http::request<http::string_body>& req);

    // Request handlers
    http::response<http::string_body> handle_get(const std::string& key);
    http::response<http::string_body> handle_set(const std::string& key,
                                                  const std::string& value);
    http::response<http::string_body> handle_delete(const std::string& key);
    http::response<http::string_body> handle_keys();
    http::response<http::string_body> handle_health();

    // URL utilities
    static std::string url_decode(const std::string& str);
    static std::string base64url_decode(const std::string& str);
    static std::string base64url_encode(const std::string& str);
    static std::string extract_query_param(const std::string& target,
                                           const std::string& param);
};

}  // namespace diskhash
