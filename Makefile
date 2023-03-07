CC=gcc
CFLAGS=-std=c99 -Wall -O2 -D_POSIX_C_SOURCE=199309L -I. -lncurses -lsqlite3 -lm

all : clean sneeky

sneeky : main.c db.c
	$(CC) -o $@ $^ $(CFLAGS)
	mkdir -p bin
	mv sneeky bin/sneeky

clean :
	rm -rf sneeky *.o bin
