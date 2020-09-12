#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "item.h"
#include "db.h"

/**
 * Hash function used to generate key for hash table
 */
int hash(char *key)
{
    unsigned long h = 5381;
    int c;

    // djb2 algo
    while (c = *key++)
    {
        h = ((h << 5) + h) + c;
    }

    return h;
}

/**
 * Get item from DB
 */
int DB_get(DB *db, Item *item)
{
    if (item->key == NULL)
    {
        return 0;
    }

    int h = hash(item->key) % DB_SIZE;
    Item *tmpItem = db->items[h];

    while (tmpItem != NULL && (strncmp(tmpItem->key, item->key, KEY_SIZE) != 0))
    {
        tmpItem = tmpItem->next;
    }

    if (tmpItem == NULL)
    {
        return 0;
    }

    strncpy(item->value, tmpItem->value, VALUE_SIZE);

    return 1;
}

/**
 * Set item in DB
 */
int DB_set(DB *db, Item *item)
{
    if (item->key == NULL || item->value == NULL)
    {
        return 0;
    }

    int h = hash(item->key) % DB_SIZE;

    item->next = db->items[h];
    db->items[h] = item;

    return 1;
}

/**
 * Remove item in DB
 */
int DB_remove(DB *db, Item *item)
{
    if (item->key == NULL)
    {
        return 0;
    }

    int h = hash(item->key) % DB_SIZE;

    Item *tmpItem = db->items[h];
    Item *prevTmpItem = NULL;

    while (tmpItem != NULL && (strncmp(tmpItem->key, item->key, KEY_SIZE) != 0))
    {
        prevTmpItem = tmpItem;
        tmpItem = tmpItem->next;
    }

    if (tmpItem == NULL)
    {
        return 0;
    }

    if (prevTmpItem == NULL)
    {
        db->items[h] = tmpItem->next;
    }
    else
    {
        prevTmpItem->next = tmpItem->next;
    }

    return 1;
}