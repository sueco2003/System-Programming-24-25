CC = gcc
LIBS = -lzmq -lncurses

TARGETS = astronaut-client/astronaut-client game-server/game-server outer-space-display/outer-space-display
SRCS = astronaut-client/astronaut-client.c game-server/game-server.c outer-space-display/outer-space-display.c

all: $(TARGETS)

%: %.c
	$(CC) $< -o $@ $(LIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
