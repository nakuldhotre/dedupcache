all: 
	gcc -g -pg -Wall -O2 -D_FILE_OFFSET_BITS=64 -I. fusexmp2.c nhash.c -o fusexmp2 -lfuse -lhashkit
	gcc -g -pg -Wall -O2 -D_FILE_OFFSET_BITS=64 -I. fusexmp3.c nhash.c -o fusexmp3 -lfuse -lhashkit
