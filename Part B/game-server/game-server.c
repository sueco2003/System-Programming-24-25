#include "common.h"
void proto_buffer_send(GameState *gameState) {
    void *socket = zmq_socket(context, ZMQ_PUSH);
    zmq_connect(socket, "tcp://localhost:5559");
    // Initialize the Score protobuf message
    SimpleMessage msg = SIMPLE_MESSAGE__INIT;
    msg.n_players = MAX_PLAYERS;
    msg.players = malloc(sizeof(Player *) * MAX_PLAYERS);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        msg.players[i] = malloc(sizeof(Player));
        player__init(msg.players[i]);

        char *id = malloc(2); // Allocate memory for the ID
        id[0] = gameState->astronauts[i].id;
        id[1] = '\0';

        msg.players[i]->id = id;
        msg.players[i]->score = gameState->astronauts[i].score;
    }

    // Serialize the message
    int msg_len = simple_message__get_packed_size(&msg);
    char *msg_buf = malloc(msg_len);
    simple_message__pack(&msg, msg_buf);

    // Send the serialized data over ZeroMQ
    zmq_send(socket, msg_buf, msg_len, 0);

    // Cleanup
    free(msg_buf);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        free(msg.players[i]->id);
        free(msg.players[i]);
    }
    free(msg.players);
    zmq_close(socket);
}


/**
 * Updates the game board in the GameState structure.
 *
 * This function clears the current game board and then populates it with
 * the positions of aliens and astronauts. Aliens are represented by '*'
 * and astronauts by their respective IDs. The board is updated based on
 * the current positions stored in the GameState.
 *
 * @param gameState Pointer to the GameState structure containing the
 *                  current positions of aliens and astronauts.
 */
void update_board(GameState *gameState) {
  memset(gameState->board, ' ', sizeof(gameState->board));

  // Add remaining aliens
  for (int i = 0; i < gameState->alien_count; i++)
    gameState->board[gameState->aliens[i].x][gameState->aliens[i].y] = '*';

  // Add astronauts
  for (int i = 0; i < MAX_PLAYERS; i++)
    if (astronaut_ids_in_use[i])
      gameState->board[gameState->astronauts[i].x][gameState->astronauts[i].y] =
          gameState->astronauts[i].id;
}

/**
 * Renders the game board on the screen using ncurses windows.
 *
 * This function initializes the ncurses screen, creates separate windows for
 * displaying line numbers, column numbers, and the game board itself, and
 * then populates these windows with the appropriate data from the provided
 * GameState structure. The board is displayed with a border, and the line
 * and column numbers are displayed along the edges of the board.
 *
 * @param gameState Pointer to the GameState structure containing the current
 *                  state of the game, including the board to be rendered.
 */

void render_board(GameState *gameState) {
  initscr();
  noecho();
  clear();

  WINDOW *line_win = newwin(BOARD_SIZE + 2, 1, 3, 1); // Window for line numbers
  WINDOW *column_win =
      newwin(1, BOARD_SIZE + 2, 1, 3); // Window for column numbers
  WINDOW *board_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2,
                             2); // Window for the board with a border

  for (int i = 0; i < BOARD_SIZE; i++) {
    mvwprintw(line_win, i, 0, "%d", i % 10);
    mvwprintw(column_win, 0, i, "%d", i % 10);
  }

  box(board_win, 0, 0);
  wrefresh(column_win);
  wrefresh(line_win);

  // Print the board
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      mvwaddch(board_win, i + 1, j + 1,
               gameState->board[i][j]); // Adjust position for border
                                        // within the window
    }
  }

  wrefresh(board_win);
}

/**
 * Renders the scores of astronauts on the screen using ncurses.
 *
 * This function initializes the ncurses screen, creates a window for
 * displaying the scores, and populates it with each astronaut's ID
 * and score from the provided GameState structure. The scores are
 * displayed in a bordered window, and the screen is refreshed to
 * show the updated scores.
 *
 * @param gameState Pointer to the GameState structure containing the
 *                  current scores and IDs of the astronauts.
 */
