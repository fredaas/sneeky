all : clean main

main : main.c
	gcc -o main $^ -std=c99 -Wall -O2 -D_POSIX_C_SOURCE=199309L -lncurses
	mkdir bin
	mv main bin/sneeky

clean :
	rm -rf main *.o bin
