#include "klist.h"
#include "kref.h"
#include "kref_alloc.h"
#include <stdarg.h>
#include <stdlib.h>

#define KMEM_MAGIC "km"
void halt(char *reason);


__attribute__((packed)) struct kralloc {
    char magic[3];
    u8 shift_size;
    struct kref kref;
    size_t size;
#ifdef KMEM_LINK_MEM
        struct list list;
        struct le le;
        struct kralloc *linked_mem;
#endif
#ifdef KMEM_DEBUG
    struct le debug_le;
    char *name;
#endif
    void (*destructor)(void *mem);
};

#ifdef KMEM_DEBUG
static struct list kmem_list = LIST_INIT;
static size_t max_usage = 0;

char *kmem_name(void *mem)
{
    struct kralloc *a = (struct kralloc *)mem - 1;
    return a->name;
}
#else
char *kmem_name(void *mem)
{
    return "";
}
#endif


static void k_destructor(struct kref *kref)
{
    struct kralloc *a = (struct kralloc *)container_of(kref, struct kralloc, kref);
    struct kralloc *a_root;
#ifdef KMEM_LINK_MEM
    struct le *le, *safe_le;
#endif

    /* find root memory descriptor */
    a_root = a;
#ifdef KMEM_LINK_MEM
    while(a_root->linked_mem) {
        a_root = a_root->linked_mem;

        if(strcmp(a_root->magic, KMEM_MAGIC) != 0)
            break;
    }
    /* free all linked memories */
    LIST_FOREACH_SAFE(&a_root->list, le, safe_le) {
        a = (struct kralloc *)list_ledata(le);
        list_unlink(le);
        if (a->destructor) {
            a->destructor(a + 1);
        }
        strcpy(a->magic, "\0");
        free((u8 *)a - a->shift_size);
    }
    /* free root memory */
#endif
    if (a_root->destructor)
        a_root->destructor(a_root + 1);
    a_root->magic[0] = 0;

#ifdef KMEM_DEBUG
    list_unlink(&a_root->debug_le);
#endif
    free((u8 *)a_root - a_root->shift_size);
}


/**
 * Allocate aligned memory
 * @param size: needed memory size
 * @param flags: kmalloc flags
 * @param align: align divider (4, 8, 16 ant etc.) or 0 if no align
 * @param destructor: destructor for this memory
 */
void *kref_alloc_aligned(char *name, int size, uint align, void (*destructor)(void *mem))
{
    struct kralloc *a;
    void *ptr, *end_ptr, *aligned_ptr;
    void *addr;

    u8 shift;

    /* fixed align value if incorrect */
    if (align) {
        shift = (fls(align) - 1);
        align = 1 << shift;
    }
    ptr = malloc(sizeof(struct kralloc) + size + align);
    if (!ptr)
        halt("outOfMemory");

    end_ptr = (u8 *)ptr + sizeof(*a);
    if (align && ((ulong)end_ptr % align))
        aligned_ptr = (void *)((((ulong)end_ptr >> shift) + 1) << shift);
    else
        aligned_ptr = end_ptr;

    a = (struct kralloc *)((u8 *)aligned_ptr - sizeof(*a));
    a->shift_size = ((u8 *)a - (u8 *)ptr);
    a->size = size;
    strcpy(a->magic, KMEM_MAGIC);
    a->destructor = destructor;
    kref_init(&a->kref);

#ifdef KMEM_LINK_MEM
    a->linked_mem = NULL; /* mark as root memory */
    memset(&a->list, 0, sizeof a->list);
    memset(&a->le, 0, sizeof a->le);
#endif

#ifdef KMEM_DEBUG
    a->name = name;
    memset(&a->debug_le, 0, sizeof a->debug_le);
    list_append(&kmem_list, &a->debug_le, a);
#endif
    addr = (void *)(a + 1);
    if (((size_t)addr + a->size) > max_usage)
        max_usage = ((size_t)addr + a->size);
    return addr;
}

