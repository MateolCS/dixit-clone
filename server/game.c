#include <stdlib.h> 
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "helpers.h"
#define BUFFER_SIZE 256


void create_game(GameState *game_state) {
    game_state->top_card = 0;
    game_state->active_player = 0;
    game_state->current_phase = WAITING_FOR_PLAYERS;
    game_state->phase_start = time(NULL);
    initialize_deck(game_state);
    deal_hands(game_state);
    reset_player_selections(game_state);
}

void start_active_hint_phase(GameState *game_state) {
    broadcast_to_players(game_state, "WAITING FOR %d player selection\n", game_state->active_player);
    game_state->current_phase = ACTIVE_HINT;
    game_state->phase_start = time(NULL);

    Player *active_player = &game_state->players[game_state->active_player];
    active_player->selected_card = -1;
    active_player->has_voted = 0;
    send_to_player(active_player->fd, "YOUR TURN SELECT CARD AND GIVE A HINT\nFormat: HINT <card_number> <word>\nYou have 15 seconds\n");
}

void handle_active_hint_phase(GameState *game_state, int player_index, char *buffer) {
  
    if(player_index != game_state->active_player) {
        send_to_player(game_state->players[player_index].fd, "NOT YOUR TURN TO GIVE A HINT\n");
        return;
    }

    int card;
    char hint[HINT_LENGTH];
    if(sscanf(buffer, "HINT %d %[^\n]", &card, hint) != 2) {
        send_to_player(game_state->players[player_index].fd, "INVALID HINT FORMAT. USE: HINT <card_number> <word>\n");
        return;
    }

    Player *active_player = &game_state->players[player_index];
    int card_found = 0;
    for(int i = 0; i < active_player->hand_size; i++) {
        if(active_player->hand[i] == card) {
            card_found = 1;
            break;
        }
    }

    if(!card_found) {
        send_to_player(active_player->fd, "YOU DON'T HAVE THAT CARD IN YOUR HAND.\n");
        return;
    }

    active_player->selected_card = card;
    strncpy(game_state->hint, hint, HINT_LENGTH);
    broadcast_to_players(game_state, "PLAYER %d GAVE HINT: %s\n", active_player->id, hint);
    printf("Player %d selected card %d with hint: %s\n", active_player->id, card, hint);
}

void check_active_hint_timeout(GameState *game_state) {
    if (game_state->current_phase != ACTIVE_HINT)
        return;

    if (time(NULL) - game_state->phase_start < 15)
        return;

    Player *active_player = &game_state->players[game_state->active_player];

    int idx = rand() % active_player->hand_size;
    active_player->selected_card = active_player->hand[idx];
    strcpy(game_state->hint, "random");

    send_to_player(active_player->fd,
        "Time expired. Random card and hint chosen.");

    for (int i = 0; i < game_state->player_count; i++) {
        if (i != game_state->active_player) {
            send_to_player(game_state->players[i].fd,
                "Hint: %s", game_state->hint);
        }
    }

    start_inactive_select_phase(game_state);
}



void game_loop(GameState *game_state) {
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    int max_fd;
    printf("Game loop started.\n");
    while(1){

        switch (game_state->current_phase)
        {
        case ACTIVE_HINT:
            check_active_hint_timeout(game_state);
            break;
        
        default:
            break;
        }

        FD_ZERO(&read_fds);
        max_fd = 0;
        for(int i = 0; i < game_state->player_count; i++){
            int fd = game_state->players[i].fd;
            FD_SET(fd, &read_fds);
            if(fd > max_fd){
                max_fd = fd;
            }
        }

        struct timeval tv;
        tv.tv_sec = 1; 
        tv.tv_usec = 0;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);


        if(activity < 0){
            perror("Select error in game loop\n");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < game_state->player_count; i++){
            int fd = game_state->players[i].fd;
            if(FD_ISSET(fd, &read_fds)){
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
                if(bytes_read <= 0){
                    printf("Player %d disconnected.\n", game_state->players[i].id);
                    close(fd);
                }
                buffer[strcspn(buffer, "\n")] = 0;
                send_to_player(fd, "ECHO: %s\n", buffer);
                printf("Received from Player %d: %s\n", game_state->players[i].id, buffer);
            }
        }
    }
}




void initialize_deck(GameState *game_state) {
    for (int i = 0; i < TOTAL_CARDS; i++) {
        game_state->cards[i] = i;
    }

    srand((unsigned int)time(NULL));
    for (int i = TOTAL_CARDS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = game_state->cards[i];
        game_state->cards[i] = game_state->cards[j];
        game_state->cards[j] = temp;
    }

    game_state->top_card = 0;
}

void deal_hands(GameState *game_state) {
    printf("Deck: ");
    for (int i = 0; i < TOTAL_CARDS; i++) {
        printf("%d ", game_state->cards[i]);
    }
    printf("\n");
    for (int i = 0; i < game_state->player_count; i++) {
        Player *player = &game_state->players[i];
        player->hand_size = HANDSIZE;
        for (int j = 0; j < HANDSIZE; j++) {
            player->hand[j] = game_state->cards[game_state->top_card++];
        }
    }
}

void reset_player_selections(GameState *game_state) {
    for (int i = 0; i < game_state->player_count; i++) {
        game_state->players[i].selected_card = -1;
        game_state->players[i].voted_card = -1;
        game_state->players[i].has_voted = 0;
    }
}

void change_active_player(GameState *game_state) {
    game_state->active_player = (game_state->active_player + 1) % game_state->player_count;
}



