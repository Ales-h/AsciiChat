#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <thread>

using tcp = boost::asio::ip::tcp;

class Echo: public std::enable_shared_from_this<Echo> {

    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::beast::flat_buffer buffer;
    
    public:
    Echo(tcp::socket&& socket):ws(std::move(socket)){}

    void run(){
        ws.async_accept([self{shared_from_this()}](boost::beast::error_code ec){

            if(ec) {
                std::cerr << "Accept Error in Echo: " << ec.message() << std::endl;
                 return;
            }

            self->echo();
        });
    }

    void echo(){
        ws.async_read(buffer, [self{shared_from_this()}](boost::beast::error_code ec, std::size_t byte_transfered){
            if(ec == boost::beast::websocket::error::closed) { 
                return; }
            if(ec) {
                std::cerr << "echo Error in Echo: " << ec.message() << std::endl;
                return;
            }
            std::string output = boost::beast::buffers_to_string(self->buffer.cdata());

            std::cout << output << std::endl;
        });
    }

};

class Listener : public std::enable_shared_from_this<Listener> {

    boost::asio::io_context& ioc;
    tcp::acceptor acceptor;

    public:
        Listener(boost::asio::io_context& io_con, boost::asio::ip::address ip, unsigned short int port):
        ioc(io_con), acceptor(ioc, {ip, port}) {}
    
    void asyncAccept()
    {
        acceptor.async_accept(ioc, [self{shared_from_this()}](boost::system::error_code ec, tcp::socket socket){

            if (!ec)
            {
                std::cout << "Socket Accepted" << std::endl;
            }
           self->asyncAccept(); 
        });
    } 

};

int main(int argc, char** argv) { // args: ip-address, port
    
    if(argc!=3)
    {
        std::cerr << "Usage: ./wsServer <id-address> <port> \n";
        return EXIT_FAILURE;
    }
   
    boost::asio::ip::address const address = boost::asio::ip::make_address(argv[1]);
    unsigned short const port = static_cast<unsigned short>(std::atoi(argv[2]));   

    boost::asio::io_context ioc{};

    std::make_shared<Listener>(ioc, address, port)->asyncAccept();
    
    
    ioc.run();

    //tcp::acceptor acceptor{ioc, {address, port}};

    // acceptor.async_accept([&ioc](const boost::system::error_code& ec, tcp::socket socket) {
    //     if (!ec){
    //         std::cout << "Socket accepted" << std::endl;
    //     }
    // });


   

 
    return 0;
}