#ifndef WSSESSION_HP
#define WSSESSION_HP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "shared_state.hpp"
#include <memory>
#include <string>
#include <vector>

using tcp = boost::asio::ip::tcp;


class wsSession : public std::enable_shared_from_this<wsSession> {
    public:
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::beast::flat_buffer buffer;
    std::shared_ptr<shared_state> state;
    std::vector<std::shared_ptr<std::string>> queue;

    wsSession(tcp::socket&& socket, std::shared_ptr<shared_state> _state);
    ~wsSession();

    void run();
    void echo();
    void send(std::shared_ptr<std::string> const& msg);
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
};


#endif