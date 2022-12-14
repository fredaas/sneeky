#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ncurses.h>
#include <math.h>
#include <sqlite3.h>

#define MSEC(sec) ((sec) * 1e+3)

#define WALL_RIGHT  (world.w - 2)
#define WALL_LEFT   (0)
#define WALL_TOP    (0)
#define WALL_BOTTOM (world.h - 1)

struct Apple {
    int x;
    int y;
} apple;

struct Snake {
    int *x;
    int *y;
    int d[2];
    int size;
    int q;
    int max_q;
} snake;

struct Pane {
    int w;
    int h;
    WINDOW *win;
    char **buff;
} pane;

enum {
    STATE_START_SCREEN,
    STATE_RUN_GAME,
    STATE_GAME_OVER
};

void display_start_screen(void);
void display_score_screen(void);
void run_game(void);

const int FPS = 15;
const int T_SLEEP_MS = 1000 / FPS;

const char TOKEN_SNAKE_HEAD = 'O';
const char TOKEN_SNAKE_BODY = '#';
const char TOKEN_APPLE = 'Q';

static inline int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

static inline void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

static inline double walltime(void)
{
    static struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1e-6;
}

static inline wchar_t nextkey(void)
{
    double t_start = walltime();
    wchar_t key;
    while (((key = getch()) == ERR)
             && (MSEC(walltime() - t_start) < T_SLEEP_MS));
    sleep_ms(T_SLEEP_MS - MSEC(walltime() - t_start));
    flushinp();
    return key;
}

void set_game_state(int state)
{
    switch (state)
    {
    case STATE_START_SCREEN:
        display_start_screen();
        break;
    case STATE_RUN_GAME:
        run_game();
        break;
    case STATE_GAME_OVER:
        display_score_screen();
        break;
    }
}

void snake_init(int x, int y, int size)
{
    snake.q = size - 1;
    snake.max_q = pane.w * pane.h;
    snake.x = (int *)malloc(snake.max_q * sizeof(int));
    snake.y = (int *)malloc(snake.max_q * sizeof(int));
    snake.d[0] = 1;
    snake.d[1] = 0;
    snake.size = size;
    for (int i = 0; i < size; i++)
    {
        snake.x[i] = x + i;
        snake.y[i] = y;
        if (i < snake.q)
            pane.buff[y][x + i] = TOKEN_SNAKE_BODY;
        else
            pane.buff[y][x + i] = TOKEN_SNAKE_HEAD;
    }
}

int snake_collision()
{
    if (snake.size < 5)
        return 0;

    int x = snake.x[snake.q];
    int y = snake.y[snake.q];

    for (int i = 0; i < snake.size - 4; i++)
    {
        int k = mod(snake.q - 4 - i, snake.max_q);
        if ((snake.x[k] == x) && (snake.y[k] == y))
            return 1;
    }

    return 0;
}

void world_update(void)
{
    int nx = snake.x[snake.q] + snake.d[0];
    int ny = snake.y[snake.q] + snake.d[1];

    if (nx >= pane.w)
        nx = 0;
    else if (nx < 0)
        nx = pane.w - 1;
    if (ny >= pane.h)
        ny = 0;
    else if (ny < 0)
        ny = pane.h - 1;

    if (apple.x == nx && apple.y == ny)
    {
        apple.x = rand() % pane.w;
        apple.y = rand() % pane.h;
        snake.size++;
    }

    snake.q = (snake.q + 1) % snake.max_q;
    snake.x[snake.q] = nx;
    snake.y[snake.q] = ny;

    if (snake_collision())
        set_game_state(STATE_GAME_OVER);
}

void world_draw(void)
{
    /* Draw snake */
    int s0 = snake.q;
    int s1 = mod(snake.q - 1, snake.max_q);
    int s2 = mod(snake.q - snake.size, snake.max_q);
    pane.buff[snake.y[s0]][snake.x[s0]] = TOKEN_SNAKE_HEAD;
    pane.buff[snake.y[s1]][snake.x[s1]] = TOKEN_SNAKE_BODY;
    pane.buff[snake.y[s2]][snake.x[s2]] = ' ';
    /* Draw apple */
    pane.buff[apple.y][apple.x] = TOKEN_APPLE;
    /* Draw world */
    for (int y = 0; y < pane.h; y++)
        mvwprintw(pane.win, y + 1, 1, "%s", pane.buff[y]);
    wrefresh(pane.win);
}

void keyboard_read_input(void)
{
    wchar_t key = nextkey();

    int dx = snake.d[0];
    int dy = snake.d[1];

    if ((key == 'a' || key == KEY_LEFT) && dx != 1)
    {
        snake.d[0] = -1;
        snake.d[1] = 0;
    }
    else if ((key == 'd' || key == KEY_RIGHT) && dx != -1)
    {
        snake.d[0] = 1;
        snake.d[1] = 0;
    }
    else if ((key == 'w' || key == KEY_UP) && dy != 1)
    {
        snake.d[0] = 0;
        snake.d[1] = -1;
    }
    else if ((key == 's' || key == KEY_DOWN) && dy != -1)
    {
        snake.d[0] = 0;
        snake.d[1] = 1;
    }

    else if (key == 'q')
    {
        endwin();
        exit(0);
    }
}

