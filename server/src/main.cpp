#include "message_queue.h"

#include <pthread.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> 
#include <sys/select.h>
#include <unistd.h>

void setnonblock (int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

struct ReciveArgs {
    MessageQueue* msgQueue;
    int serverFd;
};

// funkcja odbierania wiadomosci
void* recive (void* args) {
    ReciveArgs* reciveArgs = (ReciveArgs*)args;
    MessageQueue* msgQueue = reciveArgs->msgQueue;
    int serverFD = reciveArgs->serverFd;

    fd_set readFDs;
    std::vector<int> clientFDs;

    char buffer[256];

    while (true) {
        FD_ZERO(&readFDs);
        FD_SET(serverFD, &readFDs);

        // dodawanie wszystkich klientow do setu i wyznaczanie max gniazda
        int maxFD = serverFD;
        for (int clientFD : clientFDs) {
            FD_SET(clientFD, &readFDs);
            if (clientFD > maxFD) {
                maxFD = clientFD;
            }
        }

        int ready = select(maxFD + 1, &readFDs, nullptr, nullptr, nullptr);
        if(ready < 0) {
            printf("select error\n");
            continue;
        }

        // akceptowanie polaczen do serwera
        if (FD_ISSET(serverFD, &readFDs)) {
            int newClientFD = accept(serverFD, nullptr, nullptr);
            if (newClientFD < 0) {
                printf("faild to accept\n");
            } else {
                setnonblock(newClientFD);
                clientFDs.push_back(newClientFD);
                printf("client %d connected\n", newClientFD);
            }
        }

        for (auto it = clientFDs.begin(); it != clientFDs.end(); ) {
            int clientFD = *it;
            if (FD_ISSET(clientFD, &readFDs)) {
                int n = read(clientFD, buffer, sizeof(buffer));
                if (n <= 0) {
                    printf("client %d disconnected\n", clientFD);
                    close(clientFD);
                    it = clientFDs.erase(it);
                } else {
                    Message msg;
                    //odczytywanie wiadomosci 
                    msg.type = buffer[0];
                    msgQueue->push(msg);
                    it++;
                }
            } else {
                it++;
            }
        }
    }
    return nullptr;
}

int main(int argc, char** argv) {
    int serverFD;
    serverFD = socket(PF_INET, SOCK_STREAM, 0);
    if (serverFD == -1) {
        perror("faild to create socket\n");
        exit(EXIT_FAILURE);
    }
    setnonblock(serverFD);

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
    pthread_t reciveThread;
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