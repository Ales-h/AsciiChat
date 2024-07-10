
#include "wsSession.hpp"


using tcp = boost::asio::ip::tcp;


wsSession::wsSession(tcp::socket&& socket, std::shared_ptr<shared_state> _state):ws(std::move(socket)), state(_state){

        state->join(this);

         ws.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server, "wsServer");
        }
        ));
        }

wsSession::~wsSession(){
    //state->leave(this);
}

void wsSession::run(){
        ws.async_accept([self{shared_from_this()}](boost::beast::error_code ec){

            if(ec) {
                std::cerr << "Accept Error in wsSession: " << ec << ec.message() << std::endl;
                 return;
            }

            self->echo();
        });
    }

void wsSession::echo(){
        ws.async_read(buffer, [self{shared_from_this()}, this](boost::beast::error_code ec, std::size_t byte_transfered){
            if(ec == boost::beast::websocket::error::closed) { 
                state->leave(this);
                return; }
            if(ec) {
                std::cerr << "echo Error in wsSession: " << ec << ec.message() << std::endl;
                return;
            }
            std::string output = boost::beast::buffers_to_string(self->buffer.data());
            std::cout << output << std::endl;
            self->state->send(output);

            self->buffer.consume(self->buffer.size());

            self->echo();
        
        });
    }


void wsSession::send(std::shared_ptr<std::string> const& msg){
    
    boost::asio::post(ws.get_executor(), [self{shared_from_this()}, msg, this](){
        
        self->queue.push_back(msg);

        // Are we already writing?
        if(self->queue.size() > 1)
            return;

    // We are not currently writing, so send this immediately
    ws.async_write(
        boost::asio::buffer(*queue.front()),
        boost::beast::bind_front_handler(
            &wsSession::on_write,
            shared_from_this()));
    });
}

void wsSession::on_write(boost::beast::error_code ec, std::size_t) {
    if(ec) {
    std::cerr << "on_write Error in wsSession: " << ec << ec.message() << std::endl;
    return; 
    }

    // Remove the string from the queue
    queue.erase(queue.begin());
    std::cout << "on write" << std::endl;
    // Send the next message if any
    if(! queue.empty())
        ws.async_write(
            boost::asio::buffer(*queue.front()),
            boost::beast::bind_front_handler(
                &wsSession::on_write,
                shared_from_this()));
}

