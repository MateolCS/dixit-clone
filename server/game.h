#ifndef GAME_H
#define GAME_H

#include <time.h>

#define PLAYERS 5
#define HANDSIZE 5
#define TOTAL_CARDS 100


typedef enum {
    WAITING_FOR_PLAYERS,
    DEAL_CARDS,
    ACTIVE_HINT,
    INACTIVE_SELECT,
    REVEAL,
    VOTE,
    SCORE,
    DRAW,
    GAME_OVER
} GamePhase;

typedef struct{
    int id;
    int fd;
    int score;
    int hand[HANDSIZE];
    int hand_size;
    int selected_card;
    int voted_card;
    int has_voted;
} Player;

typedef struct{
    Player players[PLAYERS];
    int player_count;
    int cards[TOTAL_CARDS];
    int top_card;
    int active_player;
    GamePhase current_phase;
    time_t phase_start;
} GameState;




void create_game(GameState *game_state);
void start_game(GameState *game_state);
void next_phase(GameState *game_state);

#endif