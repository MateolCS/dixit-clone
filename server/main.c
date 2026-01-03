#include <stdio.h>
#include "server.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "game.h"
#include "helpers.h"

void launchServer(struct Server *server){

    GameState game;
    game.player_count = 0;

    printf("Server launched\n");
    printf("Waiting for connection on port %d...\n", server->port);

    while(game.player_count < 5){

        int player_fd = accept(server->socket, NULL, NULL);

        if(player_fd < 0){
            perror("Connection failed.\n");
            close(server->socket);
            exit(EXIT_FAILURE);
        }

        Player *new_player = &game.players[game.player_count];
        new_player->fd = player_fd;
        new_player->id = game.player_count;

        game.player_count++;

        printf("Player %d connected fd(%d)\n", new_player->id, new_player->fd);
        send_to_player(new_player->fd, "WELCOME Player %d\n", new_player->id);
        send_to_player(new_player->fd, "WAITING FOR PLAYERS %d/%d \n", game.player_count, 5);
    }

    printf("All players connected, game ready to be launched.\n\n");
    broadcast_to_players(&game, "ALL PLAYERS CONNECTED. STARTING GAME...\n\n");
    create_game(&game);
    printf("Game created. Dealing hands...\n\n");
    printf("Player  1 hand: %d %d %d %d %d\n\n", 
        game.players[0].hand[0],
        game.players[0].hand[1],
        game.players[0].hand[2],
        game.players[0].hand[3],
        game.players[0].hand[4]
    );

    printf("Player  2 hand: %d %d %d %d %d\n\n", 
        game.players[1].hand[0],
        game.players[1].hand[1],
        game.players[1].hand[2],
        game.players[1].hand[3],
        game.players[1].hand[4]
    );

    printf("Player  3 hand: %d %d %d %d %d\n\n", 
        game.players[2].hand[0],
        game.players[2].hand[1],
        game.players[2].hand[2],
        game.players[2].hand[3],
        game.players[2].hand[4]
    );

    printf("Player  4 hand: %d %d %d %d %d\n\n", 
        game.players[3].hand[0],
        game.players[3].hand[1],
        game.players[3].hand[2],
        game.players[3].hand[3],
        game.players[3].hand[4]
    );

    printf("Player  5 hand: %d %d %d %d %d\n\n", 
        game.players[4].hand[0],
        game.players[4].hand[1],
        game.players[4].hand[2],
        game.players[4].hand[3],
        game.players[4].hand[4]
    );

    for(int i = 0; i < game.player_count; i++){
        send_to_player(game.players[i].fd, "YOUR HAND: %d %d %d %d %d\n", 
            game.players[i].hand[0],
            game.players[i].hand[1],
            game.players[i].hand[2],
            game.players[i].hand[3],
            game.players[i].hand[4]
        );
    }

    //game_loop(&game);

}

int main(){
    struct Server server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 1100, 10, &launchServer);
    server.launchServer(&server);
}