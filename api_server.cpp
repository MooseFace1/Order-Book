#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include "orderbook.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class BookHandler : public std::enable_shared_from_this<BookHandler> {
public:
    BookHandler(tcp::socket socket, orderbook& ob) : socket_(std::move(socket)), ob_(ob) {}
    void run() { read(); }
private:
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    orderbook& ob_;

    void read() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_, [self](auto ec, auto) {
            if (!ec) self->handle();
        });
    }

    void handle() {
        http::response<http::string_body> res{http::status::ok, req_.version()};
        res.set(http::field::server, "beast-orderbook");
        res.set(http::field::content_type, "application/json");

        if (req_.method() == http::verb::get && req_.target().starts_with("/book")) {
            std::size_t depth = 10;
            auto snap = ob_.snapshot(depth);
            std::ostringstream oss;
            oss << "{\"bids\":[";
            for (std::size_t i=0; i<snap.bids.size(); ++i) {
                if (i) oss << ',';
                oss << "{\"price\":" << snap.bids[i].price << ",\"qty\":" << snap.bids[i].qty << "}";
            }
            oss << "],\"asks\":[";
            for (std::size_t i=0; i<snap.asks.size(); ++i) {
                if (i) oss << ',';
                oss << "{\"price\":" << snap.asks[i].price << ",\"qty\":" << snap.asks[i].qty << "}";
            }
            oss << "]}";
            res.body() = oss.str();
            res.prepare_payload();
        } else if (req_.method() == http::verb::post && req_.target() == "/orders") {
            // Simple form: body = "side=buy|sell&type=limit|market&price=123.4&qty=10"
            std::string body = req_.body();
            // TODO: parse body properly; for now, return 501
            res.result(http::status::not_implemented);
            res.body() = "{\"error\":\"order submission not wired yet\"}";
            res.prepare_payload();
        } else {
            res.result(http::status::not_found);
            res.body() = "{\"error\":\"not found\"}";
            res.prepare_payload();
        }

        auto self = shared_from_this();
        http::async_write(socket_, res, [self](auto ec, auto) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
        });
    }
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(boost::asio::io_context& ioc, tcp::endpoint ep, orderbook& ob)
        : ioc_(ioc), acceptor_(ioc), ob_(ob) {
        acceptor_.open(ep.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }
    void run() { accept(); }
private:
    boost::asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    orderbook& ob_;
    void accept() {
        acceptor_.async_accept([self=shared_from_this()](auto ec, auto socket) {
            if (!ec) std::make_shared<BookHandler>(std::move(socket), self->ob_)->run();
            self->accept();
        });
    }
};

int main() {
    try {
        orderbook ob;
        boost::asio::io_context ioc{1};
        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{tcp::v4(), 8080}, ob);
        listener->run();
        ioc.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }
}
