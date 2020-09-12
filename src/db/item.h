/**
 * Defines
 */
#define KEY_SIZE 10000
#define VALUE_SIZE 10000

/**
 * Item structure
 */
typedef struct Item
{
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    struct Item *next;
} Item;