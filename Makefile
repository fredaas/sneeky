CC := gcc
CFLAGS := -std=c99 -Wall -O2 -D_POSIX_C_SOURCE=199309L -I. -lncurses -lsqlite3 -lm
BIN := sneeky
OBJ := sneeky.o db.o

.PHONY : clean debug install build-debug build-install

all : debug

debug : clean build-debug

install : clean build-install

build-debug : $(OBJ)
	mkdir -p bin
	$(CC) -o bin/$(BIN) $^ $(CFLAGS)

build-install : $(OBJ)
	$(CC) -o /usr/local/bin/$(BIN) $^ $(CFLAGS)

$(OBJ) : %.o : %.c
	$(CC) -c -o $@ $< $(CC_FLAGS)

clean :
	rm -rf sneeky *.o bin
