# Compiler and flags
CC = clang
CFLAGS = -std=c17 -Wall -Wextra -O2
LDFLAGS = -lvulkan -lglfw -lcglm -lm

# Directories
SRC_DIR = src/engine
BUILD_DIR = build
BIN = build/app

# Source and object files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

# Default target
all: $(BIN)

# Linking the final binary
$(BIN): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compiling each source file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the app
run: $(BIN)
	./$(BIN)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN)

