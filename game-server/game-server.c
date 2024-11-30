#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"

typedef struct {
    char id;        // Astronaut letter
    int x, y;       // Position
    int score;      // Current score
    int stunned;    // Stunned timer
} Astronaut;

typedef struct {
    int x, y;       // Alien position
    int alive;      // 1 = alive, 0 = dead
} Alien;

Astronaut astronauts[MAX_PLAYERS];
Alien aliens[MAX_ALIENS];
char board[BOARD_SIZE][BOARD_SIZE];
int astronaut_count = 0;
int alien_count = MAX_ALIENS;

void init_game_state() {
    memset(board, ' ', sizeof(board));

    // Initialize aliens in the orange area
    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].x = rand() % 14 + 3; // Range 3–16
        aliens[i].y = rand() % 14 + 3;
        aliens[i].alive = 1;
    }
}

void update_aliens() {
    for (int i = 0; i < alien_count; i++) {
        if (!aliens[i].alive) continue;
        int dx = (rand() % 3) - 1; // Random -1, 0, or 1
        int dy = (rand() % 3) - 1;
        aliens[i].x = (aliens[i].x + dx + BOARD_SIZE) % BOARD_SIZE;
        aliens[i].y = (aliens[i].y + dy + BOARD_SIZE) % BOARD_SIZE;
    }
}

void update_board() {
    memset(board, ' ', sizeof(board));

    // Add aliens
    for (int i = 0; i < alien_count; i++) {
        if (aliens[i].alive) board[aliens[i].x][aliens[i].y] = '*';
    }

    // Add astronauts
    for (int i = 0; i < astronaut_count; i++) {
        if (astronauts[i].stunned <= 0) {
            board[astronauts[i].x][astronauts[i].y] = astronauts[i].id;
        }
    }
}

void process_message(void *socket, char *message) {
    printf("fora\n");
    if (strncmp(message, MSG_CONNECT, strlen(MSG_CONNECT)) == 0) {
printf("dentro\n");
        if (astronaut_count >= MAX_PLAYERS) {
            zmq_send(socket, "Server full", 11, 0);
            return;
        }

        // Add new astronaut
        char id = 'A' + astronaut_count;
        astronauts[astronaut_count] = (Astronaut){id, astronaut_count, 0, 0, 0};
        astronaut_count++;

        char response[16];
        sprintf(response, "Connected %c", id);
        zmq_send(socket, response, strlen(response), 0);
        printf("enviei %s\n", response);
    } else if (strncmp(message, MSG_MOVE, strlen(MSG_MOVE)) == 0) {
        // Handle movement...
        zmq_send(socket, "Move processed", 15, 0);
    } else if (strncmp(message, MSG_ZAP, strlen(MSG_ZAP)) == 0) {
        // Handle zapping...
        zmq_send(socket, "Zap processed", 14, 0);
    }
}

void broadcast_game_state(void *publisher) {
    char serialized_board[BOARD_SIZE * BOARD_SIZE + 1];
    int index = 0;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            serialized_board[index++] = board[i][j];
        }
    }
    serialized_board[index] = '\0';

    zmq_send(publisher, serialized_board, strlen(serialized_board), 0);
}

void game_loop(void *socket, void *publisher) {
    init_game_state();

    char message[256];
    while (1) {
        zmq_recv(socket, message, sizeof(message), 0);
        printf("RECEBEU %s\n", message);
        process_message(socket, message);

        update_aliens();
        update_board();
        broadcast_game_state(publisher);
        usleep(500000); // Slow down game loop
    }
}

int main() {
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP);  // Using REP socket type
    zmq_bind(socket, SERVER_ADDRESS);

    void *publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, PUBLISHER_ADDRESS);

    game_loop(socket, publisher);

    zmq_close(socket);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 0;
}
