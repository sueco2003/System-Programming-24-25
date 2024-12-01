#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

int Y_MAX[] = {0, 1, 18, 19, 17, 17, 17, 17};
int Y_MIN[] = {0, 1, 18, 19, 2, 2, 2, 2};
int X_MAX[] = {17, 17, 17, 17, 0, 1, 18, 19};
int X_MIN[] = {2, 2, 2, 2, 0, 1, 18, 19};

// Define the structure for an astronaut
typedef struct {
    char id;            // Astronaut ID (A, B, C, etc.)
    int x, y;           // Position on the board
    int score;          // Score of the astronaut
    time_t stunned_time; // Time when astronaut was stunned
    time_t last_shot_time; // Time when astronaut last shot
} Astronaut;

#define STUN_DURATION 10  // Stun duration in seconds
#define SHOOT_COOLDOWN 3  // Time required between shots in seconds

// Function to get the current time in seconds
time_t current_time() {
    return time(NULL);
}

typedef struct
{
    int x, y; // Alien position
} Alien;

Astronaut astronauts[MAX_PLAYERS];
Alien aliens[MAX_ALIENS];
char board[BOARD_SIZE][BOARD_SIZE];
int astronaut_count = 0;
int alien_count = MAX_ALIENS;
int region_counts[] = {0, 0, 0, 0};

void render_board()
{
    // Initialize ncurses for user input/output
    initscr();
    noecho();
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
            board[astronauts[i].x][astronauts[i].y] = astronauts[i].id;
    }
}

