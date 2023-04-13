#include "sneeky.h"

#define TABLE_ASSERT_EXEC(self, query, callback) do { \
    char *err_msg; \
    int ret = sqlite3_exec(self->db, query, callback, (void *)self, &err_msg); \
    if (ret != SQLITE_OK) \
    { \
        fprintf(stderr, "SQL error: %s\n", err_msg); \
        sqlite3_free(err_msg); \
        sqlite3_close(self->db); \
        exit(1); \
    } \
} while (0)

/**
 * @brief Callback for reading highscores.
 *
 * The callback is invoked on every row that is returned from the query.
 * Row values and column names follow the same ordering as in the database
 * table.
 *
 * @param table Pointer to the Table struct.
 * @param numcols Number of columns returned from the query.
 * @param rows Row values.
 * @param colname Column names.
 */
int table_read_callback(void *table, int numcols, char **rows, char **colname)
{
    Table *self = (Table *)table;
    int i = self->head;
    strcpy(self->name[i], rows[0]);
    self->score[i] = atoi(rows[1]);
    self->head++;
    return 0;
}

/**
 * @brief Read all player names and scores.
 *
 * The results are stored in the Table struct.
 */
void table_read_rows(Table *self)
{
    self->head = 0;
    char query[TABLE_QUERY_SIZE];
    char *string =
        "SELECT name, score FROM highscores ORDER BY score DESC LIMIT %d;";
    sprintf(query, string, TABLE_SIZE);
    TABLE_ASSERT_EXEC(self, query, table_read_callback);
}

/**
 * @brief Write player name and score.
 *
 * @param name Player name.
 * @param score Player score.
 */
void table_write_row(Table *self, char *name, int score)
{
    char query[TABLE_QUERY_SIZE];
    char *string =
        "INSERT INTO highscores (name, score) VALUES (\"%s\", %d)";
    sprintf(query, string, name, score);
    TABLE_ASSERT_EXEC(self, query, NULL);
}

/**
 * @brief Free table.
 */
void table_free(Table *self)
{
    sqlite3_close(self->db);
    free(self);
}

/**
 * @brief Initalize database.
 */
Table * table_init(void)
{
    Table *self = (Table *)malloc(sizeof(Table));

    for (int i = 0; i < TABLE_SIZE; i++)
    {
        memset(self->name[i], '\0', TABLE_STRING_SIZE * sizeof(char));
        self->score[i] = 0;
    }

    int ret = sqlite3_open("highscores.db", &self->db);

    if (ret != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(self->db));
        sqlite3_close(self->db);
        exit(1);
    }

    char *query =
        "CREATE TABLE IF NOT EXISTS highscores ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "score INTEGER NOT NULL"
        ");";
    TABLE_ASSERT_EXEC(self, query, NULL);

    return self;
}
