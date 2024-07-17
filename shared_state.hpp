#ifndef SHARED_STATE_HPP
#define SHARED_STATE_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_set>


class wsSession;


class shared_state {
    std::mutex mutex;
    std::unordered_set<std::shared_ptr<wsSession>> sessions;

    public:
    shared_state();
    void join(std::shared_ptr<wsSession> session);
    void leave(std::shared_ptr<wsSession> session);
    void send(std::string message);
};


#endif