void render_score(GameState *gameState) {
  initscr();
  noecho();
  clear();

  WINDOW *score_win = newwin(BOARD_SIZE + 2, BOARD_SIZE + 2, 2, 25);

  mvwprintw(score_win, 1, 3, "%s", "SCORE");

  box(score_win, 0, 0);

  int j = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (astronaut_ids_in_use[i]) {
      mvwprintw(score_win, 2 + j, 3, "%c - %d", gameState->astronauts[i].id,
                gameState->astronauts[i].score);
      j++;
    }
  }
  wrefresh(score_win);
}

/**
 * Initializes the game state for a new game session.
 *
 * This function sets up the initial state of the game by clearing the game
 * board, setting the astronaut count to zero, and initializing the aliens
 * with random positions within the board boundaries. The alien count is set
 * to the maximum allowed number of aliens.
 *
 * @param gameState Pointer to the GameState structure to be initialized.
 */
void init_game_state(GameState *gameState) {
    bool alien_placement[BOARD_SIZE][BOARD_SIZE] = {false};
    memset(gameState->board, ' ', sizeof(gameState->board));
    gameState->astronaut_count = 0;
    gameState->alien_count = START_ALIENS;

    // Initialize aliens
    for (int i = 0; i < START_ALIENS; i++) {
        int x, y;
        do {
            x = rand() % (BOARD_SIZE - 4) + 2; // Random X position (avoiding borders)
            y = rand() % (BOARD_SIZE - 4) + 2; // Random Y position (avoiding borders)
        } while (alien_placement[x][y]); // Repeat if the spot is already taken

        alien_placement[x][y] = true; // Mark the position as occupied
        gameState->aliens[i].x = x;
        gameState->aliens[i].y = y;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        gameState->astronauts[i] = (Astronaut){0};
    }
}
/**
 * Removes an alien from the GameState's alien array at the specified index.
 *
 * This function shifts all aliens in the array one position to the left
 * starting from the given index, effectively removing the alien at that
 * index. It then decrements the alien count in the GameState.
 *
 * @param index The index of the alien to be removed.
 * @param gameState Pointer to the GameState structure from which the alien
 *                  is to be removed.
 */
void remove_alien(int index, GameState *gameState) {
  for (int i = index; i < gameState->alien_count - 1; i++)
    gameState->aliens[i] = gameState->aliens[i + 1];
  gameState->alien_count--;
}

/**
 * Updates the positions of all aliens in the GameState.
 *
 * This function iterates over each alien in the GameState and adjusts
 * their positions randomly within a specified range. The movement is
 * constrained to ensure aliens remain within the defined area on the
 * board, specifically between coordinates 2 and 17.
 *
 * @param gameState Pointer to the GameState structure containing the
 *                  current positions and count of aliens.
 */

void update_aliens(GameState *gameState) {

    for (int i = 0; i < gameState->alien_count; i++) {
        int new_x, new_y;
        bool valid_move;

            // Random movement within the range of -1, 0, 1
            int dx = (rand() % 3) - 1;
            int dy = (rand() % 3) - 1;

            // Calculate new position
            new_x = gameState->aliens[i].x + dx;
            new_y = gameState->aliens[i].y + dy;

            // Ensure the new position is within the restricted area (2–17)
            if (new_x < 2) new_x = 2;
            if (new_x > 17) new_x = 17;
            if (new_y < 2) new_y = 2;
            if (new_y > 17) new_y = 17;



        if (alien_placement[new_x][new_y]) continue; // Skip if the spot is taken
        alien_placement[gameState->aliens[i].x][gameState->aliens[i].y] = false; // Clear old position
        gameState->aliens[i].x = new_x;
        gameState->aliens[i].y = new_y;
        alien_placement[new_x][new_y] = true; // Mark new position
    }
}


