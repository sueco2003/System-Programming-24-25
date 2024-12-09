#ifndef COMMON_H
#define COMMON_H

#include <curses.h>  // for mvprintw, refresh, clrtoeol, endwin, initscr
#include <stdio.h>   // for sprintf, printf
#include <string.h>  // for strlen, strcmp
#include <unistd.h>  // for sleep
#include <zmq.h>     // for zmq_recv, zmq_send, zmq_close, zmq_connect, zmq_...

#define SERVER_ADDRESS "tcp://127.0.0.1:5551"
#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5552"


#define MSG_CONNECT "Astronaut_connect"
#define MSG_DISCONNECT "Astronaut_disconnect"
#define MSG_MOVE "Astronaut_movement"
#define MSG_ZAP "Astronaut_zap"

#endif