void kref_set_name(void *mem, char *name)
{
#ifdef KMEM_DEBUG
    struct kralloc *a = (struct kralloc *)mem - 1;
    if(strcmp(a->magic, KMEM_MAGIC) != 0)
        return;
    a->name = name;
#endif
}

/**
 * Increase memory link counter
 * @param mem: pointer to memory allocated
 *         with kref_alloc()
 */
void *kmem_ref(void *mem)
{
    struct kralloc *a = (struct kralloc *)mem - 1;

    if(strcmp(a->magic, KMEM_MAGIC) != 0)
        return NULL;

#ifdef KMEM_LINK_MEM
    if (a->linked_mem)
        kref_get(&a->linked_mem->kref);
    else
        kref_get(&a->kref);
#else
    kref_get(&a->kref);
#endif

    return mem;
}


/**
 * Return allocated size
 */
size_t kmem_size(void *mem)
{
    struct kralloc *a = (struct kralloc *)mem - 1;
    if(strcmp(a->magic, KMEM_MAGIC) != 0)
        return 0;

    return a->size;
}

#ifdef KMEM_LINK_MEM
/**
 * Link kmem to another kmem. Set mem_new as child of root memory
 * @param mem_new - pointer to memory allocated with kref_alloc()
 * @param mem_parent - pointer to root memory allocated with kref_alloc()
 * @return 0 if ok
 */
int kmem_link_to_kmem(void *mem_new, void *mem_parent)
{
    struct kralloc *a_new = (struct kralloc *)mem_new - 1;
    struct kralloc *a_parent = (struct kralloc *)mem_parent - 1;
    struct kralloc *a_root;

    if(strcmp(a_new->magic, KMEM_MAGIC) != 0)
        return -1;

    if(strcmp(a_parent->magic, KMEM_MAGIC) != 0)
        return -1;


    /* find root memory descriptor */
    a_root = a_parent;
    while(a_root->linked_mem) {
        a_root = a_root->linked_mem;

        if(strcmp(a_root->magic, KMEM_MAGIC) != 0)
            return -1;
    }

    /* add current memory descriptor to
     * head list of all linked descriptors */
    list_append(&a_root->list, &a_new->le, a_new);
    a_new->linked_mem = a_root; /* mark mem_new as child memory */
    return 0;
}
#endif

/**
 * Get reference counter value
 * @param mem - pointer to memory allocated with kref_alloc()
 */
int kmem_get_ref_count(void *mem)
{
    struct kralloc *a = (struct kralloc *)mem - 1;
    uint cnt;

    if(strcmp(a->magic, KMEM_MAGIC) != 0)
        return 0;

#ifdef KMEM_LINK_MEM
    if (a->linked_mem)
        cnt = a->linked_mem->kref.refcount;
    else
        cnt = a->kref.refcount;
#else
    cnt = a->kref.refcount;
#endif
    return cnt;
}

/**
 * Decrease memory link counter and free memory
 * if link counter reach to zero
 * @param mem: pointer to memory allocated
 *         with kref_alloc() memory pointer
 */
void *_kmem_deref(void **mem)
{
    struct kralloc *a;
    int rc;
    void *m;
    if (!mem)
        return NULL;

    m = *mem;

    if (!m)
        return NULL;

    a = (struct kralloc *)m - 1;

    if(strcmp(a->magic, KMEM_MAGIC) != 0)
        return NULL;

#ifdef KMEM_LINK_MEM
    if (a->linked_mem)
        rc = kref_put(&a->linked_mem->kref, k_destructor);
    else
        rc = kref_put(&a->kref, k_destructor);
#else
    rc = kref_put(&a->kref, k_destructor);
#endif

    *mem = NULL;
    if (rc)
        return NULL;
    return m;
}

/**
 * Make sting by format in allocated memory
 * @param flags - GFP_ flags
 * @param fmt - format
 * @return formatted string or NULL if no enought memory
 */
