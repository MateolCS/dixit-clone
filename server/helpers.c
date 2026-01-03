#include "helpers.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
 #include <sys/socket.h>

#define BUFFER_SIZE 1024


void send_to_player(int player_fd, const char *message, ...) {
    va_list args;
    char buffer[BUFFER_SIZE];

    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    if(send(player_fd, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send message to player\n");
    }
}


void broadcast_to_players(GameState *game_state, const char *message, ...) {
    va_list args;
    char buffer[BUFFER_SIZE];

    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);

    for(int i = 0; i < game_state->player_count; i++) {
        if(send(game_state->players[i].fd, buffer, strlen(buffer), 0) < 0) {
            perror("Failed to broadcast message to player\n");
            printf("Player ID: %d, FD: %d\n", game_state->players[i].id, game_state->players[i].fd);
        }
    }
}