void pane_clear(void)
{
    for (int y = 0; y < pane.h; y++)
    {
        mvwprintw(pane.win, y + 1, 1, "%*s", pane.w, "");
        memset(pane.buff[y], ' ', pane.w * sizeof(char));
    }
    refresh();
}

void pane_init(int w, int h)
{
    /* Setup main window */
    initscr();
    raw();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    refresh();

    /* Get terminal dimensions */
    int tw, th;
    getmaxyx(stdscr, th, tw);

    /* Create pane */
    int bw = w + 2;
    int bh = h + 2;
    pane.win = newwin(bh, bw, (th - bh) / 2, (tw - bw) / 2);

    /* Create pane buffer */
    pane.w = w;
    pane.h = h;
    pane.buff = (char **)malloc(pane.h * sizeof(char *));
    for (int i = 0; i < pane.h; i++)
    {
        pane.buff[i] = (char *)malloc(pane.w * sizeof(char));
        memset(pane.buff[i], ' ', pane.w * sizeof(char));
    }

    box(pane.win, 0, 0);

    char *title = " Sneeky ";
    mvwprintw(pane.win, 0, (bw - strlen(title)) / 2, "%s", title);

    wrefresh(pane.win);
}

void display_start_screen(void)
{
    pane_clear();

    timeout(100);

    int y = ceil(pane.h / 2);

    char *s1 = "Select difficulty";
    mvwprintw(pane.win, y - 2, (pane.w - strlen(s1)) / 2 + 1, "%s", s1);

    char *s2 = "Press Enter to start";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);

    int size = 3;
    char *list[] = { "Easy", "Medium", "Hard" };
    for (int i = i; i < size; i++)
        mvwprintw(pane.win, y + i, (pane.w - strlen(list[i])) / 2, " %s ", list[i]);

    int i = 0;
    int j = 0;
    wchar_t key;
    while((key = getch()) != '\n')
    {
        switch(key)
        {
        case KEY_UP:
            j = i--;
            i = mod(i, size);
            break;
        case KEY_DOWN:
            j = i++;
            i = mod(i, size);
            break;
        }
        mvwprintw(pane.win, y + j, (pane.w - strlen(list[j])) / 2, " %s ", list[j]);
        mvwprintw(pane.win, y + i, (pane.w - strlen(list[i])) / 2, "[%s]", list[i]);
        wrefresh(pane.win);
        flushinp();
    }

    set_game_state(STATE_RUN_GAME);
}

int is_ascii_char(wchar_t c)
{
    return ((c >= 'A') && (c <= 'Z')) ||
           ((c >= 'a') && (c <= 'z')) ||
           ((c >= '0') && (c <= '9'));
}

void display_score_screen(void)
{
    pane_clear();

    timeout(100);

    int y = ceil(pane.h / 2);

    char *s1 = "Enter name";
    mvwprintw(pane.win, y - 2, (pane.w - strlen(s1)) / 2, "%s", s1);

    char *s2 = "Press Enter to save";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);

    char s3[32];
    sprintf(s3, "Score: %d", 42);
    mvwprintw(pane.win, y + 2, (pane.w - strlen(s3)) / 2, "%s", s3);

    int buff_size = 16 + 1;
    char buff[buff_size];
    memset(buff, '\0', sizeof(char) * buff_size);

    int i = 0;
    wchar_t key;
    while(1)
    {
        key = getch();
        if (key != ERR && is_ascii_char(key))
        {
            if (i < buff_size - 1)
            {
                buff[i++] = key;
                buff[i] = '\0';
            }
        }
        else if (key == KEY_BACKSPACE)
        {
            if (i > 0)
                buff[--i] = '\0';
        }
        if (key == '\n' && i > 0)
        {
            break;
        }
        mvwprintw(pane.win, y, (pane.w - buff_size) / 2, "%*s", buff_size, "");
        mvwprintw(pane.win, y, (pane.w - strlen(buff)) / 2, "%s", buff);
        wrefresh(pane.win);
    }

    set_game_state(STATE_START_SCREEN);
}

void run_game(void)
{
    pane_clear();

    srand(3141592654);

    timeout(0);

    snake_init(0, 0, 3);

    apple.x = rand() % pane.w;
    apple.y = rand() % pane.h;

    while (1)
    {
        keyboard_read_input();
        world_update();
        world_draw();
    }
}

int main(int argc, char **argv)
{
    pane_init(30, 15);

    set_game_state(STATE_START_SCREEN);

    delwin(pane.win);
    endwin();

    return 0;
}
