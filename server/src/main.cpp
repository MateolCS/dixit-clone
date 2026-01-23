#include "message_queue.h"

#include <pthread.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> 
#include <sys/poll.h>
#include <unistd.h>

struct ReciveArgs {
    MessageQueue* msgQueue;
    int serverFd;
};

// funkcja odbierania wiadomosci
void* recive (void* args) {
    ReciveArgs* reciveArgs = (ReciveArgs*)args;
    MessageQueue* msgQueue = reciveArgs->msgQueue;
    int serverFD = reciveArgs->serverFd;

    std::vector<int> clientFDs;

    char buffer[256];

    while (true) {

        std::vector<pollfd> pollFDs;
        pollFDs.reserve(clientFDs.size() + 1);

        // dodanie deskryptora serwera do nasluchiwania
        pollFDs.push_back({serverFD, POLLIN, 0});
        for (int clientFD : clientFDs) {
            pollFDs.push_back({clientFD, POLLIN, 0});
        }

        // oczekiwanie na zdarzenia
        int ready = poll(pollFDs.data(), pollFDs.size(), -1);
        if(ready < 0) {
            printf("poll error\n");
            continue;
        }

        // akceptowanie polaczen do serwera
        if (pollFDs[0].revents & POLLIN) {
            int clientFD = accept(serverFD, nullptr, nullptr);
            if (clientFD != -1) {
                printf("client %d connected\n", clientFD);
                clientFDs.push_back(clientFD);
            }
        }
        
        // odbieranie danych od klientow
        std::vector<int> disconnectedClients;
        for (size_t i = 1; i < pollFDs.size(); i++) {
            if (pollFDs[i].revents & POLLIN) {
                ssize_t bytesRead = read(pollFDs[i].fd, buffer, sizeof(buffer));
                if (bytesRead <= 0) {
                    // dodanie do listy rozlaczonych klientow
                    disconnectedClients.push_back(pollFDs[i].fd);
                    close(pollFDs[i].fd);
                    printf("client %d disconnected\n", pollFDs[i].fd);
                    continue;
                } 
                // przetwarzanie odebranych danych
                Message msg;
                msg.type = buffer[0];
                msgQueue->push(msg);
            } 
        }

        // usuwanie rozlaczonych klientow
        for (auto discClient : disconnectedClients) {
            clientFDs.erase(std::remove(clientFDs.begin(), clientFDs.end(), discClient), clientFDs.end());
        }

    }
    return nullptr;
}

void* send (void* args) {
    // do zaimplementowania
    return nullptr;
}

int main(int argc, char** argv) {
    int serverFD;
    serverFD = socket(PF_INET, SOCK_STREAM, 0);
    if (serverFD == -1) {
        perror("faild to create socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof serverAddr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1100);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("faild to bind\n");
        exit(EXIT_FAILURE);
    }

    listen(serverFD, 5);

    MessageQueue inputQueue = MessageQueue();
    pthread_t reciveThread, sendThread;
    ReciveArgs reciveArgs;
    reciveArgs.msgQueue = &inputQueue;
    reciveArgs.serverFd = serverFD;

    pthread_create(&reciveThread, nullptr, recive, &reciveArgs);
    pthread_detach(reciveThread);

    while(true) {
        Message msg = inputQueue.pop();
        std::cout << msg.type << std::endl;
        if (msg.type == '0') {
            break;
        }
    }

    return 0;
}