/**
 * Processes incoming messages and updates the game state accordingly.
 *
 * This function handles various types of messages received from players,
 * including connection requests, disconnection requests, movement commands,
 * and shooting actions. It updates the GameState structure based on the
 * message type, manages player positions, scores, and interactions with
 * aliens. The function also sends appropriate responses back to the client
 * through the provided ZeroMQ socket.
 *
 * @param socket Pointer to the ZeroMQ socket for sending responses.
 * @param message The message received from a player.
 * @param gameState Pointer to the GameState structure to be updated.
 */
void process_message(void *socket, char *message, GameState *gameState, void *publisher, char **validation_tokens) {
  if (strncmp(message, MSG_CONNECT, strlen(MSG_CONNECT)) == 0) {
    srand(time(NULL));
    char id = '\0';
    int index;
    if (gameState->astronaut_count >= MAX_PLAYERS) {
      zmq_send(socket, "Sorry, the game is full", 23, 0);
      return;
    }

    for (char player = 'A'; player <= 'H'; player++) {
      index = player - 'A'; // Calcular o índice de 0 a 7

      if (astronaut_ids_in_use[index] == 0) {
        astronaut_ids_in_use[index] = 1; // Marcar como em uso
        id = player;
        break;
      }
    }
    validation_tokens[id - 'A'] = (char *)malloc(7 * sizeof(char));
    // Create a random token
    for (int i = 0; i <= 5; i++) {
      validation_tokens[id - 'A'][i] =
          (rand() % 26) + 'A'; // 26 letters from 'A' to 'Z'
    }
    validation_tokens[id - 'A'][6] = '\0';
    // Randomly choose coordinates for the new astronaut
    int x = X_MIN[index] + (rand() % (X_MAX[index] - X_MIN[index] + 1));
    int y = Y_MIN[index] + (rand() % (Y_MAX[index] - Y_MIN[index] + 1));

    gameState->astronauts[index] = (Astronaut){id, x, y, 0, 0, 0};
    gameState->astronaut_count++;

    // Send confirmation response
    char response[50];
    sprintf(response, "Welcome! You are player %c %s", id,
            validation_tokens[index]);
    zmq_send(socket, response, strlen(response), 0); // Send the message
  } else if (strncmp(message, MSG_DISCONNECT, strlen(MSG_DISCONNECT)) == 0) {
    int found = 0; // Track if the astronaut is found
    char id;
    char *token_message = malloc(7);
    sscanf(message, "Astronaut_disconnect %c %s", &id, token_message);
    token_message[6] = '\0';
    if (!astronaut_ids_in_use[id - 'A'] ||
        strcmp(token_message, validation_tokens[id - 'A']) != 0) {
      zmq_send(socket, "Invalid token! You are cheating", 31, 0);
      return;
    }
    free(token_message);
    int index_to_remove = id - 'A'; // Index corresponds to 'A' to 'H'

    if (index_to_remove >= 0 && index_to_remove < MAX_PLAYERS &&
        astronaut_ids_in_use[index_to_remove] == 1) {
      found = 1; // Mark as found

      // Remove astronaut by resetting their values
      gameState->astronauts[index_to_remove] =
          (Astronaut){0}; // Reset astronaut's state
      astronaut_ids_in_use[index_to_remove] = 0; // Mark ID as available
      gameState->astronaut_count--;              // Decrease astronaut count
      free(validation_tokens[id - 'A']);
    }

    // Send appropriate response
    zmq_send(socket, found ? "Disconnected" : "Astronaut not found", 20, 0);
  } else if (strncmp(message, MSG_MOVE, strlen(MSG_MOVE)) == 0) {
    char id, direction;
    char *token_message = malloc(7);
    sscanf(message, "Astronaut_movement %c %c %s", &id, &direction,
           token_message);
    token_message[6] = '\0';
    if (!astronaut_ids_in_use[id - 'A'] ||
        strcmp(token_message, validation_tokens[id - 'A']) != 0 ||
        token_message == NULL) {
      zmq_send(socket, "Invalid token! You are cheating", 31, 0);
      return;
    }
    free(token_message);
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (gameState->astronauts[i].id == id) {
        time_t now = time(NULL);

        // Check if the astronaut is stunned
        if (gameState->astronauts[i].stunned_time != 0 &&
            (now - gameState->astronauts[i].stunned_time) < 10) {
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
  } else if (strncmp(message, MSG_ZAP, strlen(MSG_ZAP)) == 0) {
    char id = message[strlen(MSG_ZAP) + 1];
    int player = -1;
    int previous_score;
    int play_score = 0;
    char *token_message = malloc(7);
    sscanf(message, "Astronaut_zap %c %s", &id, token_message);
    token_message[6] = '\0';
    if (!astronaut_ids_in_use[id - 'A'] ||
        strcmp(token_message, validation_tokens[id - 'A']) != 0) {
      zmq_send(socket, "Invalid token! You are cheating", 31, 0);
      return;
    }
    free(token_message);
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (gameState->astronauts[i].id == id) {
        previous_score = gameState->astronauts[i].score;

        player = i;
        time_t now = time(NULL);

        // Check if the astronaut is stunned
        if (gameState->astronauts[i].stunned_time != 0 &&
            (now - gameState->astronauts[i].stunned_time) < 10) {
          zmq_send(socket, "You are stunned! Cannot shoot.", 31, 0);
          return; // Prevent the astronaut from shooting if stunned
        }

        // Check if enough time has passed since the last shot
        if (now - gameState->astronauts[i].last_shot_time < 3) {
          zmq_send(socket, "You must wait before shooting again.", 38, 0);
          return; // Prevent shooting if within cooldown period
        }

        int x = gameState->astronauts[i].x, y = gameState->astronauts[i].y;

        // Record the time of the shot
        gameState->astronauts[i].last_shot_time = now;

        // Determine shot direction based on the player index
        if (i == 0 || i == 1) { // Players 0 and 1: shoot to the right
          for (int j = y + 1; j < BOARD_SIZE; j++) {
            if (gameState->board[x][j] == '*') { // Alien hit
              play_score++;
              gameState->astronauts[i].score++; // Increase score
              // Remove alien after hit
              for (int k = 0; k < gameState->alien_count; k++) {
                if (gameState->aliens[k].x == x &&
                    gameState->aliens[k].y == j) {
                  remove_alien(k, gameState); // Remove alien
                  break;
                }
              }
              last_alien_shot = now;
            } else if (isalnum(gameState->board[x][j])) { // Astronaut
              // hit
              // Stun the astronaut if hit
              for (int k = 0; k < gameState->astronaut_count; k++) {
                if (gameState->astronauts[k].x == x &&
                    gameState->astronauts[k].y == j) {
                  gameState->astronauts[k].stunned_time =
                      now; // Set stunned time
                  break;
                }
              }
            } else
              gameState->board[x][j] =
                  '-'; // Mark the shot with a line (horizontal)
          }
        } else if (i == 2 || i == 3) { // Players 2 and 3: shoot to the left
          for (int j = y - 1; j >= 0; j--) {
            if (gameState->board[x][j] == '*') { // Alien hit
              play_score++;
              gameState->astronauts[i].score++; // Increase score
              // Remove alien after hit
              for (int k = 0; k < gameState->alien_count; k++) {
                if (gameState->aliens[k].x == x &&
                    gameState->aliens[k].y == j) {
                  remove_alien(k, gameState); // Remove alien
                  break;
                }
              }
              last_alien_shot = now;
            } else if (isalnum(gameState->board[x][j])) { // Astronaut
              // hit
              // Stun the astronaut if hit
              for (int k = 0; k < gameState->astronaut_count; k++) {
                if (gameState->astronauts[k].x == x &&
                    gameState->astronauts[k].y == j) {
                  gameState->astronauts[k].stunned_time =
                      now; // Set stunned time
                  break;
                }
              }
            } else
              gameState->board[x][j] =
                  '-'; // Mark the shot with a line (horizontal)
          }
        }

        if (i == 4 || i == 5) { // Players 0 and 1: shoot to the right
          for (int j = x + 1; j < BOARD_SIZE; j++) {
            if (gameState->board[j][y] == '*') { // Alien hit
              play_score++;
              gameState->astronauts[i].score++; // Increase score
              // Remove alien after hit
              for (int k = 0; k < gameState->alien_count; k++) {
                if (gameState->aliens[k].x == j &&
                    gameState->aliens[k].y == y) {
                  remove_alien(k, gameState); // Remove alien
                  break;
                }
              }
              last_alien_shot = now;
            } else if (isalnum(gameState->board[j][y])) { // Astronaut
              // hit
              // Stun the astronaut if hit
              for (int k = 0; k < gameState->astronaut_count; k++) {
                if (gameState->astronauts[k].x == j &&
                    gameState->astronauts[k].y == y) {
                  gameState->astronauts[k].stunned_time =
                      now; // Set stunned time
                  break;
                }
              }
            } else
              gameState->board[j][y] =
                  '|'; // Mark the shot with a line (horizontal)
          }
        } else if (i == 6 || i == 7) { // Players 2 and 3: shoot to the left
          for (int j = x - 1; j >= 0; j--) {
            if (gameState->board[j][y] == '*') { // Alien hit
              play_score++;
              gameState->astronauts[i].score++; // Increase score
              // Remove alien after hit
              for (int k = 0; k < gameState->alien_count; k++) {
                if (gameState->aliens[k].x == j &&
                    gameState->aliens[k].y == y) {
                  remove_alien(k, gameState); // Remove alien
                  break;
                }
              }
              last_alien_shot = now;
            } else if (isalnum(gameState->board[j][y])) { // Astronaut
              // hit
              // Stun the astronaut if hit
              for (int k = 0; k < gameState->astronaut_count; k++) {
                if (gameState->astronauts[k].x == j &&
                    gameState->astronauts[k].y == y) {
                  gameState->astronauts[k].stunned_time =
                      now; // Set stunned time
                  break;
                }
              }
            } else
              gameState->board[j][y] =
                  '|'; // Mark the shot with a line (horizontal)
          }
        }

        render_board(gameState); // Render the board after the shot is marked
        zmq_send(publisher, MSG_UPDATE, strlen(MSG_UPDATE), ZMQ_SNDMORE);
        zmq_send(publisher, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), ZMQ_SNDMORE);
        zmq_send(publisher, gameState, sizeof(GameState), 0);
        usleep(500000);
        break; // Break after processing the zap for the current
               // astronaut
      }
    }
    if (play_score > 0) {
      proto_buffer_send(gameState);
    }
    char message[56];
    sprintf(message, "This play: %d points | Current score: %d", play_score,
            gameState->astronauts[player].score);
    zmq_send(socket, message, strlen(message), 0);
  } else {
    zmq_send(socket, "Invalid message", 15, 0);
    return;
  }
  update_board(gameState);
  render_score(gameState);
  render_board(gameState); // Render the board after the shot is marked
}


