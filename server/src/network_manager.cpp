#include "network_manager.h"

#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "serializer.h"

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {}

// stworzenie serwera
void NetworkManager::initialize() {
    m_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(1100);

    bind(m_serverSocketFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(m_serverSocketFd, 5);
}

void NetworkManager::shutdown() {
    close(m_serverSocketFd);

    for (const auto& connection : m_connections) {
        close(connection.socketFd);
    }
}

// odbieranie wiadomości od klientów
std::vector<NetworkEvent> NetworkManager::poolMessages() {
    std::vector<NetworkEvent> events;

    std::vector<pollfd> fds;
    fds.reserve(m_connections.size() + 1);

    fds.push_back({ m_serverSocketFd, POLLIN, 0 });
    for (const auto& connection : m_connections) {
        fds.push_back({ connection.socketFd, POLLIN, 0 });
    }

    // sprawdzenie, które gniazda mają dane do odczytu
    int readyCount = poll(fds.data(), fds.size(), 0);
    if (readyCount < 0) return events;

    // obsługa nowych połączeń
    if (fds[0].revents & POLLIN) {
        int clientSocket = accept(m_serverSocketFd, nullptr, nullptr);
        if (clientSocket >= 0) {
            m_connections.push_back({ clientSocket, m_nextId });
            m_nextId++;
        }
    }

    // obsługa wiadomości od istniejących połączeń
    std::vector<int> disconnectedFds;
    for (size_t i = 1; i < fds.size(); i++) {
        if (fds[i].revents & POLLIN) {
            uint32_t messageLength = 0;
            ssize_t bytesRead = read(fds[i].fd, &messageLength, sizeof(uint32_t));
            // obsługa rozłączenia
            if (bytesRead <= 0) {
                close(fds[i].fd);
                disconnectedFds.push_back(fds[i].fd);
                continue;
            }

            // odczyt wiadomości
            std::vector<uint8_t> buffer(messageLength);
            read(fds[i].fd, buffer.data(), messageLength);

            size_t senderId = 0;
            for (const auto& connection : m_connections) {
                if (connection.socketFd == fds[i].fd) {
                    senderId = connection.id;
                    break;
                }
            }

            // deserializacja wiadomości
            auto messageOpt = deserializeMesssage(buffer);
            if (messageOpt.has_value()) {
                events.push_back({ senderId, messageOpt.value() });
            }
        }
    }

    return events;
}

// wysyłanie wiadomości do klienta
void NetworkManager::sendMessage(size_t receiverId, const Message& message) {
    for (const auto& connection : m_connections) {
        if (connection.id != receiverId) continue;

        pollfd fd {
            .fd = connection.socketFd,
            .events = POLLOUT,
            .revents = 0
        };

        // sprawdzenie, czy gniazdo jest gotowe do wysyłania
        int result = poll(&fd, 1, 0);
        if (result > 0 && (fd.revents & POLLOUT)) {
            auto serializedMessage = serializeMessage(message);
            uint32_t messageLength = static_cast<uint32_t>(serializedMessage.size());

            // wysłanie 
            write(connection.socketFd, &messageLength, sizeof(uint32_t));
            write(connection.socketFd, serializedMessage.data(), serializedMessage.size());
        }
    }
}
