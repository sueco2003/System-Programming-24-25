#include <curses.h>  // for mvprintw, refresh, clrtoeol, endwin, initscr
#include <stdio.h>   // for sprintf, printf
#include <string.h>  // for strlen, strcmp
#include <unistd.h>  // for sleep
#include <zmq.h>     // for zmq_recv, zmq_send, zmq_close, zmq_connect, zmq_...

#define SERVER_ADDRESS "tcp://127.0.0.1:5543"
#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5544"


#define MSG_CONNECT "Astronaut_connect"
#define MSG_DISCONNECT "Astronaut_disconnect"
#define MSG_MOVE "Astronaut_movement"
#define MSG_ZAP "Astronaut_zap"


/**
 * Runs the client application, handling user input and server communication.
 *
 * This function initializes the ncurses library for user input and output,
 * sets up a ZeroMQ context and socket to communicate with the server, and
 * processes user commands to send movement or action messages. It displays
 * server responses and handles disconnection gracefully.
 */
void run_client() {
    // Initialize ncurses for user input/output

    char token[6];
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    // Initialize ZeroMQ context and socket
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REQ);
    zmq_connect(socket, SERVER_ADDRESS);

    // Receive response from the server and extract astronaut ID
    char response[65];  
    int bytes ;
   
    char astronaut_id = 'A';
    refresh();

    // Send the message to the server and wait for a response
        zmq_send(socket, "ola", 4, 0);
        bytes = zmq_recv(socket, response, sizeof(response), 0);
        response[bytes] = '\0';
        move(3, 0);          // move to begining of line
        clrtoeol(); 
        move(4, 0);          // move to begining of line
        clrtoeol(); 
        refresh();
        mvprintw(3, 0, "%s", response);
        refresh();

    while (1) {

        int ch = getch();

        // Prepare the movement message based on key press
        char message[38];
        if (ch == KEY_UP) sprintf(message, "%s %c %c", MSG_MOVE, astronaut_id, 'U');
        else if (ch == KEY_DOWN)sprintf(message, "%s %c %c ", MSG_MOVE, astronaut_id, 'D');
        else if (ch == KEY_LEFT) sprintf(message, "%s %c %c ", MSG_MOVE, astronaut_id, 'L' );
        else if (ch == KEY_RIGHT) sprintf(message, "%s %c %c", MSG_MOVE, astronaut_id, 'R'  );
        else if (ch == ' ') sprintf(message, "%s %c UKQYAG", MSG_ZAP, astronaut_id );
        else if (ch == 'q' || ch == 'Q') sprintf(message, "%s %c ", MSG_DISCONNECT, astronaut_id);
        else continue;

        // Send the message to the server and wait for a response
        zmq_send(socket, message, strlen(message), 0);
        bytes = zmq_recv(socket, response, sizeof(response), 0);
        response[bytes] = '\0';
        move(3, 0);          // move to begining of line
        clrtoeol(); 
        move(4, 0);          // move to begining of line
        clrtoeol(); 
        refresh();
        mvprintw(3, 0, "%s", response);
        refresh();

        if (strcmp(response, "Disconnected") == 0)  {  
            mvprintw(5, 0, "Thanks for playing! See you soon!");
            refresh(); 
            break;
        }
    }


    sleep(5);
    endwin();

    // Clean up and close ZeroMQ socket and context
    zmq_close(socket);
    zmq_ctx_destroy(context);
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
int main()
{
    run_client(); // Run the client application
    printf("Client finished\n");

    return 0;
}