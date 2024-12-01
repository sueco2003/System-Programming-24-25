#include <ncurses.h>
#include <zmq.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include "common.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>

#define FIFO_PARENT_TO_CHILD "/tmp/game_fifo_ptc"
#define FIFO_CHILD_TO_PARENT "/tmp/game_fifo_ctp"

// Structs for astronaut and alien
typedef struct
{
    char id;
    int x, y;
    int score;
    time_t stunned_time;
    time_t last_shot_time;
} Astronaut;

typedef struct
{
    int x, y;
} Alien;

// Shared game state
typedef struct
{
    Astronaut astronauts[MAX_PLAYERS];
    Alien aliens[MAX_ALIENS];
    char board[BOARD_SIZE][BOARD_SIZE];
    int astronaut_count;
    int alien_count;
} GameState;

// Constants for X and Y limits for regions
int Y_MAX[] = {0, 1, 18, 19, 17, 17, 17, 17};
int Y_MIN[] = {0, 1, 18, 19, 2, 2, 2, 2};
int X_MAX[] = {17, 17, 17, 17, 0, 1, 18, 19};
int X_MIN[] = {2, 2, 2, 2, 0, 1, 18, 19};

// Function to get the current time in seconds
time_t current_time()
{
    return time(NULL);
}
void handle_signal(int signal)
{
    // Handle cleanup when SIGINT (Ctrl+C) is received
    endwin(); // End ncurses properly
    printf("\nServer shutting down gracefully...\n");
    exit(0); // Exit the program cleanly
}

void update_board(GameState *gameState)
{
    memset(gameState->board, ' ', sizeof(gameState->board));

    // Add remaining aliens
    for (int i = 0; i < gameState->alien_count; i++)
    {
        gameState->board[gameState->aliens[i].x][gameState->aliens[i].y] = '*';
    }

    // Add astronauts
    for (int i = 0; i < gameState->astronaut_count; i++)
    {
        gameState->board[gameState->astronauts[i].x][gameState->astronauts[i].y] = gameState->astronauts[i].id;
    }
}

void render_board(GameState *gameState)
{
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
            mvwaddch(board_win, i + 1, j + 1, gameState->board[i][j]); // Adjust position for border within the window
        }
    }
    wrefresh(board_win);

    for (int i = 0; i < gameState->astronaut_count; i++)
    {
        mvwprintw(score_win, 2 + i, 3, "%c - %d", gameState->astronauts[i].id, gameState->astronauts[i].score);
    }
    wrefresh(score_win);
}

void init_game_state(GameState *gameState)
{
    memset(gameState->board, ' ', sizeof(gameState->board));
    gameState->astronaut_count = 0;
    gameState->alien_count = MAX_ALIENS;

    // Initialize aliens
    for (int i = 0; i < MAX_ALIENS; i++)
    {
        gameState->aliens[i].x = rand() % (BOARD_SIZE - 4) + 2; // Random X position (avoiding borders)
        gameState->aliens[i].y = rand() % (BOARD_SIZE - 4) + 2; // Random Y position (avoiding borders)
    }
}

void update_aliens(GameState *gameState)
{
    for (int i = 0; i < gameState->alien_count; i++)
    {
        // Random movement within the range of 2 to 17
        int dx = (rand() % 3) - 1; // Random -1, 0, or 1 for X movement
        int dy = (rand() % 3) - 1; // Random -1, 0, or 1 for Y movement

        // Update alien position while keeping it within the range 2–17
        gameState->aliens[i].x = (gameState->aliens[i].x + dx + BOARD_SIZE) % BOARD_SIZE;
        gameState->aliens[i].y = (gameState->aliens[i].y + dy + BOARD_SIZE) % BOARD_SIZE;

        // Ensure aliens are within the restricted area (2–17)
        if (gameState->aliens[i].x < 2)
            gameState->aliens[i].x = 2;
        if (gameState->aliens[i].x > 17)
            gameState->aliens[i].x = 17;
        if (gameState->aliens[i].y < 2)
            gameState->aliens[i].y = 2;
        if (gameState->aliens[i].y > 17)
            gameState->aliens[i].y = 17;
    }
}

void remove_alien(int index, GameState *gameState)
{
    for (int i = index; i < gameState->alien_count - 1; i++)
    {
        gameState->aliens[i] = gameState->aliens[i + 1];
    }
    gameState->alien_count--;
}

