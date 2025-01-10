#include "common.h"

/**
 * Displays the current game state in a terminal window using ncurses.
 *
 * Initializes a ZeroMQ context and subscribes to a publisher to receive
 * game state updates. Sets up ncurses windows to display line numbers,
 * column numbers, the game board, and player scores. Continuously receives
 * game state updates and refreshes the display accordingly.
 *
 * Cleans up ncurses windows and ZeroMQ resources upon termination.
 */
void display_game_state() {
    // Initialize ZMQ context and subscriber
    void *context = zmq_ctx_new();
    if (!context) {
        perror("Failed to create ZeroMQ context");
        return;
    }

    void *subscriber = zmq_socket(context, ZMQ_SUB);
    if (!subscriber) {
        perror("Failed to create ZeroMQ subscriber socket");
        zmq_ctx_destroy(context);
        return;
    }

    if (zmq_connect(subscriber, PUBLISHER_ADDRESS) != 0) {
        perror("Failed to connect ZeroMQ subscriber to publisher");
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    if (zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_UPDATE, strlen(MSG_UPDATE)) != 0) {
        perror("Failed to set ZeroMQ subscription for MSG_UPDATE");
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    if (zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER)) != 0) {
        perror("Failed to set ZeroMQ subscription for MSG_SERVER");
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    if (initscr() == NULL) {
        perror("Failed to initialize ncurses");
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);
    if (!line_win) {
        perror("Failed to create ncurses line window");
        endwin();
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);
    if (!column_win) {
        perror("Failed to create ncurses column window");
        delwin(line_win);
        endwin();
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2);
    if (!board_win) {
        perror("Failed to create ncurses board window");
        delwin(line_win);
        delwin(column_win);
        endwin();
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);
    if (!score_win) {
        perror("Failed to create ncurses score window");
        delwin(line_win);
        delwin(column_win);
        delwin(board_win);
        endwin();
        zmq_close(subscriber);
        zmq_ctx_destroy(context);
        return;
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        mvwprintw(line_win, i, 0, "%d", i % 10);
        mvwprintw(column_win, 0, i, "%d", i % 10);
    }

    mvwprintw(score_win, 1, 3, "%s", "SCORE");
    box(board_win, 0, 0);
    box(score_win, 0, 0);

    GameState gameState = {0};
    char topic[256];
    while (1) {
        if (zmq_recv(subscriber, topic, sizeof(topic), 0) == -1) {
            perror("Failed to receive topic from ZeroMQ subscriber");
            break;
        }

        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            break;
        }

        if (strncmp(topic, MSG_UPDATE, strlen(MSG_UPDATE)) != 0) {
            continue;
        }

        if (zmq_recv(subscriber, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), 0) == -1) {
            perror("Failed to receive astronaut IDs from ZeroMQ subscriber");
            break;
        }

        if (zmq_recv(subscriber, &gameState, sizeof(GameState), 0) == -1) {
            perror("Failed to receive game state from ZeroMQ subscriber");
            break;
        }

        wclear(score_win);
        box(score_win, 0, 0);
        mvwprintw(score_win, 1, 3, "%s", "SCORE");

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                mvwaddch(board_win, i + 1, j + 1, gameState.board[i][j]);
            }
        }

        int j = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (astronaut_ids_in_use[i]) {
                mvwprintw(score_win, 2 + j, 3, "%c - %d", gameState.astronauts[i].id, gameState.astronauts[i].score);
                j++;
            }
        }

        refresh();
        wrefresh(board_win);
        wrefresh(score_win);
        wrefresh(line_win);
        wrefresh(column_win);
    }

    // Cleanup
    delwin(line_win);
    delwin(column_win);
    delwin(board_win);
    delwin(score_win);

    endwin();

    if (zmq_close(subscriber) != 0) {
        perror("Failed to close ZeroMQ subscriber socket");
    }

    if (zmq_ctx_destroy(context) != 0) {
        perror("Failed to destroy ZeroMQ context");
    }
}


int main() {
    display_game_state();
    return 0;
}
