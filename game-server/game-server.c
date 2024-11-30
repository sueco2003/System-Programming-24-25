#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"

typedef struct
{
    char id;     // Astronaut letter
    int x, y;    // Position
    int score;   // Current score
    int stunned; // Stunned timer
} Astronaut;

typedef struct
{
    int x, y; // Alien position
} Alien;

Astronaut astronauts[MAX_PLAYERS];
Alien aliens[MAX_ALIENS];
char board[BOARD_SIZE][BOARD_SIZE];
int astronaut_count = 0;
int alien_count = MAX_ALIENS;

void render_board()
{
    clear();

    WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1);               // Window for line numbers
    WINDOW *column_win = newwin(1, BOARD_SIZE + 2, 1, 3);             // Window for column numbers
    WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 2); // Window for the board with a border
    WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);
    

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        mvwprintw(line_win, i, 0, "%d", i % 10);
        mvwprintw(column_win, 0, i, "%d", i % 10);
    }
    mvwprintw(score_win, 1, 3, "%s", "SCORE");

    box(board_win, 0, 0);
    box(score_win, 0, 0);
    wrefresh(column_win);
    wrefresh(line_win);
    wrefresh(score_win);

    // Print the board
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            mvwaddch(board_win, i + 1, j + 1, board[i][j]); // Adjust position for border within the window
        }
    }
    wrefresh(board_win);

    for (int i = 0; i < astronaut_count; i++)
    {
        mvwprintw(score_win, 2 + i, 3, "%c - %d", astronauts[i].id, astronauts[i].score);
    }
    wrefresh(score_win);

}

void init_game_state()
{
    memset(board, ' ', sizeof(board));

    // Initialize aliens in the restricted area (between index 2 and 17)
    for (int i = 0; i < MAX_ALIENS; i++)
    {
        aliens[i].x = rand() % 16 + 2; // Range 2–17
        aliens[i].y = rand() % 16 + 2; // Range 2–17
    }
}

void update_aliens()
{
    for (int i = 0; i < alien_count; i++)
    {
        // Random movement within the range of 2 to 17
        int dx = (rand() % 3) - 1; // Random -1, 0, or 1 for X movement
        int dy = (rand() % 3) - 1; // Random -1, 0, or 1 for Y movement

        // Update alien position while keeping it within the range 2–17
        aliens[i].x = (aliens[i].x + dx + BOARD_SIZE) % BOARD_SIZE;
        aliens[i].y = (aliens[i].y + dy + BOARD_SIZE) % BOARD_SIZE;

        // Ensure aliens are within the restricted area (2–17)
        if (aliens[i].x < 2)
            aliens[i].x = 2;
        if (aliens[i].x > 17)
            aliens[i].x = 17;
        if (aliens[i].y < 2)
            aliens[i].y = 2;
        if (aliens[i].y > 17)
            aliens[i].y = 17;
    }
}

void remove_alien(int index)
{
    for (int i = index; i < alien_count - 1; i++)
    {
        aliens[i] = aliens[i + 1];
    }
    alien_count--;
}

void broadcast_game_state(void *publisher)
{
    char serialized_board[BOARD_SIZE * BOARD_SIZE + 1];
    int index = 0;

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            serialized_board[index++] = board[i][j];
        }
    }
    serialized_board[index] = '\0';

    zmq_send(publisher, serialized_board, strlen(serialized_board), 0);
}

void update_board()
{
    memset(board, ' ', sizeof(board));

    // Add remaining aliens
    for (int i = 0; i < alien_count; i++)
    {
        board[aliens[i].x][aliens[i].y] = '*';
    }

    // Add astronauts
    for (int i = 0; i < astronaut_count; i++)
    {
        if (astronauts[i].stunned <= 0)
        {
            board[astronauts[i].x][astronauts[i].y] = astronauts[i].id;
        }
    }
}