void broadcast_game_state(void *publisher, GameState *gameState)
{
    char serialized_board[BOARD_SIZE * BOARD_SIZE + 1] = {0};
    char scoring[MAX_PLAYERS * 4] = {0};
    char player_score[8];
    int index = 0;

    // Serialize the board into a single string
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            serialized_board[index++] = gameState->board[i][j];
        }
    }
    serialized_board[index] = '\0';

    // Serialize the player scores
    for (int i = 0; i < gameState->astronaut_count; i++)
    {
        snprintf(player_score, sizeof(player_score), "%d\n", gameState->astronauts[i].score);
        strncat(scoring, player_score, sizeof(scoring) - strlen(scoring) - 1); // Safe concatenation
    }

    zmq_send(publisher, serialized_board, strlen(serialized_board), 0);
    zmq_send(publisher, scoring, strlen(scoring), 0);
}

void process_message(void *socket, char *message, GameState *gameState)
{
    if (strncmp(message, MSG_CONNECT, strlen(MSG_CONNECT)) == 0)
    {
        if (gameState->astronaut_count >= MAX_PLAYERS)
        {
            zmq_send(socket, "Sorry, the game is full", 23, 0);
            return;
        }

        // Randomly choose coordinates for the new astronaut
        int x = X_MIN[gameState->astronaut_count] + (rand() % (X_MAX[gameState->astronaut_count] - X_MIN[gameState->astronaut_count] + 1));
        int y = Y_MIN[gameState->astronaut_count] + (rand() % (Y_MAX[gameState->astronaut_count] - Y_MIN[gameState->astronaut_count] + 1));

        // Add new astronaut to the game state
        char id = 'A' + gameState->astronaut_count;
        gameState->astronauts[gameState->astronaut_count] = (Astronaut){id, x, y, 0, 0, 0};
        gameState->astronaut_count++;

        // Send confirmation response
        char response[27];
        sprintf(response, "Welcome! You are player %c.", id);
        zmq_send(socket, response, strlen(response), 0); // Send the message
    }
    else if (strncmp(message, MSG_DISCONNECT, strlen(MSG_DISCONNECT)) == 0)
    {
        char id = message[strlen(MSG_DISCONNECT) + 1];
        int found = 0;
        for (int i = 0; i < gameState->astronaut_count; i++)
        {
            if (gameState->astronauts[i].id == id)
            {
                for (int j = i; j < gameState->astronaut_count - 1; j++)
                {
                    gameState->astronauts[j] = gameState->astronauts[j + 1];
                }
                gameState->astronaut_count--;
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

        for (int i = 0; i < gameState->astronaut_count; i++)
        {
            if (gameState->astronauts[i].id == id)
            {
                time_t now = current_time();

                // Check if the astronaut is stunned
                if (gameState->astronauts[i].stunned_time != 0 && (now - gameState->astronauts[i].stunned_time) < 10)
                {
                    zmq_send(socket, "You are stunned! Cannot move.", 31, 0);
                    return; // Prevent the astronaut from moving if stunned
                }

                int x = gameState->astronauts[i].x;
                int y = gameState->astronauts[i].y;

                // Handle movement within allowed boundaries
                if (direction == 'U' && x - 1 >= X_MIN[i])
                    gameState->astronauts[i].x--;
                else if (direction == 'D' && x + 1 <= X_MAX[i])
                    gameState->astronauts[i].x++;
                else if (direction == 'L' && y - 1 >= Y_MIN[i])
                    gameState->astronauts[i].y--;
                else if (direction == 'R' && y + 1 <= Y_MAX[i])
                    gameState->astronauts[i].y++;

                break;
            }
        }
        zmq_send(socket, "Move processed", 15, 0);
    }
    else if (strncmp(message, MSG_ZAP, strlen(MSG_ZAP)) == 0)
    {
        char id = message[strlen(MSG_ZAP) + 1];
        int player = -1;

        for (int i = 0; i < gameState->astronaut_count; i++)
        {
            if (gameState->astronauts[i].id == id)
            {
                player = i;
                time_t now = current_time();

                // Check if the astronaut is stunned
                if (gameState->astronauts[i].stunned_time != 0 && (now - gameState->astronauts[i].stunned_time) < 10)
                {
                    zmq_send(socket, "You are stunned! Cannot shoot.", 31, 0);
                    return; // Prevent the astronaut from shooting if stunned
                }

                // Check if enough time has passed since the last shot
                if (now - gameState->astronauts[i].last_shot_time < 3)
                {
                    zmq_send(socket, "You must wait before shooting again.", 38, 0);
                    return; // Prevent shooting if within cooldown period
                }

                int x = gameState->astronauts[i].x, y = gameState->astronauts[i].y;

                // Record the time of the shot
                gameState->astronauts[i].last_shot_time = now;

                // Determine shot direction based on the player index
                if (i == 0 || i == 1) // Players 0 and 1: shoot to the right
                {
                    for (int j = y + 1; j < BOARD_SIZE; j++)
                    {
                        if (gameState->board[x][j] == '*') // Alien hit
                        {
                            gameState->astronauts[i].score++; // Increase score
                            // Remove alien after hit
                            for (int k = 0; k < gameState->alien_count; k++)
                            {
                                if (gameState->aliens[k].x == x && gameState->aliens[k].y == j)
                                {
                                    remove_alien(k, gameState); // Remove alien
                                    break;
                                }
                            }
                        }
                        else if (isalnum(gameState->board[x][j])) // Astronaut hit
                        {
                            // Stun the astronaut if hit
                            for (int k = 0; k < gameState->astronaut_count; k++)
                            {
                                if (gameState->astronauts[k].x == x && gameState->astronauts[k].y == j)
                                {
                                    gameState->astronauts[k].stunned_time = now; // Set stunned time
                                    break;
                                }
                            }
                        }
                        else
                        {
                            gameState->board[x][j] = '-'; // Mark the shot with a line (horizontal)
                        }
                    }
                }
                else if (i == 2 || i == 3) // Players 2 and 3: shoot to the left
                {
                    for (int j = y - 1; j >= 0; j--)
                    {
                        if (gameState->board[x][j] == '*') // Alien hit
                        {
                            gameState->astronauts[i].score++; // Increase score
                            // Remove alien after hit
                            for (int k = 0; k < gameState->alien_count; k++)
                            {
                                if (gameState->aliens[k].x == x && gameState->aliens[k].y == j)
                                {
                                    remove_alien(k, gameState); // Remove alien
                                    break;
                                }
                            }
                        }
                        else if (isalnum(gameState->board[x][j])) // Astronaut hit
                        {
                            // Stun the astronaut if hit
                            for (int k = 0; k < gameState->astronaut_count; k++)
                            {
                                if (gameState->astronauts[k].x == x && gameState->astronauts[k].y == j)
                                {
                                    gameState->astronauts[k].stunned_time = now; // Set stunned time
                                    break;
                                }
                            }
                        }
                        else
                        {
                            gameState->board[x][j] = '-'; // Mark the shot with a line (horizontal)
                        }
                    }
                }
                // Similar handling for players 4-7 ...

                render_board(gameState); // Render the board after the shot is marked
                usleep(500000);          // Show the shot for 500ms

                break; // Break after processing the zap for the current astronaut
            }
        }

        char message[56];
        sprintf(message, "Your current score is %d.", gameState->astronauts[player].score);
        zmq_send(socket, message, strlen(message), 0);
    }
}

void create_fifo(const char *fifo_path)
{
    // Remove FIFO se já existir
    if (access(fifo_path, F_OK) == 0)
    {
        if (unlink(fifo_path) == -1)
        {
            perror("Failed to remove existing FIFO");
            exit(EXIT_FAILURE);
        }
    }

    // Cria o FIFO
    if (mkfifo(fifo_path, 0666) == -1)
    {
        perror("Failed to create FIFO");
        exit(EXIT_FAILURE);
    }
}

void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl SETFL");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    signal(SIGINT, handle_signal); // Handle Ctrl+C gracefully

    unlink(FIFO_PARENT_TO_CHILD);
    unlink(FIFO_CHILD_TO_PARENT);

    // Create FIFOs
    create_fifo(FIFO_PARENT_TO_CHILD);
    create_fifo(FIFO_CHILD_TO_PARENT);

    void *context = zmq_ctx_new();
    if (!context)
    {
        perror("Failed to create ZMQ context");
        unlink(FIFO_PARENT_TO_CHILD);
        unlink(FIFO_CHILD_TO_PARENT);
        exit(EXIT_FAILURE);
    }

    void *socket = zmq_socket(context, ZMQ_REP); // Using REP socket type
    if (!socket || zmq_bind(socket, SERVER_ADDRESS) != 0)
    {
        perror("Failed to bind ZMQ socket");
        zmq_ctx_destroy(context);
        unlink(FIFO_PARENT_TO_CHILD);
        unlink(FIFO_CHILD_TO_PARENT);
        exit(EXIT_FAILURE);
    }

    void *publisher = zmq_socket(context, ZMQ_PUB);
    if (!publisher || zmq_bind(publisher, PUBLISHER_ADDRESS) != 0)
    {
        perror("Failed to bind ZMQ publisher");
        zmq_close(socket);
        zmq_ctx_destroy(context);
        unlink(FIFO_PARENT_TO_CHILD);
        unlink(FIFO_CHILD_TO_PARENT);
        exit(EXIT_FAILURE);
    }

    GameState gameState;
    init_game_state(&gameState); // Initialize the game state

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Fork failed");
        zmq_close(socket);
        zmq_close(publisher);
        zmq_ctx_destroy(context);
        unlink(FIFO_PARENT_TO_CHILD);
        unlink(FIFO_CHILD_TO_PARENT);
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // Child process
        int ptc_fd = open(FIFO_PARENT_TO_CHILD, O_RDONLY | O_NONBLOCK); // Open FIFO in non-blocking mode
        int ctp_fd = open(FIFO_CHILD_TO_PARENT, O_WRONLY);              // Write FIFO can block if no reader is ready
        if (ptc_fd == -1 || ctp_fd == -1)
        {
            perror("Failed to open FIFOs in child process");
            exit(EXIT_FAILURE);
        }

        set_nonblocking(ptc_fd); // Set parent-to-child FIFO to non-blocking
        struct timespec lastUpdateTime;
        clock_gettime(CLOCK_MONOTONIC, &lastUpdateTime); // Get the current time
        while (1)
        {
            // Attempt to read from FIFO (non-blocking)
            read(ptc_fd, &gameState, sizeof(GameState));

            // Check if 1 second has passed
            struct timespec currentTime;
            clock_gettime(CLOCK_MONOTONIC, &currentTime);

            // Calculate elapsed time
            double elapsedTime = (currentTime.tv_sec - lastUpdateTime.tv_sec) +
                                 (currentTime.tv_nsec - lastUpdateTime.tv_nsec) / 1e9;

            // If 1 second has passed, do the periodic updates
            if (elapsedTime >= 1.0)
            {
                // Perform game state updates every second
                update_aliens(&gameState);                    // Update alien positions
                update_board(&gameState);                     // Update the game board
                broadcast_game_state(publisher, &gameState);  // Broadcast the updated state
                render_board(&gameState);                     // Render the game board
                write(ctp_fd, &gameState, sizeof(GameState)); // Send updated state to parent

                // Update the last update time
                lastUpdateTime = currentTime;
            }
        }
        close(ptc_fd);
        close(ctp_fd);
        exit(EXIT_SUCCESS);
    }
    else
    {
        // Parent process
        int ptc_fd = open(FIFO_PARENT_TO_CHILD, O_WRONLY);              // Open FIFO for writing
        int ctp_fd = open(FIFO_CHILD_TO_PARENT, O_RDONLY | O_NONBLOCK); // Open FIFO in non-blocking mode
        if (ptc_fd == -1 || ctp_fd == -1)
        {
            perror("Failed to open FIFOs in parent process");
            kill(pid, SIGKILL); // Ensure child process is killed
            exit(EXIT_FAILURE);
        }

        set_nonblocking(ctp_fd); // Set parent-to-child FIFO to non-blocking
        char message[32];
        while (1)
        {
            ssize_t bytesRead = read(ctp_fd, &gameState, sizeof(GameState));
            if (bytesRead > 0)
            {
                render_board(&gameState); // Update the display
            }

            // Non-blocking ZeroMQ receive
            if (zmq_recv(socket, message, sizeof(message), ZMQ_NOBLOCK) > 0)
            {
                process_message(socket, message, &gameState); // Handle player messages
                update_board(&gameState);                     // Update the game board
                write(ptc_fd, &gameState, sizeof(GameState)); // Send updated state to child
                render_board(&gameState);                     // Update the display
            }
        }
        close(ptc_fd);
        close(ctp_fd);
    }

    // Cleanup
    zmq_close(socket);
    zmq_close(publisher);
    zmq_ctx_destroy(context);
    unlink(FIFO_PARENT_TO_CHILD);
    unlink(FIFO_CHILD_TO_PARENT);

    return EXIT_SUCCESS;
}
