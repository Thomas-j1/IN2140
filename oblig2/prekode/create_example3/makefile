CFLAGS = -g -std=gnu11 -Wall -Wextra -I..
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = create
FILES = ../allocation.c ../inode.c

all: run

build: create_fs.c ../inode.c
	gcc $(CFLAGS) load_fs.c $(FILES) -o create

run: build
	valgrind $(VFLAGS) ./create

clean:
	rm -f $(BIN)