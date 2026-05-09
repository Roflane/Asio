#include <iostream>
#include <array>
#include <vector>
#include <asio.hpp>

using asio::ip::tcp;

#define BUFFER_CAP 1024

class Server {
private:
   // std::unordered_map<std::string, std::thread> _threadPool;
    std::shared_ptr<asio::io_context> _pIo;
    std::shared_ptr<tcp::acceptor> _pServer;
public:
    ~Server() = default;
    explicit Server(const tcp proto, const ushort port) {
        _pIo = std::make_shared<asio::io_context>();
        _pServer = std::make_shared<tcp::acceptor>(*_pIo, tcp::endpoint(proto, port));
    }
    Server(const Server& other) = default;
    Server(Server&& other) noexcept = default;

    Server& operator=(const Server& other) = default;
    Server& operator=(Server&& other) noexcept = default;

    const auto get_io() const { return _pIo; }
    const auto get_server() const { return _pServer; }


    // template <typename T>
    // void push_thread(T &&thread) {
    //     _threadPool.emplace(std::forward<T>(thread));
    // }

    template <typename AcceptHandlerT>
    void accept(AcceptHandlerT accept_handler) {
        auto accept_lambda = [this, accept_handler = std::move(accept_handler)](const asio::error_code &ec, tcp::socket socket) {
            accept_handler(ec, std::move(socket));
            this->accept(accept_handler);
        };
        _pServer->async_accept(accept_lambda);
    }

};

void log_accept(const std::error_code &e, const size_t &bytes) {
    printf("Transferred %llu bytes. Status: %s\n", bytes, e.message().c_str());
}

void accept_handler(const asio::error_code &ec, tcp::socket socket) {
    try {
        const auto ep = socket.remote_endpoint();
        printf("Accepted connection %s:%d\n", ep.address().to_string().c_str(), ep.port());
        //auto buffer = asio::buffer("", 1024);
        //asio::async_read(socket, buffer);
        asio::async_write(socket, asio::buffer("Hello World!\r\n"), std::bind(std::ref(log_accept), std::placeholders::_1, std::placeholders::_2));
    }
    catch (std::exception &e) {
        std::printf("%s\n", e.what());
    }
};


void accept_client_handler(const asio::error_code &ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Accept error: " << ec.message() << "\n";
        return;
    }

    const auto ep = socket.remote_endpoint();
    std::cout << "Accepted connection "
              << ep.address().to_string()
              << ":"
              << ep.port()
              << "\n";

    auto pSocket = std::make_shared<tcp::socket>(std::move(socket));
    auto pBuffer = std::make_shared<std::array<char, BUFFER_CAP>>();
    auto read_handler = std::make_shared<std::function<void(const asio::error_code&, std::size_t)>>();

    *read_handler = [pSocket, pBuffer, read_handler](const asio::error_code& ec, std::size_t bytes){
        if (ec) {
            std::cout << "Disconnected: " << ec.message() << "\n";
            return;
        }

        std::cout.write(pBuffer->data(), bytes);
        std::cout.flush();

        pSocket->async_read_some(
            asio::buffer(*pBuffer),
            *read_handler
        );
    };

    pSocket->async_read_some(asio::buffer(*pBuffer), *read_handler);
}

int main(int argc, char **argv) {
    try {
        Server server(tcp::v4(), std::atoi(argv[1]));
        const auto local_ep = server.get_server()->local_endpoint();
        printf("Started listening on %s:%d\n", local_ep.address().to_string().c_str(), local_ep.port());
        server.accept(std::ref(accept_client_handler));
        server.get_io()->run();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << "\n";
    }
}