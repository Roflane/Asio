#include <iostream>
#include <array>
#include <vector>
#include <asio.hpp>

using asio::ip::tcp;

class Server {
private:
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

    template <typename AcceptHandlerT>
    void accept(AcceptHandlerT accept_handler) {
        auto accept_lambda = [this, accept_handler = std::move(accept_handler)](const asio::error_code &ec, tcp::socket socket) {
            accept_handler(ec, std::move(socket));
            this->accept(accept_handler);
        };
        _pServer->async_accept(*_pIo, accept_lambda);
    }

};
Server server(tcp::v4(), 7373);

void log_accept(const std::error_code &e, const size_t &bytes) {
    printf("Transferred %llu bytes. Status: %s\n", bytes, e.message().c_str());
}

void accept_handler(const asio::error_code &ec, tcp::socket socket) {
    try {
        const auto ep = socket.remote_endpoint();
        printf("Accepted connection %s:%d\n", ep.address().to_string().c_str(), ep.port());
        auto buffer = asio::buffer("", 1024);
        //asio::async_read(socket, buffer);
        asio::async_write(socket, asio::buffer("Hello World!\r\n"), std::bind(log_accept, std::placeholders::_1, std::placeholders::_2));
    }
    catch (std::exception &e) {
        std::printf("%s\n", e.what());
    }
};


void accept_client_handler(const asio::error_code &ec, tcp::socket socket) {
    try {
        const auto ep = socket.remote_endpoint();
        printf("Accepted connection %s:%d\n", ep.address().to_string().c_str(), ep.port());
        auto buf = std::make_shared<asio::streambuf>();
        auto socket = std::make_shared<tcp::socket>(*server.get_io());

        std::thread([socket = std::move(socket), buf = std::move(buf)]() {
            std::function<void()> do_read;
            do_read = [socket, buf, &do_read]() {
               socket->async_read_some(buf->prepare(1024),
                   [socket, buf, &do_read](std::error_code ec, std::size_t bytes) {
                       if (bytes > 0) {
                           buf->commit(bytes);
                           std::istream is(buf.get());
                           std::string data(bytes, 0);
                           is.read(&data[0], bytes);
                           std::cout << data << std::endl;

                           printf("Read %d bytes\n", bytes);

                           buf->consume(bytes);
                           do_read();
                       } else {
                           if (ec != asio::error::eof) {
                               std::cerr << ec.message() << std::endl;
                           }
                           std::printf("Disconnected: %s\n", socket->remote_endpoint().address().to_string().c_str());
                           socket->close();
                       }
                   });
                do_read();
           };
           do_read();
        }).detach();
    }
    catch (std::exception &e) {
       std::printf("%s\n", e.what());
    }
};


int main() {
    try {
        const auto local_ep = server.get_server()->local_endpoint();
        printf("Started listening on %s:%d\n", local_ep.address().to_string().c_str(), local_ep.port());
        server.accept(std::ref(accept_client_handler));
        server.get_io()->run();
    }
    catch (std::exception &e) {
        std::printf("%s\n", e.what());
    }
}