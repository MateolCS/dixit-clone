#include "message_queue.h"

MessageQueue::MessageQueue() {
    pthread_mutex_init(&m_mutex, nullptr);
    pthread_cond_init(&m_condVar, nullptr);
}

MessageQueue::~MessageQueue() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condVar);
}

void MessageQueue::push(Message msg) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(msg);
    pthread_cond_signal(&m_condVar);
    pthread_mutex_unlock(&m_mutex);

}

Message MessageQueue::pop() {
    pthread_mutex_lock(&m_mutex);
    while(m_queue.empty()) {
        pthread_cond_wait(&m_condVar, &m_mutex);
    }
    Message msg = m_queue.front();
    m_queue.pop();
    pthread_mutex_unlock(&m_mutex);
    return msg;
}
