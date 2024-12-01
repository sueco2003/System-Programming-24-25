#ifndef COMMON_H
#define COMMON_H

#define SERVER_ADDRESS "tcp://127.0.0.1:5555" // VER ESTES IPS O QUE E PARA POR AQUI
#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5556"

#define BOARD_SIZE 20
#define MAX_PLAYERS 8
#define MAX_ALIENS 1 // 1/3 of the board

// Message types
#define MSG_CONNECT "Astronaut_connect"
#define MSG_DISCONNECT "Astronaut_disconnect"
#define MSG_MOVE "Astronaut_movement"
#define MSG_ZAP "Astronaut_zap"

// Directions
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#endif
