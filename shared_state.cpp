
#include "shared_state.hpp"
#include "wsSession.hpp"


shared_state::shared_state() {}

void shared_state::join(std::shared_ptr<wsSession> session){
    std::lock_guard<std::mutex> lock(mutex);
    sessions.insert(session);
}
void shared_state::leave(std::shared_ptr<wsSession> session){ 
    std::lock_guard<std::mutex> lock(mutex); 
    sessions.erase(session);
}

void shared_state::send(std::string message){
    std::shared_ptr<std::string> const shared_mess = std::make_shared<std::string>(std::move(message));
    std::vector<std::weak_ptr<wsSession>> v;
    {
        std::lock_guard<std::mutex> lock(mutex);
        v.reserve(sessions.size());
    
        for(auto p: sessions){
            
            v.emplace_back(p->weak_from_this());
        }
    }

    for(auto const& wp: v){
            if(auto sp = wp.lock())
            {
                sp->send(shared_mess);
            }
        }
    

}





