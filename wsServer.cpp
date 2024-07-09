#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;

class Echo: public std::enable_shared_from_this<Echo> {

    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::beast::flat_buffer buffer;
    
    public:
    Echo(tcp::socket&& socket):ws(std::move(socket)){

         ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server, "wsServer");
        }
        ));
        }

    void run(){
        ws.async_accept([self{shared_from_this()}](boost::beast::error_code ec){

            if(ec) {
                std::cerr << "Accept Error in Echo: " << ec << ec.message() << std::endl;
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
                std::cerr << "echo Error in Echo: " << ec << ec.message() << std::endl;
                return;
            }
            std::string output = boost::beast::buffers_to_string(self->buffer.cdata());

            std::cout << output << std::endl;   

            self->ws.async_write(self->buffer.data(),[self](boost::beast::error_code ec, std::size_t byte_transfered){
                if(ec) {
                    std::cerr << ec << ec.message() << std::endl;
                    return;
                }

                self->buffer.consume(self->buffer.size());

                self->echo();
            });
        });
    }

};

class Listener : public std::enable_shared_from_this<Listener> {

    boost::asio::io_context& ioc;
    tcp::acceptor acceptor;

    public:
        Listener(boost::asio::io_context& io_con, boost::asio::ip::address ip, unsigned short int port):
        ioc(io_con), acceptor(boost::asio::make_strand(ioc), {ip, port}) {}
    
    void asyncAccept()
    {
        acceptor.async_accept(boost::asio::make_strand(ioc), [self{shared_from_this()}](boost::system::error_code ec, tcp::socket socket){

            if (!ec)
            {
                std::make_shared<Echo>(std::move(socket))->run();
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
    int threads = 4;

    boost::asio::io_context ioc{threads};

    std::make_shared<Listener>(ioc, address, port)->asyncAccept();
    
    std::vector<std::thread> v;

    for(auto i = threads - 1; i > 0; --i){
        v.emplace_back([&ioc](){ioc.run();});
    }
    ioc.run();
 
    return 0;
}