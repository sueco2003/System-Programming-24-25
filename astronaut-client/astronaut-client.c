#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include <stdio.h>
#include <assert.h>

void run_client() {
    // Initialize ncurses for user input/output
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    // Initialize ZeroMQ context and socket
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REQ);
    zmq_connect(socket, SERVER_ADDRESS);

    // Connect to the server
    zmq_send(socket, MSG_CONNECT, strlen(MSG_CONNECT), 0);
    printf("mandei %s\n", MSG_CONNECT);
    
    // Receive response from the server and extract astronaut ID
    char response[16];
    zmq_recv(socket, response, sizeof(response), 0);
    printf("Received: %s\n", response);  // Debugging
    mvprintw(1, 0, "Server: %s", response);  // Display the response
    char astronaut_id = response[10];  // Extract the astronaut ID (e.g., "Connected X")

    mvprintw(0, 0, "You are astronaut %c", astronaut_id);  // Display astronaut ID
    refresh();

    while (1) {
        int ch = getch();
        
        if (ch == 'q' || ch == 'Q') break;  // Exit on 'q' or 'Q'

        // Prepare the movement message based on key press
        char message[16];
        if (ch == KEY_UP) {
            strcpy(message, "MOVE UP");
        } else if (ch == KEY_DOWN) {
            strcpy(message, "MOVE DOWN");
        } else if (ch == KEY_LEFT) {
            strcpy(message, "MOVE LEFT");
        } else if (ch == KEY_RIGHT) {
            strcpy(message, "MOVE RIGHT");
        } else if (ch == ' ') {
            strcpy(message, "ZAP");
        } else {
            continue; // Skip unrecognized keys
        }

        // Send the message to the server and wait for a response
        zmq_send(socket, message, strlen(message), 0);
        zmq_recv(socket, response, sizeof(response), 0);

        // Display server's response on the screen
        mvprintw(1, 0, "Server: %s", response);
        refresh();
    }

    // Send disconnect message to server before closing
    zmq_send(socket, MSG_DISCONNECT, strlen(MSG_DISCONNECT), 0);
    
    // Clean up and close ZeroMQ socket and context
    zmq_close(socket);
    zmq_ctx_destroy(context);
    
    // End ncurses mode
    endwin();
}

int main() {
    run_client();  // Run the client application
    printf("Client finished\n");
    printf("Client finished\n");
    return 0;


}