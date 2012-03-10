all: 
	gcc -pg -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 fusexmp.c -o fusexmp -lfuse -lhashkit
	gcc -g -pg -O2 -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 -I. btree.c -c -lfuse -lhashkit
	gcc -g -Wall -O2 -pg -DHASH_BLOOM=27 -D_FILE_OFFSET_BITS=64 -I. fusexmp1.c btree.o -o fusexmp1 -lfuse -lhashkit
	gcc -g -Wall -pg nhash.c test_nhash.c -o test_nhash
