# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc
LDFLAGS = -lssl -lcrypto -lpthread 

# Directories
SRC_DIR = src
BIN_DIR = bin

# Targets
all: controller generator

controller: $(SRC_DIR)/controller.c $(SRC_DIR)/common.c $(SRC_DIR)/txgen.c $(SRC_DIR)/pow.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/controller $^ $(LDFLAGS)

generator: $(SRC_DIR)/generator.c $(SRC_DIR)/common.c $(SRC_DIR)/txgen.c $(SRC_DIR)/pow.c 
	$(CC) $(CFLAGS) -o $(BIN_DIR)/generator $^ $(LDFLAGS)
# Run targets with argument passing
run-controller:
	./$(BIN_DIR)/controller config.cfg

run-generator:
	./$(BIN_DIR)/generator $(filter-out $@,$(MAKECMDGOALS))
	@:  # This prevents make from interpreting args as targets

clean:
	rm -f $(BIN_DIR)/* *.o
	rm -f DEIChain_log.txt 
#novo remove log
.PHONY: all clean run-controller run-generator