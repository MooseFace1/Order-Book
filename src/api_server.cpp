#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <chrono>
#include <random>
#include <cctype>
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

    template <typename Resp>
    void addCors(Resp& res) {
        res.set(http::field::access_control_allow_origin, "*");
        res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
    }

    static bool extractString(const std::string& body, const std::string& key, std::string& out) {
        auto pos = body.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = body.find(':', pos);
        if (pos == std::string::npos) return false;
        ++pos;
        while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) ++pos;
        if (pos >= body.size() || body[pos] != '"') return false;
        auto end = body.find('"', pos + 1);
        if (end == std::string::npos) return false;
        out = body.substr(pos + 1, end - pos - 1);
        return true;
    }

    static bool extractNumber(const std::string& body, const std::string& key, double& out) {
        auto pos = body.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        pos = body.find(':', pos);
        if (pos == std::string::npos) return false;
        ++pos;
        while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) ++pos;
        char* endPtr = nullptr;
        out = std::strtod(body.c_str() + pos, &endPtr);
        return endPtr != body.c_str() + pos;
    }

    void read() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_, [self](auto ec, auto) {
            if (!ec) self->handle();
        });
    }

    void handle() {
        // Keep response alive for async_write
        auto res = std::make_shared<http::response<http::string_body>>(http::status::ok, req_.version());
        res->set(http::field::server, "beast-orderbook");

        // Preflight for CORS
        if (req_.method() == http::verb::options) {
            res->result(http::status::no_content);
            addCors(*res);
            res->prepare_payload();
        } else if (req_.method() == http::verb::get && req_.target().starts_with("/book")) {
            addCors(*res);
            res->set(http::field::content_type, "application/json");
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
            res->body() = oss.str();
            res->prepare_payload();
        } else if (req_.method() == http::verb::get && (req_.target() == "/" || req_.target() == "/index.html")) {
            addCors(*res);
            res->set(http::field::content_type, "text/html");
            std::ifstream file("web/index.html"); // path relative to repo root / working dir
            if (file) {
                std::ostringstream html;
                html << file.rdbuf();
                res->body() = html.str();
            } else {
                res->body() = "<html><body><p>Orderbook API: try <a href=\"/book\">/book</a></p></body></html>";
            }
            res->prepare_payload();
        } else if (req_.method() == http::verb::get && req_.target() == "/favicon.ico") {
            res->result(http::status::no_content);
            res->prepare_payload();
        } else if (req_.method() == http::verb::post && req_.target() == "/orders") {
            addCors(*res);
            std::string sideStr;
            std::string typeStr;
            double price = 0.0;
            double qtyNum = 0.0;

            bool sideOk = extractString(req_.body(), "side", sideStr);
            bool typeOk = extractString(req_.body(), "type", typeStr);
            bool qtyOk = extractNumber(req_.body(), "qty", qtyNum);

            if (!sideOk || !typeOk || !qtyOk) {
                res->result(http::status::bad_request);
                res->body() = "{\"error\":\"missing side/type/qty\"}";
                res->prepare_payload();
            } else {
                Side side = (sideStr == "buy" || sideStr == "Buy") ? Side::Buy : Side::Sell;
                bool isLimit = (typeStr == "limit" || typeStr == "Limit");
                ExecutionResults exec{};
                auto start = std::chrono::high_resolution_clock::now();
                if (isLimit) {
                    bool priceOk = extractNumber(req_.body(), "price", price);
                    if (!priceOk) {
                        res->result(http::status::bad_request);
                        res->body() = "{\"error\":\"missing price for limit order\"}";
                        res->prepare_payload();
                    } else {
                        exec = ob_.addLimitOrder(price, static_cast<int>(qtyNum), side);
                        auto end = std::chrono::high_resolution_clock::now();
                        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                        res->set(http::field::content_type, "application/json");
                        double avg = exec.filled > 0 ? exec.notional / static_cast<double>(exec.filled) : 0.0;
                        std::ostringstream body;
                        body << "{\"status\":\"ok\",\"filled\":" << exec.filled
                             << ",\"requested\":" << exec.requested
                             << ",\"trades\":" << exec.trades
                             << ",\"avg_price\":" << avg
                             << ",\"latency_ns\":" << ns << "}";
                        res->body() = body.str();
                        res->prepare_payload();
                    }
                } else {
                    exec = ob_.addMarketOrder(static_cast<int>(qtyNum), side);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    res->set(http::field::content_type, "application/json");
                    double avg = exec.filled > 0 ? exec.notional / static_cast<double>(exec.filled) : 0.0;
                    std::ostringstream body;
                    body << "{\"status\":\"ok\",\"filled\":" << exec.filled
                         << ",\"requested\":" << exec.requested
                         << ",\"trades\":" << exec.trades
                         << ",\"avg_price\":" << avg
                         << ",\"latency_ns\":" << ns << "}";
                    res->body() = body.str();
                    res->prepare_payload();
                }
            }
        } else if (req_.method() == http::verb::post && req_.target() == "/stimmy") {
            addCors(*res);
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<double> priceDist(95.0, 125.0);
            std::uniform_int_distribution<int> qtyDist(1, 25);
            for (int i = 0; i < 20; ++i) {
                ob_.addLimitOrder(priceDist(rng), qtyDist(rng), Side::Buy);
                ob_.addLimitOrder(priceDist(rng), qtyDist(rng), Side::Sell);
            }
            res->set(http::field::content_type, "application/json");
            res->body() = "{\"status\":\"ok\",\"added\":40}";
            res->prepare_payload();
        } else if (req_.method() == http::verb::post && req_.target() == "/clear") {
            addCors(*res);
            ob_ = orderbook{};
            res->set(http::field::content_type, "application/json");
            res->body() = "{\"status\":\"cleared\"}";
            res->prepare_payload();
        } else {
            res->result(http::status::not_found);
            addCors(*res);
            res->set(http::field::content_type, "application/json");
            res->body() = "{\"error\":\"not found\"}";
            res->prepare_payload();
        }

        auto self = shared_from_this();
        http::async_write(socket_, *res, [self, res](auto ec, auto) {
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
        // Render provides the port via the PORT env var; default to 9000 for local dev.
        unsigned short port = 9000;
        if (const char* env = std::getenv("PORT")) {
            int p = std::atoi(env);
            if (p > 0 && p < 65536) port = static_cast<unsigned short>(p);
        }
        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{tcp::v4(), port}, ob);
        listener->run();
        ioc.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }
}
