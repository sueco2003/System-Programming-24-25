#include <ncurses.h>
#include <zmq.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>


void display_game_state() {
    initscr();
    noecho();
    curs_set(0);

    void *context = zmq_ctx_new();
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, PUBLISHER_ADDRESS);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    char board[BOARD_SIZE][BOARD_SIZE];
    while (1) {
        zmq_recv(subscriber, board, sizeof(board), 0);

        clear();
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                mvaddch(i, j, board[i][j]);
            }
        }
        refresh();
        usleep(100000);
    }

    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    endwin();
}

int main() {
    display_game_state();
    return 0;
}
