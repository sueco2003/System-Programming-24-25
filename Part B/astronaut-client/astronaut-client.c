#include "common.h"
#include <assert.h>

int quit_flag = 0;
/**
 * Listens for messages from a ZeroMQ publisher in a separate thread.
 *
 * This function creates a ZeroMQ subscriber socket, connects it to a
 * specified publisher address, and subscribes to a specific message
 * topic. It continuously listens for messages, handling errors and
 * cleaning up resources such as the ZeroMQ socket and context, and
 * the ncurses environment upon receiving a termination signal.
 *
 * @return Always returns NULL after thread exit.
 */
void *listen_thread() {
    subscriber = zmq_socket(context, ZMQ_SUB);
    if (!subscriber) {
        perror("Failed to create ZeroMQ subscriber socket");
        return NULL;
    }

    if (zmq_connect(subscriber, PUBLISHER_ADDRESS) != 0) {
        perror("Failed to connect ZeroMQ subscriber to publisher");
        zmq_close(subscriber);
        return NULL;
    }

    if (zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER)) != 0) {
        perror("Failed to set ZeroMQ subscription for MSG_SERVER");
        zmq_close(subscriber);
        return NULL;
    }

    while (1) {
        char topic[strlen(MSG_SERVER)];
        if (zmq_recv(subscriber, topic, sizeof(topic), 0) == -1) {
            break;
        }

        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            zmq_close(subscriber);
            zmq_close(socket);
            zmq_ctx_destroy(context);
            endwin(); // Ensure ncurses is cleaned up
            exit(0);
            break;
        }
    }

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
	// Initialize ncurses for user input/output

	initscr();
	keypad(stdscr, TRUE);
	noecho();
	socket = zmq_socket(context, ZMQ_REQ);
	int rc = zmq_connect(socket, SERVER_ADDRESS);
	assert(rc == 0);
	// Connect to the server
	zmq_send(socket, MSG_CONNECT, strlen(MSG_CONNECT), 0);

	// Receive response from the server and extract astronaut ID
	char response[65];
	int bytes = zmq_recv(socket, response, sizeof(response), 0);
	response[bytes] = '\0';

	sscanf(response, "Welcome! You are player %c %s", &astronaut_id, token);
	mvprintw(1, 0, "Welcome! You are player %c", astronaut_id);	 // Display the response
	mvprintw(2, 0, "- - - - - - - - - - - - - - - - -");	// Display the response
	refresh();

	while (quit_flag == 0) {
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

		// Send the message to the server and wait for a response
		zmq_send(socket, message, strlen(message), 0);
		bytes = zmq_recv(socket, response, sizeof(response), 0);
		response[bytes] = '\0';
		move(3, 0);	 // move to begining of line
		clrtoeol();
		move(4, 0);	 // move to begining of line
		clrtoeol();
		refresh();
		mvprintw(3, 0, "%s", response);
		refresh();

		if (strcmp(response, "Disconnected") == 0) {
			mvprintw(5, 0, "Thanks for playing! See you soon!");
			refresh();
			break;
		}
	}
	sleep(5);
	endwin();

	// Clean up and close ZeroMQ socket and context
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
    if (!context) {
        fprintf(stderr, "Error creating ZeroMQ context: %s\n", zmq_strerror(errno));
        return 1;
    }

    pthread_t client_thread_id, listen_thread_id;

    if (pthread_create(&client_thread_id, NULL, run_client, NULL) != 0) {
        fprintf(stderr, "Error creating client thread.\n");
        zmq_ctx_destroy(context);
        return 1;
    }

    if (pthread_create(&listen_thread_id, NULL, listen_thread, NULL) != 0) {
        fprintf(stderr, "Error creating listener thread.\n");
        quit_flag = 1;
		pthread_join(client_thread_id, NULL);
        zmq_ctx_destroy(context);
        return 1;
    }

    if (pthread_join(client_thread_id, NULL) != 0) {
        perror("Failed to join client thread");
    }

    if (pthread_join(listen_thread_id, NULL) != 0) {
        perror("Failed to join listen thread");
    }

    endwin();  // Cleanup ncurses
    return 0;
}