void process_message(void *socket, char *message)
{
    if (strncmp(message, MSG_CONNECT, strlen(MSG_CONNECT)) == 0)
    {
        if (astronaut_count >= MAX_PLAYERS)
        {
            zmq_send(socket, "Server full", 11, 0);
            return;
        }

        int x = rand() % 16 + 18; // Ensure astronauts spawn outside alien space
        int y = rand() % 16 + 18; // Ensure astronauts spawn outside alien space

        // Add new astronaut
        char id = 'A' + astronaut_count;
        astronauts[astronaut_count] = (Astronaut){id, astronaut_count, x, y, 0};
        astronaut_count++;

        char response[16];
        sprintf(response, "Connected %c", id);
        zmq_send(socket, response, strlen(response), 0);
    }
    else if (strncmp(message, MSG_DISCONNECT, strlen(MSG_DISCONNECT)) == 0)
    {
        char id = message[strlen(MSG_DISCONNECT) + 1];
        int found = 0;
        for (int i = 0; i < astronaut_count; i++)
        {
            if (astronauts[i].id == id)
            {
                for (int j = i; j < astronaut_count - 1; j++)
                {
                    astronauts[j] = astronauts[j + 1];
                }
                astronaut_count--;
                found = 1;
                break;
            }
        }
        zmq_send(socket, found ? "Disconnected" : "Astronaut not found", 20, 0);
    }
    else if (strncmp(message, MSG_MOVE, strlen(MSG_MOVE)) == 0)
    {
        char id = message[strlen(MSG_MOVE) + 1];
        char direction = message[strlen(MSG_MOVE) + 3]; // Assuming "MOVE X D" (X = ID, D = direction)

        for (int i = 0; i < astronaut_count; i++)
        {
            if (astronauts[i].id == id)
            {
                if (direction == 'U')
                    astronauts[i].x = (astronauts[i].x - 1 + BOARD_SIZE) % BOARD_SIZE;
                if (direction == 'D')
                    astronauts[i].x = (astronauts[i].x + 1) % BOARD_SIZE;
                if (direction == 'L')
                    astronauts[i].y = (astronauts[i].y - 1 + BOARD_SIZE) % BOARD_SIZE;
                if (direction == 'R')
                    astronauts[i].y = (astronauts[i].y + 1) % BOARD_SIZE;
                break;
            }
        }
        zmq_send(socket, "Move processed", 15, 0);
    }
    else if (strncmp(message, MSG_ZAP, strlen(MSG_ZAP)) == 0)
    {
        char id = message[strlen(MSG_ZAP) + 1];

        for (int i = 0; i < astronaut_count; i++)
        {
            if (astronauts[i].id == id)
            {
                int x = astronauts[i].x, y = astronauts[i].y;

                // Simulate laser shot upwards
                for (int j = x - 1; j >= 0; j--)
                { // Laser moves up
                    if (board[j][y] == '*')
                    { // Alien is hit
                        for (int k = 0; k < alien_count; k++)
                        {
                            if (aliens[k].x == j && aliens[k].y == y)
                            {
                                remove_alien(k);
                                break;
                            }
                        }
                        astronauts[i].score += 10;
                        break;
                    }
                    else
                    {
                        board[j][y] = '|'; // Show laser
                        render_board();
                        usleep(50000);     // Show for 50ms
                        board[j][y] = ' '; // Clear laser
                    }
                }
                break;
            }
        }
        zmq_send(socket, "Zap processed", 14, 0);
    }
}

void game_loop(void *socket, void *publisher)
{
    initscr();       // Initialize ncurses
    noecho();        // Don't echo input
    curs_set(FALSE); // Hide cursor
    init_game_state();
    char message[256];
    while (1)
    {
        zmq_recv(socket, message, sizeof(message), 0);
        mvprintw(1, 0, "recebeste algo? %s\n", message);
        process_message(socket, message);

        update_aliens();
        update_board();
        render_board();
        broadcast_game_state(publisher);
        usleep(500000); // Slow down game loop
    }

    endwin(); // End ncurses
}

int main()
{
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP); // Using REP socket type
    zmq_bind(socket, SERVER_ADDRESS);
    printf("ei\n");
    void *publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, PUBLISHER_ADDRESS);
    printf("ei\n");
    game_loop(socket, publisher);

    zmq_close(socket);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 0;
}