/**
 * Thread function to update alien positions and broadcast game state.
 *
 * This function runs in a loop, periodically updating the positions of
 * aliens in the GameState, updating the game board, and rendering the
 * board and scores. It then broadcasts the updated game state to clients
 * using a ZeroMQ publisher socket. The function locks a mutex to ensure
 * thread-safe updates to the GameState.
 *
 * @param arg Pointer to the GameState structure to be updated.
 * @return NULL upon completion.
 */
void *alien_position_update(void *arg) {
  GameState *gameState = (GameState *)arg;

  while (on) {
    pthread_mutex_lock(&mutex);

    update_aliens(gameState);
    update_board(gameState);
    render_board(gameState);
    render_score(gameState);

    zmq_send(publisher, MSG_UPDATE, strlen(MSG_UPDATE), ZMQ_SNDMORE);
    zmq_send(publisher, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), ZMQ_SNDMORE);
    zmq_send(publisher, gameState, sizeof(GameState), 0);

    pthread_mutex_unlock(&mutex);
    sleep(1);
  }

  zmq_close(publisher);
  return NULL;
}

/**
 * Thread function to periodically increase the alien count and update the game state.
 *
 * This function runs in a loop, checking if more than 10 seconds have passed since
 * the last alien was shot. If so, it increases the alien count by 10% (up to a maximum
 * limit), assigns random positions to new aliens, updates the game board, and renders
 * the board and scores. The updated game state is then broadcasted to clients using
 * a ZeroMQ publisher socket. The function uses a mutex to ensure thread-safe updates
 * to the GameState.
 *
 * @param arg Pointer to the GameState structure to be updated.
 * @return NULL upon completion.
 */
