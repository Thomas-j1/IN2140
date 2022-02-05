CFLAGS = -g -std=gnu11 -Wall -Wextra
BIN = vowelshift stringoperations

all: $(BIN)

vowelshift:	vowelshift.c 
	gcc $(CFLAGS) vowelshift.c -o bin/vowelshift

stringoperations: stringoperations.c
	gcc $(CFLAGS) test.c stringoperations.c -o bin/stringoperations

run1:
	./bin/vowelshift

run2:
	./bin/stringoperations

clean:
	rm -f $(BIN)