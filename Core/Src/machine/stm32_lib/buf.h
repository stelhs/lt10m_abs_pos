#ifndef BUF_H_
#define BUF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kref_alloc.h"
#include "klist.h"

struct buf {
    u8 *d;
    size_t len;
    size_t payload_len;
    struct le le;
};

struct buf *buf_alloc(char *, size_t);
struct buf *buf_strdub(const char *str);
struct buf *buf_strndub(const char *str, size_t len);

static inline struct buf *bufz_alloc(char *name, size_t size)
{
    struct buf *buf = buf_alloc(name, size);
    if (!buf)
        return NULL;

    memset(buf->d, 0, size);
    return buf;
}

#define buf_list_append(list, buf) list_append(list, &buf->le, buf)

#define buf_deref(buf) kmem_deref(buf)
struct buf *buf_cpy(const void *src, size_t len);
#define buf_ref(buf) kmem_ref(buf);
struct buf *buf_concatenate(const struct buf *b1, const struct buf *b2);
char *buf_strcpy(struct buf *buf);
void _buf_dump(const struct buf *buf, const char *name);
#define buf_dump(buf) _buf_dump(buf, #buf)
void _buf_list_dump(const struct list *list, const char *name);
#define buf_list_dump(list) _buf_list_dump(list, #list)
struct buf *buf_list_join(const struct list *list, char sep);
void buf_put(struct buf *buf, size_t payload_len);
struct list *buf_split(const struct buf *buf, char sep);
struct buf *buf_trim(const struct buf *buf);
struct buf *buf_sprintf(const char* format, ...);
void buf_erase(struct buf *buf);
struct buf *file_get_contents(const char *filename);
int buf_cmp(const struct buf *b1, const struct buf *b2);
void buf_memset(const struct buf *buf, int val);

static inline size_t buf_len(const struct buf *buf)
{
    return buf->payload_len;
}

#ifdef __cplusplus
}
#endif

#endif /* BUF_H_ */