void process_message(void *socket, char *message)
{
    if (strncmp(message, MSG_CONNECT, strlen(MSG_CONNECT)) == 0)
    {
        if (astronaut_count >= MAX_PLAYERS)
        {
            zmq_send(socket, "Sorry, the game is full", 23, 0);
            return;
        }

        int x = X_MIN[astronaut_count] + (rand() % (X_MAX[astronaut_count] - X_MIN[astronaut_count] + 1));
        int y = Y_MIN[astronaut_count] + (rand() % (Y_MAX[astronaut_count] - Y_MIN[astronaut_count] + 1));
        refresh();
        mvprintw(25, 25, "%d %d", x, y);
        refresh();
        // Add new astronaut
        char id = 'A' + astronaut_count;
        astronauts[astronaut_count] = (Astronaut){id, x, y, 0, 0};
        astronaut_count++;

        // Send confirmation response
        char response[27];
        sprintf(response, "Welcome! You are player %c.", id);
        response[strlen(response)] = '\0';
        zmq_send(socket, response, strlen(response), 0); // Send the message
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
                time_t now = current_time();
                // Check if the astronaut is stunned
                if (astronauts[i].stunned_time != 0 && (now - astronauts[i].stunned_time) < 10)
                {
                    zmq_send(socket, "You are stunned! Cannot move.", 31, 0);
                    return; // Prevent the astronaut from shooting if stunned
                }
                int x = astronauts[i].x;
                int y = astronauts[i].y;

                if (direction == 'U' && x - 1 >= X_MIN[i])
                {
                    astronauts[i].x--; // Move to top half if within bounds
                }
                else if (direction == 'D' && x + 1 <= X_MAX[i])
                {
                    astronauts[i].x++; // Move to bottom half if within bounds
                }
                else if (direction == 'L' && y - 1 >= Y_MIN[i])
                {
                    astronauts[i].y--; // Move left within bounds
                }
                else if (direction == 'R' && y + 1 <= Y_MAX[i])
                {
                    astronauts[i].y++; // Move right within bounds
                }
                break;
            }
        }

        zmq_send(socket, "Move processed", 15, 0);
    }

    else if (strncmp(message, MSG_ZAP, strlen(MSG_ZAP)) == 0)
    {
        char id = message[strlen(MSG_ZAP) + 1];
        int player = -1;
        for (int i = 0; i < astronaut_count; i++)
        {
            if (astronauts[i].id == id)
            {
                player = i;
                time_t now = current_time();

                // Check if the astronaut is stunned
                if (astronauts[i].stunned_time != 0 && (now - astronauts[i].stunned_time) < 10)
                {
                    zmq_send(socket, "You are stunned! Cannot shoot.", 31, 0);
                    return; // Prevent the astronaut from shooting if stunned
                }

                // Check if enough time has passed since the last shot
                if (now - astronauts[i].last_shot_time < SHOOT_COOLDOWN)
                {
                    zmq_send(socket, "You must wait before shooting again.", 38, 0);
                    return; // Prevent shooting if within cooldown period
                }

                int x = astronauts[i].x, y = astronauts[i].y;

                // Record the time of the shot
                astronauts[i].last_shot_time = now;

                // Determine the direction based on the player number
                if (i == 0 || i == 1) // Players 0 and 1: shoot to the right
                {
                    for (int j = y + 1; j < BOARD_SIZE; j++)
                    {
                        if (board[x][j] == '*') // Alien hit
                        {
                            astronauts[i].score++; // Increase score
                            // Remove alien after hit and clear the shot path
                            for (int k = 0; k < alien_count; k++)
                            {
                                if (aliens[k].x == x && aliens[k].y == j)
                                {
                                    remove_alien(k); // Remove alien
                                    break;
                                }
                            }
                            board[x][j] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the alien
                        }
                        else if (isalnum(board[x][j])) // Astronaut hit
                        {
                            // If the shot hits another astronaut, stun them
                            for (int k = 0; k < astronaut_count; k++)
                            {
                                if (astronauts[k].x == x && astronauts[k].y == j)
                                {
                                    astronauts[k].stunned_time = now; // Set stunned time
                                    break;                            // Stun the astronaut and continue shooting
                                }
                            }
                            board[x][j] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the astronaut
                        }
                        else
                        {
                            board[x][j] = '-'; // Mark the shot with a line (horizontal)
                        }
                    }
                }
                else if (i == 2 || i == 3) // Players 2 and 3: shoot to the left
                {
                    for (int j = y - 1; j >= 0; j--)
                    {
                        if (board[x][j] == '*') // Alien hit
                        {
                            astronauts[i].score++; // Increase score
                            // Remove alien after hit and clear the shot path
                            for (int k = 0; k < alien_count; k++)
                            {
                                if (aliens[k].x == x && aliens[k].y == j)
                                {
                                    remove_alien(k); // Remove alien
                                    break;
                                }
                            }
                            board[x][j] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the alien
                        }
                        else if (isalnum(board[x][j])) // Astronaut hit
                        {
                            // If the shot hits another astronaut, stun them
                            for (int k = 0; k < astronaut_count; k++)
                            {
                                if (astronauts[k].x == x && astronauts[k].y == j)
                                {
                                    astronauts[k].stunned_time = now; // Set stunned time
                                    break;                            // Stun the astronaut and continue shooting
                                }
                            }
                            board[x][j] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the astronaut
                        }
                        else
                        {
                            board[x][j] = '-'; // Mark the shot with a line (horizontal)
                        }
                    }
                }
                else if (i == 4 || i == 5) // Players 4 and 5: shoot downwards
                {
                    for (int j = x + 1; j < BOARD_SIZE; j++)
                    {
                        if (board[j][y] == '*') // Alien hit
                        {
                            astronauts[i].score++; // Increase score
                            // Remove alien after hit and clear the shot path
                            for (int k = 0; k < alien_count; k++)
                            {
                                if (aliens[k].x == j && aliens[k].y == y)
                                {
                                    remove_alien(k); // Remove alien
                                    break;
                                }
                            }
                            board[j][y] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the alien
                        }
                        else if (isalnum(board[j][y])) // Astronaut hit
                        {
                            for (int k = 0; k < astronaut_count; k++)
                            {
                                if (astronauts[k].x == j && astronauts[k].y == y)
                                {
                                    astronauts[k].stunned_time = now; // Set stunned time
                                    break;                            // Stun the astronaut and continue shooting
                                }
                            }
                            board[j][y] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the astronaut
                        }
                        else
                        {
                            board[j][y] = '|'; // Mark the shot with a line (vertical)
                        }
                    }
                }
                else if (i == 6 || i == 7) // Players 6 and 7: shoot upwards
                {
                    for (int j = x - 1; j >= 0; j--)
                    {
                        if (board[j][y] == '*') // Alien hit
                        {
                            astronauts[i].score++; // Increase score
                            // Remove alien after hit and clear the shot path
                            for (int k = 0; k < alien_count; k++)
                            {
                                if (aliens[k].x == j && aliens[k].y == y)
                                {
                                    remove_alien(k); // Remove alien
                                    break;
                                }
                            }
                            board[j][y] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the alien
                        }
                        else if (isalnum(board[j][y])) // Astronaut hit
                        {
                            for (int k = 0; k < astronaut_count; k++)
                            {
                                if (astronauts[k].x == j && astronauts[k].y == y)
                                {
                                    astronauts[k].stunned_time = now; // Set stunned time
                                    break;                            // Stun the astronaut and continue shooting
                                }
                            }
                            board[j][y] = ' '; // Clear the shot path
                            break;             // Stop the shot after hitting the astronaut
                        }
                        else
                        {
                            board[j][y] = '|'; // Mark the shot with a line (vertical)
                        }
                    }
                }

                render_board(); // Render the board after the shot is marked

                usleep(500000); // Show the shot for 500ms

                // Clear the shot path after 500ms
                if (i == 0 || i == 1) // Horizontal shots (right/left)
                {
                    for (int j = y + 1; j < BOARD_SIZE; j++)
                    {
                        if (board[x][j] == '-')
                        {
                            board[x][j] = ' '; // Clear the shot path (horizontal)
                        }
                        else
                        {
                            break; // Stop when hitting an obstacle
                        }
                    }
                }
                else if (i == 4 || i == 5) // Vertical shots (downwards)
                {
                    for (int j = x + 1; j < BOARD_SIZE; j++)
                    {
                        if (board[j][y] == '|')
                        {
                            board[j][y] = ' '; // Clear the shot path (vertical)
                        }
                        else
                        {
                            break; // Stop when hitting an obstacle
                        }
                    }
                }
                else if (i == 2 || i == 3) // Horizontal shots (left)
                {
                    for (int j = y - 1; j >= 0; j--)
                    {
                        if (board[x][j] == '-')
                        {
                            board[x][j] = ' '; // Clear the shot path (horizontal)
                        }
                        else
                        {
                            break; // Stop when hitting an obstacle
                        }
                    }
                }
                else if (i == 6 || i == 7) // Vertical shots (upwards)
                {
                    for (int j = x - 1; j >= 0; j--)
                    {
                        if (board[j][y] == '|')
                        {
                            board[j][y] = ' '; // Clear the shot path (vertical)
                        }
                        else
                        {
                            break; // Stop when hitting an obstacle
                        }
                    }
                }

                break; // Break after processing the zap for the current astronaut
            }
        }
        char message[56];
        sprintf(message, "Your current score is %d.", astronauts[player].score);
        zmq_send(socket, message, strlen(message), 0);
    }
}

int main()
{

    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP); // Using REP socket type
    zmq_bind(socket, SERVER_ADDRESS);
    void *publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, PUBLISHER_ADDRESS);

    init_game_state(); // Initialize the game state

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0)
    {
        // Child process: Handles alien updates
        while (1)
        {
            update_aliens();                 // Update alien positions
            update_board();                  // Update the game board
            broadcast_game_state(publisher); // Broadcast the updated state
            render_board();
            usleep(1000000); // Wait 1 second
        }
    }
    else
    {
        char message[32];
        while (1)
        {
            zmq_recv(socket, message, sizeof(message), 0);
            process_message(socket, message); // Handle player messages
            update_board();
            render_board(); // Update the display
        }

        endwin(); // End ncurses
    }

    zmq_close(socket);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    return 0;
}