void *increase_alien_count(void *arg) {
  GameState *gameState = (GameState *)arg;

  while (1) {
    time_t now = time(NULL);

    // Lock mutex for alien count and related updates
    pthread_mutex_lock(&mutex);

    if (now - last_alien_shot > 10) {
      last_alien_shot = now;

      int new_alien_count = (ceil(gameState->alien_count * 1.1) > MAX_ALIENS) 
                      ? MAX_ALIENS 
                      : ceil(gameState->alien_count * 1.1);

// Place new aliens
for (int i = gameState->alien_count; i < new_alien_count; i++) {
    int x, y;
    do {
        x = rand() % (BOARD_SIZE - 4) + 2; // Random X position (avoiding borders)
        y = rand() % (BOARD_SIZE - 4) + 2; // Random Y position (avoiding borders)
    } while (alien_placement[x][y]); // Repeat until an unoccupied spot is found

    gameState->aliens[i].x = x;
    gameState->aliens[i].y = y;
    alien_placement[x][y] = true; // Mark the position as occupied
}

// Update the alien count
gameState->alien_count = new_alien_count;


      update_board(gameState);
      render_board(gameState);
      render_score(gameState);

      zmq_send(publisher, MSG_UPDATE, strlen(MSG_UPDATE), ZMQ_SNDMORE);
      zmq_send(publisher, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), ZMQ_SNDMORE);
      zmq_send(publisher, gameState, sizeof(GameState), 0);
    } else {
      time_t waiting = last_alien_shot + 10 - now;
      pthread_mutex_unlock(&mutex); // Unlock mutex while sleeping
      sleep(waiting);
      continue; // Reacquire lock on next iteration
    }
    pthread_mutex_unlock(&mutex);
  }
  zmq_close(publisher);
  return NULL;
}

