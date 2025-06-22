#include "klist.h"
#include "kref_alloc.h"


static void list_destructor(void *mem)
{
    struct list *list = (struct list *)mem;
    struct le *le, *le_tmp;
    void *item;

    LIST_FOREACH_SAFE(list, le, le_tmp) {
        item = list_ledata(le);
        kmem_deref(&item);
    }
    list->head = NULL;
    list->tail = NULL;
}

struct list *list_create(char *name)
{
    struct list *list;
    return (struct list *)kzref_alloc(name, sizeof *list, list_destructor);
}

/**
 * Initialise a linked list
 *
 * @param list Linked list
 */
void list_init(struct list *list)
{
    if (!list)
        return;

    list->head = NULL;
    list->tail = NULL;
}



/**
 * Clear a linked list without dereferencing the elements
 *
 * @param list Linked list
 */
void list_clear(struct list *list)
{
    struct le *le;

    if (!list)
        return;

    le = list->head;
    while (le) {
        struct le *next = le->next;
        le->list = NULL;
        le->prev = le->next = NULL;
        le->data = NULL;
        le = next;
    }

    list_init(list);
}

void list_flush(struct list *list)
{
    struct le *le, *le_tmp;
    LIST_FOREACH_SAFE(list, le, le_tmp) {
        void *item = list_ledata(le);
        kmem_deref(&item);
    }
    list_clear(list);
}

/**
 * Append a list element to a linked list
 *
 * @param list  Linked list
 * @param le    List element
 * @param data  Element data
 */
void list_append(struct list *list, struct le *le, void *data)
{
    if (!list || !le)
        return;

    if (le->list)
        return;

    le->prev = list->tail;
    le->next = NULL;
    le->list = list;
    le->data = data;

    if (!list->head)
        list->head = le;

    if (list->tail)
        list->tail->next = le;

    list->tail = le;
}



/**
 * Remove a list element from a linked list
 *
 * @param le    List element to remove
 */
void list_unlink(struct le *le)
{
    struct list *list;

    if (!le || !le->list)
        return;

    list = le->list;

    if (le->prev)
        le->prev->next = le->next;
    else
        list->head = le->next;

    if (le->next)
        le->next->prev = le->prev;
    else
        list->tail = le->prev;

    le->next = NULL;
    le->prev = NULL;
    le->list = NULL;
}





/**
 * Get the first element in a linked list
 *
 * @param list  Linked list
 *
 * @return First list element (NULL if empty)
 */
struct le *list_head(const struct list *list)
{
    return list ? list->head : NULL;
}


/**
 * Get the last element in a linked list
 *
 * @param list  Linked list
 *
 * @return Last list element (NULL if empty)
 */
struct le *list_tail(const struct list *list)
{
    return list ? list->tail : NULL;
}


/**
 * Get the number of elements in a linked list
 *
 * @param list  Linked list
 *
 * @return Number of list elements
 */
size_t list_count(const struct list *list)
{
    size_t n = 0;
    struct le *le;

    if (!list)
        return 0;

    for (le = list->head; le; le = le->next)
        ++n;

    return n;
}

void *list_item_by_number(const struct list *list, int num)
{
    struct le *le;
    int i = 0;
    LIST_FOREACH(list, le) {
        if (i == num)
            return list_ledata(le);
        i++;
    }
    return NULL;
}
