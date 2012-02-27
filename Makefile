all: 
	gcc -pg -D_FILE_OFFSET_BITS=64 fusexmp.c -o fusexmp -lfuse -lhashkit
