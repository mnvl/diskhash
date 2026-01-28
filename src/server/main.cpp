#include <csignal>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include "http_server.h"

namespace po = boost::program_options;

static std::unique_ptr<diskhash::http_server> g_server;

static void signal_handler(int) {
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("diskhash_server options");
        desc.add_options()
            ("help,h", "Show help message")
            ("port,p", po::value<uint16_t>()->default_value(8080),
                "Port to listen on")
            ("address,a", po::value<std::string>()->default_value("0.0.0.0"),
                "Address to bind to")
            ("db,d", po::value<std::string>()->required(),
                "Path to database files (required)")
            ("shards,s", po::value<size_t>()->default_value(4),
                "Number of shards")
            ("threads,t", po::value<size_t>()->default_value(
                std::thread::hardware_concurrency()),
                "Number of worker threads");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage: diskhash_server [options]\n\n" << desc << "\n";
            return 0;
        }

        po::notify(vm);

        auto address = vm["address"].as<std::string>();
        auto port = vm["port"].as<uint16_t>();
        auto db_path = vm["db"].as<std::string>();
        auto num_shards = vm["shards"].as<size_t>();
        auto num_threads = vm["threads"].as<size_t>();

        if (num_threads == 0) {
            num_threads = 1;
        }

        std::cout << "Starting diskhash_server on " << address << ":" << port
                  << " with " << num_shards << " shards and "
                  << num_threads << " threads\n";
        std::cout << "Database path: " << db_path << "\n";

        // Set up signal handlers
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        g_server = std::make_unique<diskhash::http_server>(
            address, port, db_path, num_shards, num_threads);

        g_server->run();

        std::cout << "Server is running. Press Ctrl+C to stop.\n";

        // Wait for server to complete (signal handler calls stop())
        g_server->wait();

        std::cout << "Server stopped.\n";
        g_server.reset();

    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Use --help for usage information.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
