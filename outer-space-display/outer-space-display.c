#include <ncurses.h>
#include <zmq.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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


void display_game_state()
{

    // Initialize ZMQ context and subscriber
    void *context = zmq_ctx_new();
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, PUBLISHER_ADDRESS);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "SPCINVDRS", 10);

    initscr();
    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);               // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);             // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2); // Window for the board with a border
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        mvwprintw(line_win, i, 0, "%d", i % 10);
        mvwprintw(column_win, 0, i, "%d", i % 10);
    }
    mvwprintw(score_win, 1, 3, "%s", "SCORE");
    box(board_win, 0, 0);
    box(score_win, 0, 0);
    wrefresh(line_win);
    wrefresh(column_win);

    GameState gameState;
    char topic[32];
    while (1) {
        zmq_recv(subscriber, topic, sizeof(topic) - 1, 0);
        zmq_recv(subscriber, &gameState, sizeof(GameState), 0);

        // Print the board
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) mvwaddch(board_win, i + 1, j + 1, gameState.board[i][j]); // Adjust position for border within the window
        }
        for (int i = 0; i < gameState.astronaut_count; i++) mvwprintw(score_win, 2 + i, 3, "%c - %d", gameState.astronauts[i].id, gameState.astronauts[i].score);
        wrefresh(board_win);
        wrefresh(score_win);

    }

    // Cleanup
    delwin(line_win);
    delwin(board_win);
    delwin(column_win);
    delwin(score_win);
    endwin();

    zmq_close(subscriber);
    zmq_ctx_destroy(context);
}

int main() {
    display_game_state();
    return 0;
}
