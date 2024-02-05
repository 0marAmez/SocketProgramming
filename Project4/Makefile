#------------------------------------------------------------------------------
#  Omar Amezquita 03/01/23 
#  Makefile for Lab4
#  make            makes 
#  clean           removes binary files
#------------------------------------------------------------------------------
# Compiler and flags
CC = g++
CFLAGS = -Wall -Wextra -pedantic -std=c++11 -pthread

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source files
CLIENT_SRC = $(SRC_DIR)/client.cpp $(SRC_DIR)/myclient.cpp
SERVER_SRC = $(SRC_DIR)/server.cpp $(SRC_DIR)/myserver.cpp

# Object files
CLIENT_OBJ = $(CLIENT_SRC:%.cpp=%.o)
SERVER_OBJ = $(SERVER_SRC:%.cpp=%.o)

# Targets
all: $(BIN_DIR)/myclient $(BIN_DIR)/myserver

$(BIN_DIR)/myclient: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/myserver: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_OBJ): $(SRC_DIR)/client.h
$(SERVER_OBJ): $(SRC_DIR)/server.h

$(CLIENT_OBJ): | $(BIN_DIR)
$(SERVER_OBJ): | $(BIN_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(BIN_DIR)/myclient $(BIN_DIR)/myserver
