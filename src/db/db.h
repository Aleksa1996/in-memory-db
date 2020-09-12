/**
 * Defines
 */
#define DB_SIZE 100000000

/**
 * DB structure
 */
typedef struct DB
{
    Item *items[DB_SIZE];
} DB;

int hash(char *key);

int DB_init(DB *db);

int DB_get(DB *db, Item *item);

int DB_set(DB *db, Item *item);

int DB_remove(DB *db, Item *item);