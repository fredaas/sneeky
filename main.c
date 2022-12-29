#include "sneeky.h"

Apple apple;
Snake snake;
Player player;
Pane pane;

/******************************************************************************
 *
 * Utils
 *
 *****************************************************************************/

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

/******************************************************************************
 *
 * Player
 *
 *****************************************************************************/

void player_init(void)
{
    memset(player.name, '\0', sizeof(char) * (PLAYER_NAME_SIZE));
    player.score = 0;
}

/******************************************************************************
 *
 * Snake
 *
 *****************************************************************************/

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

void snake_update(void)
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
        set_game_state(STATE_QUIT);
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

/******************************************************************************
 *
 * World
 *
 *****************************************************************************/

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
        player.score += 42;
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

/******************************************************************************
 *
 * Pane
 *
 *****************************************************************************/

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

void pane_clear(void)
{
    for (int y = 0; y < pane.h; y++)
    {
        mvwprintw(pane.win, y + 1, 1, "%*s", pane.w, "");
        memset(pane.buff[y], ' ', pane.w * sizeof(char));
    }
    refresh();
}

/******************************************************************************
 *
 * Displays
 *
 *****************************************************************************/

void display_exit_screen(void)
{
    pane_clear();
    timeout(100);

    char *s1 = "Quit?";
    mvwprintw(pane.win, ceil(pane.h / 2), (pane.w - strlen(s1)) / 2 + 1, "%s", s1);

    char *s2 = "Press Enter to resume,";
    mvwprintw(pane.win, pane.h - 1, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);
    char *s3 = "or 'q' to quit";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s3)) / 2 + 1, "%s", s3);

    wrefresh(pane.win);

    wchar_t key;
    while(1)
    {
        key = getch();
        if (key == 'q')
            set_game_state(STATE_START_SCREEN);
        if (key == '\n')
            set_game_state(STATE_PLAY_GAME);
    }
}

void display_start_screen(void)
{
    pane_clear();
    timeout(100);

    int y = ceil(pane.h / 2);

    char *s1 = "Select difficulty";
    mvwprintw(pane.win, y - 2, (pane.w - strlen(s1)) / 2 + 1, "%s", s1);

    char *s2 = "Press Enter to start";
    mvwprintw(pane.win, pane.h - 1, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);
    char *s3 = "or 'q' to quit";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s3)) / 2 + 1, "%s", s3);

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
        case 'q':
            exit_and_cleanup();
            break;
        }
        mvwprintw(pane.win, y + j, (pane.w - strlen(list[j])) / 2, " %s ", list[j]);
        mvwprintw(pane.win, y + i, (pane.w - strlen(list[i])) / 2, "[%s]", list[i]);
        wrefresh(pane.win);
        flushinp();
    }

    set_game_state(STATE_PLAY_NEW_GAME);
}

int is_ascii_char(wchar_t c)
{
    return ((c >= 'A') && (c <= 'Z')) ||
           ((c >= 'a') && (c <= 'z')) ||
           ((c >= '0') && (c <= '9'));
}

void display_game_over_screen(void)
{
    pane_clear();
    timeout(100);

    int y = ceil(pane.h / 2);

    char *s1 = "Enter name";
    mvwprintw(pane.win, y - 2, (pane.w - strlen(s1)) / 2, "%s", s1);

    char *s2 = "Press Enter to save";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);

    char s3[32];
    sprintf(s3, "Score: %d", player.score);
    mvwprintw(pane.win, y + 2, (pane.w - strlen(s3)) / 2, "%s", s3);

    int buff_size = PLAYER_NAME_SIZE;
    char buff[buff_size];
    memset(buff, '\0', sizeof(char) * buff_size);
    int n = 0; /* Current buff size */

    /* If the player already typed his name, remember it from last time */
    if (strlen(player.name) > 0)
    {
        strcpy(buff, player.name);
        n = strlen(buff);
    }

    wchar_t key;
    while(1)
    {
        key = getch();
        if (key != ERR && is_ascii_char(key))
        {
            if (n < buff_size - 1)
            {
                buff[n++] = key;
                buff[n] = '\0';
            }
        }
        else if (key == KEY_BACKSPACE)
        {
            if (n > 0)
                buff[--n] = '\0';
        }
        if (key == '\n' && n > 0)
        {
            break;
        }
        mvwprintw(pane.win, y, (pane.w - buff_size) / 2, "%*s", buff_size, "");
        mvwprintw(pane.win, y, (pane.w - strlen(buff)) / 2, "%s", buff);
        wrefresh(pane.win);
    }

    strcpy(player.name, buff);

    set_game_state(STATE_HIGHSCORE);
}

void display_highscore_screen(void)
{
    pane_clear();
    timeout(100);

    Table *table = table_init();

    table_write_row(table, player.name, player.score);

    table_read_rows(table);

    int y = ceil(pane.h / 2);

    char *s1 = "Highscores";
    mvwprintw(pane.win, y - 2, (pane.w - strlen(s1)) / 2, "%s", s1);

    char *s2 = "Press Enter to continue";
    mvwprintw(pane.win, pane.h, (pane.w - strlen(s2)) / 2 + 1, "%s", s2);

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        char *name = table->name[i];
        int score = table->score[i];

        if (score > 0)
            mvwprintw(pane.win, y + i, 2,"%d. %s %d", i + 1, name, score);
        else
            mvwprintw(pane.win, y + i, 2, "%d. -", i + 1);
    }

    table_free(table);

    wrefresh(pane.win);

    wchar_t key;
    while(1)
    {
        key = getch();
        if (key == '\n')
            set_game_state(STATE_START_SCREEN);
    }
}

/******************************************************************************
 *
 * Game
 *
 *****************************************************************************/

void set_game_state(int state)
{
    switch (state)
    {
    case STATE_START_SCREEN:
        display_start_screen();
        break;
    case STATE_PLAY_NEW_GAME:
        start_new_game();
        break;
    case STATE_PLAY_GAME:
        start_game();
        break;
    case STATE_GAME_OVER:
        display_game_over_screen();
        break;
    case STATE_HIGHSCORE:
        display_highscore_screen();
        break;
    case STATE_QUIT:
        display_exit_screen();
        break;
    }
}

void start_new_game(void)
{
    srand(3141592654);

    snake_init(0, 0, 3);

    player.score = 0;

    apple.x = rand() % pane.w;
    apple.y = rand() % pane.h;

    start_game();
}

void start_game(void)
{
    pane_clear();
    timeout(0);

    while (1)
    {
        snake_update();
        world_update();
        world_draw();
    }
}

void exit_and_cleanup(void)
{
    delwin(pane.win);
    endwin();
    exit(0);
}

int main(int argc, char **argv)
{
    pane_init(30, 15);

    player_init();

    set_game_state(STATE_START_SCREEN);

    exit_and_cleanup();

    return 0;
}
