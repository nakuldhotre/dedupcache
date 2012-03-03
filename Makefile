all: 
	gcc -pg -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 fusexmp.c -o fusexmp -lfuse -lhashkit
	gcc -pg -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 fusexmp1.c -o fusexmp1 -lfuse -lhashkit
