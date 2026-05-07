#include <iostream>
#include <string>
#include <thread>
#include <asio.hpp>
#include <random>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io;

        tcp::resolver resolver(io);
        const auto endpoints = resolver.resolve("127.0.0.1", "7373");

        auto sock = std::make_shared<tcp::socket>(io);

        asio::async_connect(*sock, endpoints,
            [sock](const asio::error_code& ec, const tcp::endpoint&) {
                if (!ec) {
                    std::cout << "Connected\n";
                } else {
                    std::cerr << "Connect error: " << ec.message() << "\n";
                }
            });

        std::thread io_thread([&io]() { io.run(); });


        for (;;) {
            std::string line;
            if (!std::getline(std::cin, line)) break;
            line += '\n';
            if (line.length() > 2) {
                asio::post(io, [sock, line = std::move(line)]() {
                     asio::async_write(*sock, asio::buffer(line),
                         [sock](const asio::error_code& ec, std::size_t bytes) {
                             if (ec) std::cerr << "Write error: " << ec.message() << "\n";
                         });
                 });
            }
        }

        io_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
