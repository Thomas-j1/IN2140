CFLAGS = -g -std=gnu11 -Wall -Wextra
VFLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all --malloc-fill=0x40 --free-fill=0x23
BIN = create1 create2 create3 load1 load2 load3

buildAll: $(BIN)

runAll: runCreate1 runCreate2 runCreate3 runload1 runload2 runload3

create1: create_example1/create_fs.c
	gcc $(CFLAGS) create_example1/create_fs.c -o create1

runCreate1: create1
	valgrind $(VFLAGS) ./create1

create2: create_example2/create_fs.c
	gcc $(CFLAGS) create_example2/create_fs.c -o create2

runCreate2: create2
	valgrind $(VFLAGS) ./create2

create3: create_example3/create_fs.c
	gcc $(CFLAGS) create_example3/create_fs.c -o create3

runCreate3: create3
	valgrind $(VFLAGS) ./create3

create3: create_example3/create_fs.c
	gcc $(CFLAGS) create_example3/create_fs.c -o create3

runCreate3: create3
	valgrind $(VFLAGS) ./create3

load1: load_example1/load_fs.c
	gcc $(CFLAGS) load_example1/load_fs.c -o load1

runload1: load1
	valgrind $(VFLAGS) ./load1

load2: load_example2/load_fs.c
	gcc $(CFLAGS) load_example2/load_fs.c -o load2

runload2: load2
	valgrind $(VFLAGS) ./load2

load3: load_example3/load_fs.c
	gcc $(CFLAGS) load_example3/load_fs.c -o load3

runload3: load3
	valgrind $(VFLAGS) ./load3



clean:
	rm -f $(BIN)