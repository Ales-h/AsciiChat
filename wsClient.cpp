#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>


using tcp = boost::asio::ip::tcp;


void echo(boost::beast::websocket::stream<tcp::socket>& ws) {
    
}

void on_echo() {

}

void read(boost::beast::flat_buffer& buffer, boost::beast::websocket::stream<tcp::socket>& ws){

        ws.async_read(buffer, [&ws, &buffer](boost::beast::error_code ec, std::size_t byte_transfered){
            if(ec == boost::beast::websocket::error::closed) { 
                ws.close(boost::beast::websocket::close_code::normal);
                return;
                }
            if(ec) {
                std::cerr << "read Error in wsClient: " << ec << ec.message() << std::endl;
                return;
            }
            std::string output = boost::beast::buffers_to_string(buffer.data());
            std::cout << output << std::endl;

            buffer.consume(buffer.size());

            read(buffer, ws);
        
        });

}

int main(int argc, char** argv ){
    try{
    if(argc != 3){
        std::cerr << "Usage: ./wsClient <ip-address> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    boost::asio::ip::address const address = boost::asio::ip::make_address(argv[1]);
    unsigned short const port = static_cast<unsigned short>(std::atoi(argv[2])); 
    std::string username;

    std::cout << "Your username: " << std::endl;
    std::cin >> username;

    boost::asio::io_context ioc;

    tcp::resolver resolver{ioc};
    boost::beast::websocket::stream<tcp::socket> ws{ioc}; // local websocket object
    tcp::endpoint endpoint(address, port); 
    auto const results = resolver.resolve(endpoint);

    // tcp connection
    boost::asio::connect(ws.next_layer(), results);

    ws.handshake(argv[2], "/");

    boost::beast::flat_buffer buffer;

    read(buffer, ws);

    ioc.run();

    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}