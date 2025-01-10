CC = gcc
CXX = g++
PROTOC = protoc
LIBS = -lzmq -lprotobuf-c -lprotobuf -lncurses -lpthread -lm 

# Protobuf file name (renamed)
PROTO_FILES = points.proto

# Protobuf C sources and headers
PROTO_C_SRCS = $(PROTO_FILES:.proto=.pb-c.c)
PROTO_C_HDRS = $(PROTO_FILES:.proto=.pb-c.h)

# Protobuf C++ sources and headers
PROTO_CPP_SRCS = $(PROTO_FILES:.proto=.pb.cc)
PROTO_CPP_HDRS = $(PROTO_FILES:.proto=.pb.h)

# Target executables
TARGETS = astronaut-client/astronaut-client \
          astronaut-display-client/astronaut-display-client \
          game-server/game-server \
          outer-space-display/outer-space-display \
          space-high-scores/space-high-scores

# Source files for C and C++ files
SRCS_C = astronaut-client/astronaut-client.c \
         astronaut-display-client/astronaut-display-client.c \
         game-server/game-server.c \
         outer-space-display/outer-space-display.c

SRCS_CPP = space-high-scores/space-high-scores.cpp

# Default target
all: $(PROTO_C_SRCS) $(PROTO_CPP_SRCS) $(TARGETS)

# Rule to generate C Protobuf files
%.pb-c.c %.pb-c.h: %.proto
	$(PROTOC) --c_out=. $<

# Rule to generate C++ Protobuf files
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --cpp_out=. $<

# Compile C sources with Protobuf linkage
astronaut-client/astronaut-client: astronaut-client/astronaut-client.c $(PROTO_C_SRCS) $(PROTO_C_HDRS)
	$(CC) $< $(PROTO_C_SRCS) -g -o $@ $(LIBS)

astronaut-display-client/astronaut-display-client: astronaut-display-client/astronaut-display-client.c $(PROTO_C_SRCS) $(PROTO_C_HDRS)
	$(CC) $< $(PROTO_C_SRCS) -g -o $@ $(LIBS)

game-server/game-server: game-server/game-server.c $(PROTO_C_SRCS) $(PROTO_C_HDRS)
	$(CC) $< $(PROTO_C_SRCS) -g -o $@ $(LIBS)

outer-space-display/outer-space-display: outer-space-display/outer-space-display.c $(PROTO_C_SRCS) $(PROTO_C_HDRS)
	$(CC) $< $(PROTO_C_SRCS) -g -o $@ $(LIBS)

# Compile C++ sources with Protobuf linkage
space-high-scores/space-high-scores: space-high-scores/space-high-scores.cpp $(PROTO_CPP_SRCS) $(PROTO_CPP_HDRS)
	$(CXX) $< $(PROTO_CPP_SRCS) -g -o $@ $(LIBS)

# Clean rule to remove generated files
clean:
	rm -f $(TARGETS) $(PROTO_C_SRCS) $(PROTO_C_HDRS) $(PROTO_CPP_SRCS) $(PROTO_CPP_HDRS)
