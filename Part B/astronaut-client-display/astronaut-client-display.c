#include "common.h"

volatile int quit_flag = 0;  // Global flag to signal quit
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
    zmq_connect(subscriber, PUBLISHER_ADDRESS);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_UPDATE, strlen(MSG_UPDATE));
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER));		

    // Initialize ncurses windows
    initscr();
    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);    // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);  // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2);  // Window for the board with a border
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);

    // Set up line and column markers
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
        
        // Receive game state updates
        zmq_recv(subscriber, topic, sizeof(topic), 0);
        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            zmq_close(socket);
			endwin();
            zmq_close(subscriber);
	        zmq_ctx_destroy(context);
	        exit(0);
        }

        else if (strncmp(topic, MSG_UPDATE, strlen(MSG_UPDATE)) != 0) continue;
        zmq_recv(subscriber, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), 0);
        zmq_recv(subscriber, &gameState, sizeof(GameState), 0);
        

        // Update board and score
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
    sleep(2);  // Pause for 2 seconds before closing
    // Cleanup ncurses windows and ZMQ resources
   
    delwin(board_win);
    delwin(score_win);
    endwin();

    zmq_close(subscriber);
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
    keypad(stdscr, TRUE);
    noecho();

    socket = zmq_socket(context, ZMQ_REQ);
    zmq_connect(socket, SERVER_ADDRESS);

    zmq_send(socket, MSG_CONNECT, strlen(MSG_CONNECT), 0);

    char response[65];
    int bytes = zmq_recv(socket, response, sizeof(response), 0);
    response[bytes] = '\0';

    sscanf(response, "Welcome! You are player %c %s", &astronaut_id, token);
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

        zmq_send(socket, message, strlen(message), 0);
        bytes = zmq_recv(socket, response, sizeof(response), 0);
        response[bytes] = '\0';

        mvprintw(BOARD_SIZE + 7, 0, "%s", response);
        clrtoeol();
        refresh();
    }

    mvprintw(BOARD_SIZE + 9, 0, "Thanks for playing! See you soon!");
    refresh();
    sleep(2);  // Pause for 2 seconds before closing
    zmq_close(socket);
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
    pthread_t display_thread_id, client_thread_id;

    pthread_create(&client_thread_id, NULL, run_client, NULL);
    pthread_create(&display_thread_id, NULL, display_game_state, NULL);

    pthread_join(client_thread_id, NULL);
    pthread_join(display_thread_id, NULL);

    endwin();  // Cleanup for ncurses
    zmq_ctx_destroy(context);
    return 0;
}
