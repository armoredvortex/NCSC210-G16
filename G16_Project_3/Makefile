CC = gcc
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -Wall -Wextra -Wpedantic -O2 -pthread
TARGET = filesrv
SRC = src/server.c src/resp.c src/locks.c src/path_utils.c src/logger.c src/file_ops.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
