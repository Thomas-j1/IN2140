CFLAGS = -g -std=gnu11 -Wall -Wextra
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = testing 

all: $(BIN)

testing: testing.c
	gcc $(CFLAGS) testing.c -o testing

run: testing
	./testing

vrun: testing
	valgrind $(VFLAGS) ./testing

clean:
	rm -f $(BIN)