CC = gcc
LIBS = -lzmq -lncurses -pthread -lm

TARGETS = astronaut-client/astronaut-client astronaut-client-display/astronaut-client-display game-server/game-server outer-space-display/outer-space-display
SRCS = astronaut-client/astronaut-client.c astronaut-client-display/astronaut-client-display.c game-server/game-server.c outer-space-display/outer-space-display.c

all: $(TARGETS)

%: %.c
	$(CC) $< -g -o $@ $(LIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
