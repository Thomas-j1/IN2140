CFLAGS = -g -std=gnu11 -Wall -Wextra
BIN = vowelshift stringoperations apple

all: $(BIN)

vowelshift:	vowelshift.c 
	gcc $(CFLAGS) vowelshift.c -o vowelshift

stringoperations: stringoperations.c
	gcc $(CFLAGS) test.c stringoperations.c -o stringoperations

apple: apple.c
	gcc $(CFLAGS) apple.c the_apple.c the_apple.h -o apple

run1:
	./vowelshift

run2:
	./stringoperations

run3:
	./apple

clean:
	rm -f $(BIN)