#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Server server_constructor(int domain, int service, int protocol, in_addr_t interface, int port, int backlog, void (*launchServer)(struct Server *server)) {
   struct Server server;

   server.domain = domain;
   server.service = service;
   server.protocol = protocol;
   server.interface = interface;
   server.port = port;
   server.backlog = backlog;

   memset(&server.address, 0, sizeof(server.address));
   server.address.sin_addr.s_addr = htonl(interface);
   server.address.sin_family = domain;
   server.address.sin_port = htons(port);

   server.socket = socket(domain, service, protocol);
   if(server.socket == -1){
    perror("Failed to create a socket\n");
    exit(EXIT_FAILURE);
   }

   if(bind(server.socket, (struct sockaddr*)& server.address, sizeof server.address)  < 0){
    perror("Failed to bind\n");
    exit(EXIT_FAILURE);
   }

   if((listen(server.socket, server.backlog)) < 0){
    perror("Failed to listen \n");
    exit(EXIT_FAILURE);
   }

   server.launchServer = launchServer;

   return server;
}