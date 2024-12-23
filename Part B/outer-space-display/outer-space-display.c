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
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, PUBLISHER_ADDRESS);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_UPDATE, strlen(MSG_UPDATE));

    initscr();
    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);    // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);  // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2,
                               2);  // Window for the board with a border
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
    char topic[strlen(MSG_UPDATE)];
    while (1) {
        // Receive game state updates
        zmq_recv(subscriber, topic, sizeof(topic), 0);
        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            break;
        }
        zmq_recv(subscriber, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), 0);
        zmq_recv(subscriber, &gameState, sizeof(GameState), 0);

        wclear(score_win);
        box(score_win, 0, 0);
        mvwprintw(score_win, 1, 3, "%s", "SCORE");

        // Print the board
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) mvwaddch(board_win, i + 1, j + 1, gameState.board[i][j]);  // Adjust position for border within the window
        }
        int j = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (astronaut_ids_in_use[i]) {
                mvwprintw(score_win, 2 + j, 3, "%c - %d", gameState.astronauts[i].id, gameState.astronauts[i].score);
                j++;
            }
        }

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
