CFLAGS = -g -std=gnu11 -Wall -Wextra
BIN = vowelshift stringoperations

all: $(BIN)

vowelshift:	vowelshift.c 
	gcc $(CFLAGS) vowelshift.c -o vowelshift

stringoperations: stringoperations.c
	gcc $(CFLAGS) test.c stringoperations.c -o stringoperations

run1:
	./vowelshift

run2:
	./stringoperations

clean:
	rm -f $(BIN)