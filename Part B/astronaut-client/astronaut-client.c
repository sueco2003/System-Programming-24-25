#include "common.h"
#include <assert.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *listen_thread() {
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    int rc = zmq_connect(subscriber, PUBLISHER_ADDRESS);
	assert(rc == 0);
    rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_SERVER, strlen(MSG_SERVER));
	assert(rc == 0);
	rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, MSG_THREAD, strlen(MSG_THREAD));
	assert(rc == 0);


    while (1) {

        char topic[strlen(MSG_SERVER)];
        zmq_recv(subscriber, topic, sizeof(topic), 0);
		mvprintw(7,0,"recebeste\n");
        if (strncmp(topic, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
            // Inject a termination signal into the input queue
            ungetch('\r');
			mvprintw(8,0,"recebeste\n");
            break;
        } else if (strncmp(topic, MSG_THREAD, strlen(MSG_THREAD)) == 0) {
			char id_received, token_received[6];
			zmq_recv(subscriber, &id_received, sizeof(id_received), 0);
			zmq_recv(subscriber, token_received, sizeof(token_received), 0);
			if (id_received == astronaut_id && strcmp(token_received, token) == 0) {
				break;
			}
		}
    }
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
	// Initialize ncurses for user input/output

	initscr();
	keypad(stdscr, TRUE);
	noecho();

	void *socket = zmq_socket(context, ZMQ_REQ);
	zmq_connect(socket, SERVER_ADDRESS);

	void *publisher = zmq_socket(context, ZMQ_PUB);

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

	while (1) {
		int ch = getch();

		// Prepare the movement message based on key press
		char message[38];
		if (ch == KEY_UP) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'U', token);
		else if (ch == KEY_DOWN) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'D', token);
		else if (ch == KEY_LEFT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'L', token);
		else if (ch == KEY_RIGHT) sprintf(message, "%s %c %c %s", MSG_MOVE, astronaut_id, 'R', token);
		else if (ch == ' ') sprintf(message, "%s %c %s", MSG_ZAP, astronaut_id, token);
		else if (ch == 'q' || ch == 'Q') {
			zmq_send(publisher, MSG_THREAD, strlen(MSG_THREAD), ZMQ_SNDMORE);
            zmq_send(publisher, &astronaut_id, sizeof(astronaut_id), ZMQ_SNDMORE);
            zmq_send(publisher, token, sizeof(token), 0);    
			sprintf(message, "%s %c %s", MSG_DISCONNECT, astronaut_id, token);
		}
		else if (ch == '\r') {
			break;
		}
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

    pthread_t client_thread_id, listen_thread_id;
	pthread_create(&client_thread_id, NULL, run_client, NULL);
    pthread_create(&listen_thread_id, NULL, listen_thread, NULL);
	
    pthread_join(client_thread_id, NULL);
    pthread_join(listen_thread_id, NULL);

    zmq_ctx_destroy(context);

	return 0;
}