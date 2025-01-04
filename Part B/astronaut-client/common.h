#ifndef COMMON_H
#define COMMON_H

#include <curses.h>	 // for mvprintw, refresh, clrtoeol, endwin, initscr
#include <stdio.h>	 // for sprintf, printf
#include <string.h>	 // for strlen, strcmp
#include <string.h>
#include <unistd.h>	 // for sleep
#include <zmq.h>	 // for zmq_recv, zmq_send, zmq_close, zmq_connect, zmq_...
#include <pthread.h> // for pthread_create, pthread_join, pthread_mutex_lock, p...
#include <stdlib.h>	  // for rand, exit, EXIT_FAILURE, EXIT_SUCCESS

#define SERVER_ADDRESS "tcp://127.0.0.1:5533"
#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5554"

#define MSG_CONNECT "Astronaut_connect"
#define MSG_DISCONNECT "Astronaut_disconnect"
#define MSG_MOVE "Astronaut_movement"
#define MSG_ZAP "Astronaut_zap"

#define MSG_SERVER "Server_terminate"
#define MSG_THREAD "Thread_terminate"

void *context, *socket, *subscriber;

int pipefd[2];

char astronaut_id;
char token[6];

#endif