/**
 * Signal handler function to monitor keyboard input.
 *
 * This function runs in a loop, continuously checking for keyboard input.
 * If the input is 'q' or 'Q', it sets the 'on' flag to false, effectively
 * signaling the termination of the loop. The function returns NULL upon
 * completion.
 *
 * @return NULL upon completion.
 */
void *signal_handler() {
  
  while (1) {
    int c = getch();
    if (c == 'q' || c == 'Q') {
      zmq_send(publisher, MSG_SERVER, strlen(MSG_SERVER), 0); // Send the topic
      break;
    }
    else continue;
  }

  zmq_close(publisher);
  zmq_close(socket);
  zmq_ctx_destroy(context);
  
  endwin();
  free(gameState);
  exit(0);

  return NULL;
}

void *server_management(void *arg) {

  GameState *gameState = (GameState *)arg;

  // Allocate memory for validation tokens
  char **validation_tokens = malloc(8 * sizeof(char *));
  for (int i = 0; i < 8; i++) validation_tokens[i] = calloc(32, sizeof(char)); // Allocate for each token

  // Main game loop
  char message[32];
  last_alien_shot = time(NULL);

  while (1) {
    memset(message, 0, sizeof(message));
    zmq_recv(socket, message, sizeof(message), 0);
    if (strncmp(message, MSG_SERVER, strlen(MSG_SERVER)) == 0) {
      mvprintw(0, 0, "Server Ended!");
      mvprintw(1, 0, "Scores:");
      for (int i = 0; i < gameState->astronaut_count; ++i) mvprintw(2 + i, 0, "Player %c: %d", gameState->astronauts[i].id, gameState->astronauts[i].score);
      refresh();
      sleep(5);
      break;
    }

    pthread_mutex_lock(&mutex);
    process_message(socket, message, gameState, publisher, validation_tokens); // Handle player messages
    pthread_mutex_unlock(&mutex);

    zmq_send(publisher, MSG_UPDATE, strlen(MSG_UPDATE), ZMQ_SNDMORE);
    zmq_send(publisher, astronaut_ids_in_use, sizeof(astronaut_ids_in_use), ZMQ_SNDMORE);
    zmq_send(publisher, gameState, sizeof(GameState), 0);
    
    

    if (gameState->alien_count == 0) {
      zmq_send(publisher, MSG_SERVER, strlen(MSG_SERVER), 0);
      mvprintw(0, 0, "Game Over!");
      mvprintw(1, 0, "Scores:");
      for (int i = 0; i < gameState->astronaut_count; i++) {
        if (astronaut_ids_in_use[i]) {
        mvprintw(2 + i, 0, "Player %c: %d", gameState->astronauts[i].id, gameState->astronauts[i].score);
        }
      }
      refresh();
      sleep(2);
      break;
    }
  }

  // Cleanup
  for (int i = 0; i < 8; i++) free(validation_tokens[i]);
  free(validation_tokens);

  zmq_close(socket);
  zmq_close(publisher);
  endwin();
  free(gameState);
  exit(0);

  return NULL;
}

