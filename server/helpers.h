#ifndef HELPERS_H
#define HELPERS_H

#include <stdarg.h>
#include "game.h"

void send_to_player(int player_fd, const char *message, ...);
void broadcast_to_players(GameState *game_state, const char *message, ...);
void parse_player_input(const char *input, char *command, char *args);
#endif