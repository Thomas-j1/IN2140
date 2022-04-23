CFLAGS = -g -std=gnu11 -Wall -Wextra
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = upush_server upush_client tests
OFILES = upush_server.o upush_client.o tests.o common.o
sharedFiles = preCode/send_packet.c common.o

all: $(BIN)

upush_server: upush_server.o $(sharedFiles)
	gcc $(CFLAGS) upush_server.o $(sharedFiles) -o upush_server

upush_server.o: upush_server.c
	gcc $(CFLAGS) -c upush_server.c

runServer: upush_server
	./upush_server 4321 20

valServer: upush_server
	valgrind $(VFLAGS) ./upush_server 4321 20

upush_client: upush_client.o $(sharedFiles)
	gcc $(CFLAGS) upush_client.c $(sharedFiles) -o upush_client

upush_client.o: upush_client.c
	gcc $(CFLAGS) -c upush_client.c

runClient: upush_client
	./upush_client 4321 20

valClient: upush_client
	valgrind $(VFLAGS) ./upush_client 4322 20

tests: tests.o $(sharedFiles)
	gcc $(CFLAGS) tests.c $(sharedFiles) -o tests

tests.o: tests.c
	gcc $(CFLAGS) -c tests.c

runTest: tests
	./tests 4321 20

valTest: tests
	valgrind $(VFLAGS) ./tests 4322 20

common.o: common.c
	gcc $(CFLAGS) -c common.c

clean:
	rm -f $(BIN) *.o