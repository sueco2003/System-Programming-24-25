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
    WINDOW *line_win = newwin(BOARD_SIZE +2, 1, 3, 1); // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3); // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2); // Window for the board with a border
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);

    for (int i = 0; i < BOARD_SIZE; i++) {
        mvwprintw(line_win, i, 0, "%d", i%10);
        mvwprintw(column_win, 0, i, "%d", i%10);
    }
       
    box(board_win, 0, 0);
    box(score_win, 0, 0);
    wrefresh(column_win);
    wrefresh(line_win);
    wrefresh(score_win);

    while (1) {

        zmq_recv(subscriber, board, sizeof(board), 0);

        // Print the board
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                mvwaddch(board_win, i + 1, j + 1, board[i][j]); // Adjust position for border within the window
            }
        }
        wrefresh(board_win);

        usleep(100000);
    }

    delwin(line_win);
    delwin(board_win);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    endwin();
}

int main() {
    display_game_state();
    return 0;
}