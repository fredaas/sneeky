#ifndef SNEEKY_H
#define SNEEKY_H

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <math.h>

/* db.c */
typedef struct Table Table;

#define TABLE_SIZE 5
#define TABLE_STRING_SIZE 17
#define TABLE_QUERY_SIZE 1024

struct Table {
    char name[TABLE_SIZE][TABLE_STRING_SIZE];
    int score[TABLE_SIZE];
    int head;
    sqlite3 *db;
};

Table * table_init(void);
void table_free(Table *self);
void table_write_row(Table *self, char *name, int score);
void table_read_rows(Table *self);

/* sneeky.c */
#define PLAYER_NAME_SIZE TABLE_STRING_SIZE

#define MSEC(sec) ((sec) * 1e+3)

#define WALL_RIGHT  (world.w - 2)
#define WALL_LEFT   (0)
#define WALL_TOP    (0)
#define WALL_BOTTOM (world.h - 1)

typedef struct Transition Transition;

struct Transition {
    int *x;
    int *y;
};

typedef struct Apple Apple;

struct Apple {
    int x;
    int y;
};

typedef struct Snake Snake;

struct Snake {
    int *x;
    int *y;
    int d[2];
    int size;
    int q;
    int max_q;
};

typedef struct Player Player;

struct Player {
    char name[PLAYER_NAME_SIZE];
    int score;
};

typedef struct Pane Pane;

struct Pane {
    int w;
    int h;
    WINDOW *win;
    char **buff;
};

enum {
    STATE_START_SCREEN,
    STATE_PLAY_NEW_GAME,
    STATE_PLAY_GAME,
    STATE_GAME_OVER,
    STATE_HIGHSCORE,
    STATE_QUIT,
};

#define SPEED_EASY 15
#define SPEED_MEDIUM 20
#define SPEED_HARD 25

#define TOKEN_SNAKE_HEAD 'O'
#define TOKEN_SNAKE_BODY '#'
#define TOKEN_APPLE 'Q'

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void apple_distribute(void);

void player_init(void);

void snake_init(int x, int y, int size);
void snake_update(void);
int snake_contains(int x, int y);
int snake_collision();

void world_update(void);
void world_draw(void);

void pane_init(int w, int h);
void pane_clear(void);

void display_exit_screen(void);
void display_start_screen(void);
void display_game_over_screen(void);
void display_highscore_screen(void);

void set_difficulty(int level);
void set_game_state(int state);
void start_new_game(void);
void start_game(void);
void exit_and_cleanup(void);

#endif /* SNEEKY_H */