char *kref_sprintf(const char *fmt, ...)
{
    va_list vargs, vargs_tmp;
    char *p;
    size_t len;

    va_start(vargs, fmt);
    va_copy(vargs_tmp, vargs);
    len = vsnprintf(NULL, 0, fmt, vargs_tmp);
    va_end(vargs_tmp);

    p = (char *)kref_alloc("kref_sprintf", len + 1, NULL);
    if (!p)
        return NULL;

    vsnprintf(p, len + 1, fmt, vargs);
    va_end(vargs);
    return p;
}

char *kref_strdub(const char *src)
{
    char *dst;
    size_t len = strlen(src);
    dst = (char *)kref_alloc("kref_strdub", len + 1, NULL);
    if (!dst)
        return NULL;
    memcpy(dst, src, len + 1);
    return dst;
}

char *kref_strndub(const char *src, size_t len)
{
    char *dst;
    if (len == 0)
        return NULL;
    dst = (char *)kref_alloc("kref_strndub", len + 1, NULL);
    if (!dst)
        return NULL;
    memcpy(dst, src, len);
    dst[len] = 0;
    return dst;
}


void *kref_concatenate_mem(void *mem1, void *mem2)
{
    size_t len1 = kmem_size(mem1);
    size_t len2 = kmem_size(mem2);
    u8 *result;

    if (!len1 || !len2)
        return NULL;

    result = (u8 *)kref_alloc("kref_concatenate_mem",
                              len1 + len2, NULL);
    if (!result)
        return NULL;

    memcpy(result, mem1, len1);
    memcpy(result + len1, mem2, len2);
    return result;
}

#ifdef KMEM_DEBUG

#define KMEM_TYPES_MAX 64
struct kmem_type {
    char *name;
    size_t num;
    size_t size;
} kmem_types[KMEM_TYPES_MAX];


extern u32 _Min_Heap_Size;
extern u32 end;
void kmem_print_stat(void)
{
    struct le *le;
    size_t total_num = list_count(&kmem_list);
    size_t used_size = 0;
    struct kmem_type *t = NULL;
    uint i;
    size_t len;
    u32 heap_size = (u32)&_Min_Heap_Size;
    u32 heap_addr = (u32)&end;

    memset(kmem_types, 0, sizeof kmem_types);

    LIST_FOREACH(&kmem_list, le) {
        bool exist = FALSE;
        struct kralloc *a = (struct kralloc*)list_ledata(le);
        t = kmem_types;
        len = sizeof(*a) + kmem_size(a + 1);
        for (i = 0; i < KMEM_TYPES_MAX; i++) {
            t = kmem_types + i;
            if (!t->name)
                break;
            if (t->name == a->name) {
                exist = TRUE;
                t->num++;
                t->size += len;
                break;
            }
        }
        if (!exist) {
            t->name = a->name;
            t->num++;
            t->size += len;
        }
        used_size += len;
    }

    printf("\tMemory status\r\n");
    printf("struct kralloc size: %d\r\n", sizeof(struct kralloc));
    printf("Used memory: %d of %lu\r\n", used_size, heap_size);
    printf("Free memory: %lu\r\n", heap_size - used_size);
    printf("Max usage: %lu\r\n", max_usage - heap_addr);

    printf("Total allocated objects: %d\r\n", total_num);

    printf("\t   Allocated objects table\r\n");
    printf("        object name       | quantity | size  \r\n");
    printf("  -------------------------------------------\r\n");
    for (i = 0; kmem_types[i].name; i++) {
        t = kmem_types + i;
        printf("  %-23s | %-8d | %-4d\r\n", t->name, t->num, t->size);
    }
    printf("\r\n");
}

void kmem_print_table(void)
{
    struct le *le;
    u32 i = 0;

    printf("id,name,address,size\r\n");
    LIST_FOREACH(&kmem_list, le) {
        struct kralloc *a = (struct kralloc*)list_ledata(le);
        printf("%lu,%s,%lu,%d\r\n", i, a->name,
               (u32)a, a->size + sizeof(struct kralloc));
        i++;
    }
    printf("\r\n");
}
#endif
