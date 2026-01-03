#ifndef GAME_H
#define GAME_H

#include <time.h>

#define PLAYERS 5
#define HANDSIZE 5
#define TOTAL_CARDS 100
#define HINT_LENGTH 256

typedef enum {
    WAITING_FOR_PLAYERS,
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
    char* hint[HINT_LENGTH];

} GameState;




void create_game(GameState *game_state);
void start_game(GameState *game_state);
void next_phase(GameState *game_state);
void change_active_player(GameState *game_state);
void initialize_deck(GameState *game_state);
void deal_hands(GameState *game_state);
void reset_player_selections(GameState *game_state);
void game_loop(GameState *game_state);
void start_active_hint_phase(GameState *game_state);
void handle_active_hint_phase(GameState *game_state, int player_index, char *buffer);
void check_active_player_timeout(GameState *game_state);
void inactive_select_phase(GameState *game_state);
void reveal_phase(GameState *game_state);
void vote_phase(GameState *game_state);
void score_phase(GameState *game_state);


#endif