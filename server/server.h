#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Server {
    int domain;
    int service;
    int protocol;
    in_addr_t interface;
    int port;
    int backlog;

    int socket;

    struct sockaddr_in address;
    void (*launchServer)(struct Server *server);
    
};

//Definition of server constructor, whats need to be passed for it to work
struct Server server_constructor(int domain, int service, int protocol, in_addr_t interface, int port, int backlog,void (*launchServer)(struct Server *server));

#endif