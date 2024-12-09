#ifndef COMMON_H
#define COMMON_H

#include <curses.h>  // for delwin, mvwprintw, newwin, wrefresh, WINDOW, box
#include <time.h>    // for time_t
#include <zmq.h>     // for zmq_recv, zmq_close, zmq_connect, zmq_ctx_destroy

#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5546"

#define BOARD_SIZE 20
#define MAX_PLAYERS 8
#define MAX_ALIENS 166 // 1/3 of the board

// Structs for astronaut and alien
typedef struct
{
    char id;
    int x, y;
    int score;
    time_t stunned_time;
    time_t last_shot_time;
} Astronaut;

typedef struct
{
    int x, y;
} Alien;

// Shared game state
typedef struct
{
    Astronaut astronauts[MAX_PLAYERS];
    Alien aliens[MAX_ALIENS];
    char board[BOARD_SIZE][BOARD_SIZE];
    int astronaut_count;
    int alien_count;
} GameState;





#endif
