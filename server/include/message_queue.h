#pragma once
#include <queue>
#include <pthread.h>
#include <cstdint>

struct Message {
    char type;
};

class MessageQueue {

    public:
        MessageQueue();
        ~MessageQueue();
    
        void push(Message msg);
        Message pop();

    private:
        std::queue<Message> m_queue;
        pthread_mutex_t m_mutex;
        pthread_cond_t m_condVar;

};
