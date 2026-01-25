#pragma once

#include <vector>

#include "messages.h"

class NetworkManager {

public:
    NetworkManager();
    ~NetworkManager();

    void initialize();
    void shutdown();

    std::vector<NetworkEvent> poolMessages();
    void sendMessage(size_t receiverId, const Message& message);

private:
    struct Connection {
        int socketFd;
        size_t id;
    };

    int m_serverSocketFd;
    std::vector<Connection> m_connections;
    size_t m_nextId = 0;

};