/**
 * Main function for the game server application.
 *
 * This function initializes ZeroMQ context and sockets for handling
 * client requests and publishing game state updates. It sets up the
 * game state, renders the initial board and scores, and forks a child
 * process to send periodic keep-alive messages. The parent process
 * continuously listens for player messages, processes them, and
 * broadcasts the updated game state. The game ends when all aliens
 * are removed, displaying the final scores before cleanup.
 *
 * @return EXIT_SUCCESS on successful execution.
 */

int main() {
  srand(time(NULL));

  // Initialize ZMQ context and sockets
  context = zmq_ctx_new();
  socket = zmq_socket(context, ZMQ_REP);
  zmq_bind(socket, SERVER_ADDRESS);
  publisher = zmq_socket(context, ZMQ_PUB);
  zmq_bind(publisher, PUBLISHER_ADDRESS);


  // Initialize game state
  gameState = malloc(sizeof(GameState));
  init_game_state(gameState); // Initialize the game state
  render_board(gameState);
  render_score(gameState);

  // Create threads
  pthread_t server_thread_id, update_thread_id, increase_thread_id, terminate_thread_id;

  pthread_create(&server_thread_id, NULL, server_management, gameState);
  pthread_create(&update_thread_id, NULL, alien_position_update, gameState);
  pthread_create(&increase_thread_id, NULL, increase_alien_count, gameState);
  pthread_create(&terminate_thread_id, NULL, signal_handler, NULL);

  pthread_join(server_thread_id, NULL);
  pthread_join(increase_thread_id, NULL);
  pthread_join(update_thread_id, NULL);
  pthread_join(terminate_thread_id, NULL);

  free(gameState);
  zmq_ctx_destroy(context);

  return EXIT_SUCCESS;
}