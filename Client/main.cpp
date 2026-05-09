#include <iostream>
#include <thread>
#include <asio.hpp>

using asio::ip::tcp;

class Client {
private:
    asio::io_context _io;
    tcp::resolver _resolver;
    tcp::socket _socket;
    tcp::resolver::results_type _endpoints;

public:
    Client(const std::string& host, const std::string& port)
        : _resolver(_io),
          _socket(_io),
          _endpoints(_resolver.resolve(host, port)) {}

    asio::io_context& io() { return _io; }
    tcp::socket& socket() { return _socket; }

    void connect() {
        asio::connect(_socket, _endpoints);
    }

    template <typename HandlerT>
    void run(HandlerT handler) {
        std::thread io_thread([this] {
            _io.run();
        });
        handler();
        io_thread.join();
    }
};

int main(int argc, char **argv) {
    try {
        Client client(argv[1], argv[2]);
        client.connect();
        client.run([&client] {
            std::cout << "Connected\n";

            for (;;) {
                std::string line;
                if (!std::getline(std::cin, line))
                    break;

                line += '\n';

                auto msg = std::make_shared<std::string>(std::move(line));
                asio::async_write(client.socket(),
                    asio::buffer(*msg),[msg](const asio::error_code& ec, std::size_t) {
                        if (ec) {
                            std::cout << "Write error: " << ec.message() << "\n";
                        }
                    });
            }
        });
    }
    catch (const std::exception& e) {
        std::cout << e.what() << "\n";
    }
}