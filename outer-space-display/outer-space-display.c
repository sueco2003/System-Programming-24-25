#include <ncurses.h>
#include <zmq.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void display_game_state()
{
    // Initialize ncurses for user input/output
    initscr();
    noecho();
    clear();

    // Initialize ZMQ context and subscriber
    void *context = zmq_ctx_new();
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    

    int rc = zmq_connect(subscriber, PUBLISHER_ADDRESS);
    if (rc != 0)
    {
        mvprintw(0, 0, "Failed to connect to publisher.");
        refresh();
        return;
    }

    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // Game board and score buffers
    char board[BOARD_SIZE * BOARD_SIZE + 1];
    char scoring[MAX_PLAYERS * 4];

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);               // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);             // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2); // Window for the board with a border
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        mvwprintw(line_win, i, 0, "%d", i % 10);
        mvwprintw(column_win, 0, i, "%d", i % 10);
    }
    mvwprintw(score_win, 1, 3, "%s", "SCORE");

    box(board_win, 0, 0);
    box(score_win, 0, 0);
    wrefresh(column_win);
    wrefresh(line_win);
    wrefresh(score_win);
    wrefresh(board_win);
    while (1)
    {
        // Receive the game board state
        if (zmq_recv(subscriber, board, sizeof(board), 0) == -1)
        {
            mvprintw(0, 0, "Error receiving board data.");
            refresh();
            break;
        }

        // Receive the scoring state
        if (zmq_recv(subscriber, scoring, sizeof(scoring), 0) == -1)
        {
            mvprintw(0, 0, "Error receiving score data.");
            refresh();
            break;
        }

        int index = 0;
        // Print the game board
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                mvwaddch(board_win, i + 1, j + 1, board[index++]); // Adjust for border
            }
        }
        wrefresh(board_win);

        // Print scores
        char *token = strtok(scoring, "\n");
        int i = 0;
        while (token != NULL && i < MAX_PLAYERS)
        {
            mvwprintw(score_win, i + 2, 3, "%c - %s", 'A' + i, token);
            token = strtok(NULL, "\n"); // Continue tokenizing the string
            i++;
        }
        wrefresh(score_win);
    }

    // Cleanup
    delwin(line_win);
    delwin(board_win);
    delwin(column_win);
    delwin(score_win);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    endwin();
}

int main()
{
    display_game_state();
    return 0;
}
