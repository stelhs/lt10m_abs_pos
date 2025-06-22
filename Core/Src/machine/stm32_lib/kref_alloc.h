#ifndef KREF_ALLOC_H_
#define KREF_ALLOC_H_

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

void *kref_alloc_aligned(char *name, int size, uint align, void (*destructor)(void *mem));
void *kmem_ref(void *mem);
int kmem_link_to_kmem(void *mem_new, void *mem_parent);

void *_kmem_deref(void **mem);
#define kmem_deref(mem) _kmem_deref((void **)(mem))

char *kref_sprintf(const char *fmt, ...);
char *kref_strdub(const char *src);
char *kref_strndub(const char *src, size_t len);
int kmem_get_ref_count(void *mem);
size_t kmem_size(void *mem);
char *kmem_name(void *mem);
void *kref_concatenate_mem(void *mem1, void *mem2);

/**
 * Allocate memory
 * @param size: needed memory size
 * @param flags: kmalloc flags
 * @param destructor: destructor for this memory
 */
static inline void *kref_alloc(char *name, size_t size, void (*destructor)(void *mem))
{
    return kref_alloc_aligned(name, size, 0, destructor);
}


/**
 * Allocate zeroed memory
 * @param size: needed memory size
 * @param flags: kmalloc flags
 * @param destructor: destructor for this memory
 */
static inline void *kzref_alloc(char *name, size_t size, void (*destructor)(void *mem))
{
    void *mem = kref_alloc(name, size, destructor);
    if (!mem)
        return NULL;

    memset(mem, 0, size);
    return mem;
}

void kref_set_name(void *mem, char *name);

#ifdef KMEM_DEBUG
void kmem_print_stat(void);
void kmem_print_table(void);
#endif

#endif /* KREF_ALLOC_H_ */
