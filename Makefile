CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS =
TARGET = maze
SOURCE = maze.c
OBJS = $(SOURCE:.c=.o)

ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q 2>NUL
    MKDIR = if not exist
else
    RM = rm -f
    MKDIR = mkdir -p
endif

.DEFAULT_GOAL := all

all: $(TARGET) prompt-install

$(TARGET): $(SOURCE)
	@echo "Compiling TTY Maze..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)
	@echo "Build complete!"

prompt-install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo ""
	@echo "Run with: ./$(TARGET)"
else
	@echo ""
	@echo "=========================================="
	@echo "Do you want to install 'maze' command globally?"
	@echo "This requires admin permissions (sudo)"
	@echo "=========================================="
	@echo "Type 'yes' to install, or press Enter to skip:"
	@read -r response; \
	if [ "$$response" = "yes" ] || [ "$$response" = "y" ]; then \
		echo "Installing to /usr/local/bin..."; \
		sudo cp $(TARGET) /usr/local/bin/ && sudo chmod +x /usr/local/bin/$(TARGET) && \
		echo "Installation complete! Run 'maze'."; \
	else \
		echo "Skipped installation. Run with: ./$(TARGET)"; \
	fi
endif

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)
	@echo "Debug build complete!"

release: CFLAGS := -std=c99 -Wall -Wextra -O3 -DNDEBUG -march=native
release: clean $(TARGET)
	@echo "Release build complete!"

sanitize: CFLAGS += -g -fsanitize=address -fsanitize=undefined
sanitize: LDFLAGS += -fsanitize=address -fsanitize=undefined
sanitize: clean $(TARGET)
	@echo "Sanitizer build complete!"

clean:
	@echo "Cleaning build files..."
	-$(RM) $(TARGET) *.o
	@echo "Clean complete!"

distclean: clean
	@echo "Cleaning all generated files..."
	-$(RM) leaderboard.txt maze_colors.txt maze_save.txt
	@echo "Deep clean complete!"

run: $(TARGET)
	@./$(TARGET)

test: $(TARGET)
	@echo "Running basic test..."
	@./$(TARGET) --help || echo "Game built successfully!"

install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "Manual installation required on Windows"
	@echo "Copy $(TARGET) to your desired location"
else
	@echo "Installing to /usr/local/bin (requires sudo)..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation complete! Run 'maze' from anywhere"
endif

uninstall:
ifeq ($(OS),Windows_NT)
	@echo "Manual uninstallation required on Windows"
else
	@echo "Uninstalling from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstallation complete!"
endif

help:
	@echo "TTY Maze - Makefile targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  make          - Build the game (default)"
	@echo "  make debug    - Build with debug symbols (-g -O0)"
	@echo "  make release  - Build optimized release (-O3 -march=native)"
	@echo "  make sanitize - Build with address/UB sanitizers"
	@echo ""
	@echo "Utility targets:"
	@echo "  make clean    - Remove built files"
	@echo "  make distclean- Remove built files + saves/configs"
	@echo "  make run      - Build and run the game"
	@echo "  make test     - Basic build verification"
	@echo ""
	@echo "Installation (Unix only):"
	@echo "  make install  - Install to /usr/local/bin"
	@echo "  make uninstall- Remove from system"
	@echo ""
	@echo "  make help     - Show this help message"

.PHONY: all debug release sanitize clean distclean run test install uninstall help prompt-install