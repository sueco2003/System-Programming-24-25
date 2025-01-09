#include "common.h"

int quit_flag = 0;  // Global flag to signal quit
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
void *display_game_state() {
    subscriber = zmq_socket(context, ZMQ_SUB);
    if (!subscriber) {
        perror("Failed to create ZeroMQ subscriber socket");
        return NULL;
    }

    if (zmq_connect(subscriber, PUBLISHER_ADDRESS) != 0) {
        perror("Failed to connect to ZeroMQ publisher");
        zmq_close(subscriber);
        return NULL;
    }

    if (zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_UPDATE, strlen(MSG_UPDATE)) != 0) {
        perror("Failed to set ZMQ_SUBSCRIBE option for MSG_UPDATE");
        zmq_close(subscriber);
        return NULL;
    }

    if (zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER)) != 0) {
        perror("Failed to set ZMQ_SUBSCRIBE option for MSG_SERVER");
        zmq_close(subscriber);
        return NULL;
    }

    initscr();
    if (stdscr == NULL) {
        perror("Failed to initialize ncurses");
        zmq_close(subscriber);
        return NULL;
    }

    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);
    if (!line_win) {
        perror("Failed to create line_win window");
        endwin();
        zmq_close(subscriber);
        return NULL;
    }

    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);
    if (!column_win) {
        perror("Failed to create column_win window");
        delwin(line_win);
        endwin();
        zmq_close(subscriber);
        return NULL;
    }

    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2);
    if (!board_win) {
        perror("Failed to create board_win window");
        delwin(line_win);
        delwin(column_win);
        endwin();
        zmq_close(subscriber);
        return NULL;
    }

    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);
    if (!score_win) {
        perror("Failed to create score_win window");
        delwin(line_win);
        delwin(column_win);
        delwin(board_win);
        endwin();
        zmq_close(subscriber);
        return NULL;
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
    while (!quit_flag) {
        if (zmq_recv(subscriber, topic, sizeof(topic), 0) == -1) {
            break;
        }

        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            zmq_close(socket);
            endwin();
            zmq_close(subscriber);
            zmq_ctx_destroy(context);
            exit(0);
        } else if (strncmp(topic, MSG_UPDATE, strlen(MSG_UPDATE)) != 0) {
            continue;
        }

        if (zmq_recv(subscriber, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), 0) == -1) {
            perror("Failed to receive astronaut IDs");
            break;
        }

        if (zmq_recv(subscriber, &gameState, sizeof(GameState), 0) == -1) {
            perror("Failed to receive game state");
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

        int player_count = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (astronaut_ids_in_use[i]) {
                mvwprintw(score_win, 2 + player_count, 3, "%c - %d", gameState.astronauts[i].id, gameState.astronauts[i].score);
                player_count++;
            }
        }

        refresh();
        wrefresh(board_win);
        wrefresh(score_win);
        wrefresh(line_win);
        wrefresh(column_win);
    }

    sleep(2);
    delwin(board_win);
    delwin(score_win);
    delwin(line_win);
    delwin(column_win);
    endwin();
    return NULL;
}

/**
 * Runs the client application, handling user input and server communication.
 *
 * This function initializes the ncurses library for user input and output,
 * sets up a ZeroMQ context and socket to communicate with the server, and
 * processes user commands to send movement or action messages. It displays
 * server responses and handles disconnection gracefully.
 */
void *run_client() {
    initscr();
    if (stdscr == NULL) {
        perror("Failed to initialize ncurses");
        return NULL;
    }

    keypad(stdscr, TRUE);
    noecho();

    socket = zmq_socket(context, ZMQ_REQ);
    if (!socket) {
        perror("Failed to create ZeroMQ socket");
        endwin();
        return NULL;
    }

    if (zmq_connect(socket, SERVER_ADDRESS) != 0) {
        perror("Failed to connect to ZeroMQ server");
        zmq_close(socket);
        endwin();
        return NULL;
    }

    if (zmq_send(socket, MSG_CONNECT, strlen(MSG_CONNECT), 0) == -1) {
        perror("Failed to send connection message to server");
        zmq_close(socket);
        endwin();
        return NULL;
    }

    char response[65];
    int bytes = zmq_recv(socket, response, sizeof(response), 0);
    if (bytes == -1) {
        perror("Failed to receive response from server");
        zmq_close(socket);
        endwin();
        return NULL;
    }
    response[bytes] = '\0';

    if (sscanf(response, "Welcome! You are player %c %s", &astronaut_id, token) != 2) {
        perror("Failed to parse server response");
        zmq_close(socket);
        endwin();
        return NULL;
    }

    mvprintw(BOARD_SIZE + 5, 0, "Welcome! You are player %c", astronaut_id);
    mvprintw(BOARD_SIZE + 6, 0, "- - - - - - - - - - - - - - - - -");
    refresh();

    while (!quit_flag) {
        int ch = getch();
        char message[38];
        if (ch == KEY_UP) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'U', token);
        else if (ch == KEY_DOWN) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'D', token);
        else if (ch == KEY_LEFT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'L', token);
        else if (ch == KEY_RIGHT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'R', token);
        else if (ch == ' ') sprintf(message, "%s %c %s", MSG_ZAP, astronaut_id, token);
        else if (ch == 'q' || ch == 'Q') {
            sprintf(message, "%s %c %s", MSG_DISCONNECT, astronaut_id, token);
            quit_flag = 1;
        } else continue;

        if (zmq_send(socket, message, strlen(message), 0) == -1) {
            perror("Failed to send message to server");
            break;
        }

        bytes = zmq_recv(socket, response, sizeof(response), 0);
        if (bytes == -1) {
            perror("Failed to receive response from server");
            break;
        }
        response[bytes] = '\0';

        mvprintw(BOARD_SIZE + 7, 0, "%s", response);
        clrtoeol();
        refresh();
    }

    mvprintw(BOARD_SIZE + 9, 0, "Thanks for playing! See you soon!");
    refresh();
    sleep(2);

    if (zmq_close(socket) != 0) {
        perror("Failed to close ZeroMQ socket");
    }
    if (zmq_close(subscriber) != 0) {
        perror("Failed to close ZeroMQ socket");
    }
    if (zmq_ctx_destroy(context) != 0) {
        perror("Failed to destroy context");
    }
    endwin();
    return NULL;
}
/**
 * Entry point for the client application.
 *
 * This function initializes and runs the client application
 * by calling the `run_client` function. After execution,
 * it prints a message indicating the client has finished.
 *
 * @return Returns 0 upon successful completion.
 */
int main() {
    context = zmq_ctx_new();
    if (!context) {
        perror("Failed to create ZeroMQ context");
        return 1;
    }

    pthread_t display_thread_id, client_thread_id;

    if (pthread_create(&client_thread_id, NULL, run_client, NULL) != 0) {
        perror("Failed to create client thread");
        zmq_ctx_destroy(context);
        return 1;
    }

    if (pthread_create(&display_thread_id, NULL, display_game_state, NULL) != 0) {
        perror("Failed to create display thread");
        quit_flag = 1;  // Signal the client thread to quit
        pthread_join(client_thread_id, NULL);  // Wait for the client thread to finish
        zmq_ctx_destroy(context);
        return 1;
    }

    if (pthread_join(client_thread_id, NULL) != 0) {
        perror("Failed to join client thread");
    }

    if (pthread_join(display_thread_id, NULL) != 0) {
        perror("Failed to join display thread");
    }

    endwin();  // Cleanup ncurses
    return 0;
}

