all: 
	gcc -pg -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 fusexmp.c -o fusexmp -lfuse -lhashkit
