#ifndef COMMON_H
#define COMMON_H

#include <assert.h>	 // for assert
#include <ctype.h>	 // for isalnum
#include <curses.h>	 // for mvwprintw, newwin, wrefresh, mvprintw, WINDOW
#include <math.h>
#include <pthread.h>  // for pthread_create, pthread_join
#include <stdio.h>	  // for sprintf, perror
#include <stdlib.h>
#include <string.h>	  // for strlen, strncmp, memset
#include <string.h>
#include <time.h>	 // for time, time_t
#include <unistd.h>	 // for sleep, NULL, fork, usleep, pid_t
#include <zmq.h>	 // for zmq_send, zmq_close, zmq_ctx_destroy, zmq_socket
#include "../points.pb-c.h"

#define SERVER_ADDRESS "tcp://127.0.0.1:5533"
#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5554"

#define PULL_ADDRESS "tcp://127.0.0.1:5559" 
#define PUSH_ADDRESS "tcp://127.0.0.1:5564"

#define BOARD_SIZE 20
#define MAX_PLAYERS 8
#define MAX_ALIENS 256 //256
#define START_ALIENS 85 //85 // 1/3 of the board

// Message types
#define MSG_CONNECT "Astronaut_connect"
#define MSG_DISCONNECT "Astronaut_disconnect"
#define MSG_MOVE "Astronaut_movement"
#define MSG_ZAP "Astronaut_zap"
#define MSG_UPDATE "Outer_space_update"
#define MSG_SERVER "Server_terminate"


// Structs for astronaut and alien
typedef struct {
    char id;
    int x, y;
    int score;
    time_t stunned_time;
    time_t last_shot_time;
} Astronaut;

typedef struct {
    int x, y;
} Alien;

// Shared game state
typedef struct {
    Astronaut astronauts[MAX_PLAYERS];
    Alien aliens[MAX_ALIENS];
    char board[BOARD_SIZE][BOARD_SIZE];
    int astronaut_count;
    int alien_count;
} GameState;

// Constants for X and Y limits for regions
int Y_MAX[] = {0, 1, 18, 19, 17, 17, 17, 17};
int Y_MIN[] = {0, 1, 18, 19, 2, 2, 2, 2};
int X_MAX[] = {17, 17, 17, 17, 0, 1, 18, 19};
int X_MIN[] = {2, 2, 2, 2, 0, 1, 18, 19};

// Array para verificar quais IDs estão em uso (de 'A' a 'H')
int astronaut_ids_in_use[MAX_PLAYERS] = {0};  // 0: disponível, 1: em uso

int on = 1;  // Flag para manter o loop do cliente ativo

time_t last_alien_shot; // Última morte de alienígena

pthread_mutex_t mutex;
void *context, *publisher, *socket, *pusher;
GameState *gameState;
char **validation_tokens;

bool alien_placement[BOARD_SIZE][BOARD_SIZE] = {false};
#endif
