CFLAGS = -g -std=gnu11 -Wall -Wextra -I..
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = load
FILES = ../allocation.c ../inode.c

all: run

build: load_fs.c ../inode.c
	gcc $(CFLAGS) load_fs.c $(FILES) -o load

run: build
	valgrind $(VFLAGS) ./load

clean:
	rm -f $(BIN)