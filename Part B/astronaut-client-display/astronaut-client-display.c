
#include "common.h"

void *display_game_state() {

    subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, PUBLISHER_ADDRESS);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_UPDATE, strlen(MSG_UPDATE));
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER));		

    // Initialize ncurses windows
    initscr();
    noecho();
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2);
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);

    // Set up line and column markers
    for (int i = 0; i < BOARD_SIZE; i++) {
        mvwprintw(line_win, i, 0, "%d", i % 10);
        mvwprintw(column_win, 0, i, "%d", i % 10);
    }
    wrefresh(line_win);
    wrefresh(column_win);

    box(board_win, 0, 0);
    box(score_win, 0, 0);
    mvwprintw(score_win, 1, 3, "%s", "SCORE");
    wrefresh(score_win);
    wrefresh(board_win);

    GameState gameState;
    // memset(&gameState, 0, sizeof(gameState));
    // memset(&astronaut_ids_in_use, 0, sizeof(astronaut_ids_in_use));
    
    char topic[strlen(MSG_UPDATE)];
    while (1) {
    // memset(&gameState, 0, sizeof(gameState));
    // memset(&astronaut_ids_in_use, 0, sizeof(astronaut_ids_in_use));
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
        zmq_recv(subscriber, &gameState, sizeof(GameState), 0);
        zmq_recv(subscriber, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), 0);
        

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
    }

    // Cleanup ncurses windows and ZMQ resources
    delwin(line_win);
    delwin(column_win);
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

    void *publisher = zmq_socket(context, ZMQ_PUB);

    // Connect to the server
    zmq_send(socket, MSG_CONNECT, strlen(MSG_CONNECT), 0);

    char response[65];
    int bytes = zmq_recv(socket, response, sizeof(response), 0);
    response[bytes] = '\0';

    sscanf(response, "Welcome! You are player %c %s", &astronaut_id, token);
    mvprintw(BOARD_SIZE + 5, 0, "Welcome! You are player %c", astronaut_id);
	mvprintw(BOARD_SIZE + 6, 0, "- - - - - - - - - - - - - - - - -");
	refresh();

    while (1) {
        int ch = getch();

		// Prepare the movement message based on key press
		char message[38];
		if (ch == KEY_UP) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'U', token);
		else if (ch == KEY_DOWN) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'D', token);
		else if (ch == KEY_LEFT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'L', token);
		else if (ch == KEY_RIGHT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'R', token);
		else if (ch == ' ') sprintf(message, "%s %c %s", MSG_ZAP, astronaut_id, token);
		else if (ch == 'q' || ch == 'Q') sprintf(message, "%s %c %s", MSG_DISCONNECT, astronaut_id, token); 
		else continue;  // Skip unrecognized keys

        zmq_send(socket, message, strlen(message), 0);
        bytes = zmq_recv(socket, response, sizeof(response), 0);
        response[bytes] = '\0';

		mvprintw(BOARD_SIZE + 7, 0, "%s", response);
		clrtoeol();
        refresh();

        if (strcmp(response, "Disconnected") == 0) { 
			mvprintw(BOARD_SIZE + 9, 0, "Thanks for playing! See you soon!");
			refresh();
            break;
        }
    }

    sleep(5);
    endwin();
    zmq_close(socket);
	zmq_close(subscriber);
	zmq_ctx_destroy(context);
	exit(0);
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

    zmq_ctx_destroy(context);

    return 0;
}