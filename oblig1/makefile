CFLAGS = -g -std=gnu11 -Wall -Wextra
BIN = vowelshift

all: $(BIN)

vowelshift:	vowelshift.c 
	gcc $(CFLAGS) vowelshift.c -o vowelshift

run1:
	./vowelshift

clean:
	rm -f $(BIN)