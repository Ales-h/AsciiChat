#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "shared_state.hpp"
#include "wsSession.hpp"
#include <memory>
#include <string>
#include <vector>


using tcp = boost::asio::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(boost::asio::io_context& ioc, 
             boost::asio::ip::address ip, 
             unsigned short int port,
             std::shared_ptr<shared_state> state_);

    void asyncAccept(); // Start accepting connections asynchronously

private:
    boost::asio::io_context& ioc;     // Reference to the io_context
    tcp::acceptor acceptor;          // Acceptor for incoming connections
    std::shared_ptr<shared_state> state;
    
};


#endif