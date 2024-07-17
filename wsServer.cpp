#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "Listener.hpp"
#include "wsSession.hpp"
#include "shared_state.hpp"

using tcp = boost::asio::ip::tcp;


int main(int argc, char** argv) { // args: ip-address, port
    
    if(argc!=3)
    {
        std::cerr << "Usage: ./wsServer <id-address> <port> \n";
        return EXIT_FAILURE;
    }
   
    boost::asio::ip::address const address = boost::asio::ip::make_address(argv[1]);
    unsigned short const port = static_cast<unsigned short>(std::atoi(argv[2]));   
    int threads = 1;

    boost::asio::io_context ioc{threads};

    std::make_shared<Listener>(ioc, address, port, std::make_shared<shared_state>())->asyncAccept();
    
    std::vector<std::thread> v;

    for(auto i = threads - 1; i > 0; --i){
        v.emplace_back([&ioc](){ioc.run();});
    }
    ioc.run();
 
    return 0;
}