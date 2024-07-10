#include "Listener.hpp"
#include <boost/asio/strand.hpp>


Listener::Listener(boost::asio::io_context& io_con, boost::asio::ip::address ip, unsigned short int port, std::shared_ptr<shared_state> state_):
ioc(io_con), acceptor(boost::asio::make_strand(ioc), {ip, port}), state(state_) {}

void Listener::asyncAccept()
    {
        acceptor.async_accept(boost::asio::make_strand(ioc), [self{shared_from_this()}](boost::system::error_code ec, tcp::socket socket){

            if (!ec)
            {
                std::shared_ptr<wsSession> client = std::make_shared<wsSession>(std::move(socket), self->state);
                client->run();
                std::cout << "Socket Accepted" << std::endl;
            }
           self->asyncAccept(); 
        });
    } 


