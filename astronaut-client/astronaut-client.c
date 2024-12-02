#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <zmq.h>
#include <stdio.h>
#include <assert.h>

void run_client()
{
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

    // Receive response from the server and extract astronaut ID
    char response[64];
    int bytes_received = zmq_recv(socket, response, sizeof(response), 0);
    response[bytes_received] = '\0';
    mvprintw(1, 0, "%s", response);   // Display the response
    mvprintw(2, 0, "- - - - - - - - - - - - - - - - -");   // Display the response
    char astronaut_id = response[24]; // Extract the astronaut ID (e.g., "Connected X")
    refresh();

    while (1)
    {

        int ch = getch();

        // Prepare the movement message based on key press
        char message[32];
        if (ch == KEY_UP)
        {
            sprintf(message, "%s %c %s", MSG_MOVE, astronaut_id, "UP");
        }
        else if (ch == KEY_DOWN)
        {
            sprintf(message, "%s %c %s", MSG_MOVE, astronaut_id, "DOWN");
        }
        else if (ch == KEY_LEFT)
        {
            sprintf(message, "%s %c %s", MSG_MOVE, astronaut_id, "LEFT");
        }
        else if (ch == KEY_RIGHT)
        {
            sprintf(message, "%s %c %s", MSG_MOVE, astronaut_id, "RIGHT") ;
        }
        else if (ch == ' ')
        {
            sprintf(message, "%s %c", MSG_ZAP, astronaut_id);
        }
        else if (ch == 'q' || ch == 'Q')
        {
            sprintf(message, "%s %c", MSG_DISCONNECT, astronaut_id);
        }
        else
        {
            continue; // Skip unrecognized keys
        }

        // Send the message to the server and wait for a response
        zmq_send(socket, message, strlen(message), 0);
        int bytes_received = zmq_recv(socket, response, sizeof(response), 0);
        response[bytes_received] = '\0';
        move(3, 0);          // move to begining of line
        clrtoeol(); 
        mvprintw(3, 0, "%s", response);
        refresh();
        if (!strcmp(response, "Disconnected"))  {  
            mvprintw(5, 0, "Thanks for playing! See you soon!");
            refresh(); 
            break;
        }
    }

    // Clean up and close ZeroMQ socket and context
    zmq_close(socket);
    zmq_ctx_destroy(context);
    usleep(3000000);
    // End ncurses mode
    endwin();
}

int main()
{
    run_client(); // Run the client application
    printf("Client finished\n");

    return 0;
}