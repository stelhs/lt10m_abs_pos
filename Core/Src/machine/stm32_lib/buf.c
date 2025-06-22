#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "buf.h"

static void buf_destructor(void *mem)
{
    struct buf *buf = (struct buf *)mem;
    memset(buf->d, 0, buf->len);
    list_unlink(&buf->le);
}

struct buf *buf_alloc(char *name, size_t size)
{
    struct buf *buf = kzref_alloc(name, sizeof *buf + size + 1, buf_destructor);
    if (!buf)
        return NULL;

    buf->d = (u8 *)(buf + 1);
    buf->len = size;
    buf->payload_len = 0;
    return buf;
}

struct buf *buf_strdub(const char *str)
{
    size_t len = strlen(str);
    struct buf *buf = buf_alloc("buf_strdub", len);
    if (!buf)
        return NULL;

    memcpy(buf->d, str, len);
    buf_put(buf, len);
    return buf;
}

struct buf *buf_strndub(const char *str, size_t len)
{
    struct buf *buf = buf_alloc("buf_strndub", len);
    if (!buf)
        return NULL;

    memcpy(buf->d, str, len);
    buf_put(buf, len);
    return buf;
}

struct buf *buf_concatenate(const struct buf *b1, const struct buf *b2)
{
    struct buf *result;

    if (!b1 || !b2)
        return NULL;

    const size_t s1 = buf_len(b1);
    const size_t s2 = buf_len(b2);
    if (!s1 || !s2)
        return NULL;

    result = buf_alloc("buf_concatenate", s1 + s2);
    if (!result)
        return NULL;

    memcpy(result->d, b1->d, s1);
    memcpy(result->d + s1, b2->d, s2);
    buf_put(result, s1 + s2);
    return result;
}


void _buf_dump(const struct buf *buf, const char *name)
{
    uint cnt = 0;
    uint row_cnt, row_len;

    printf("\r\n");
    if (name)
        printf("buf %s:\r\n", name);

    if (!buf) {
        printf("buf is NULL\r\n");
        return;
    }
    const size_t len = buf_len(buf);
    printf("len: %u, payload_len: %u\r\n", buf->len, buf->payload_len);

    while(cnt < len) {
        printf("%.4x - ", cnt);
        row_len = (cnt + 16) < len ? 16 : (len - cnt);
        for (row_cnt = 0; row_cnt < 16; row_cnt++) {
            if (row_cnt < row_len)
                printf("%.2x ", buf->d[cnt + row_cnt]);
            else
                printf("   ");
            if (row_cnt == 7)
                printf(": ");
        }
        printf("| ");
        for (row_cnt = 0; row_cnt < row_len; row_cnt++) {
            u8 b = buf->d[cnt + row_cnt];
            if (isprint(b))
                printf("%c", b);
            else
                printf(".");
        }
        cnt += row_len;
        printf("\r\n");
    }
    fflush(stdout);
}

void _buf_list_dump(const struct list *list, const char *list_name)
{
    struct le *le;
    struct buf *buf;
    char buf_name[16];
    unsigned cnt = 0;
    unsigned numbers;

    printf("\r\n");
    if (list_name)
        printf("list %s:\r\n", list_name);
    if (!list) {
        printf("list is NULL\r\n");
        return;
    }

    numbers = list_count(list);

    if (!numbers)
        return;
    printf("buffers in list: %d\r\n", numbers);

    printf("---");
    LIST_FOREACH(list, le) {
        buf = (struct buf *)list_ledata(le);
        sprintf(buf_name, "%d", cnt);
        _buf_dump(buf, buf_name);
        cnt ++;
    }
    printf("---\r\n");
    fflush(stdout);
}


struct buf *buf_cpy(const void *src, size_t len)
{
    struct buf *buf = buf_alloc("buf_cpy", len);
    if (!buf)
        return NULL;

    memcpy(buf->d, src, len);
    buf_put(buf, len);
    return buf;
}


char *buf_strcpy(struct buf *buf)
{
    char *str;
    if (!buf)
        return NULL;

    const size_t len = buf_len(buf);
    if (len == buf->len && !buf->d[buf->len - 1])
        return (char *)buf->d;

    str = (char *)kref_alloc("buf_strcpy", buf->len + 1, NULL);
    if (!str)
        return NULL;
    memcpy(str, buf->d, len);
    str[len] = 0;
    return str;
}

void buf_put(struct buf *buf, size_t payload_len)
{
    if (payload_len > buf->len)
        return;
    buf->payload_len = payload_len;
    buf->d[buf->payload_len] = 0;
}

struct list *buf_split(const struct buf *buf, char sep)
{
    const size_t len = buf_len(buf);
    size_t part_len = 0;
    struct list *list;
    struct buf *part_buf;
    uint i;

    list = list_create("buf_split_list");
    if (!list)
        goto err;

    const u8 *part = buf->d;
    for (i = 0; i < len; i++) {
        const u8 *p = buf->d + i;
        if (*p != sep) {
            part_len ++;
            continue;
        }

        part_buf = buf_cpy((void *)part, part_len);
        kref_set_name(part_buf, "buf_split");
        if (!part_buf)
            goto err;

        buf_list_append(list, part_buf);
        part_len = 0;
        part = p + 1;
    }

    if (part_len) {
        part_buf = buf_cpy((void *)part, part_len);
        if (!part_buf)
            goto err;
        buf_list_append(list, part_buf);
    }

    return list;
err:
    kmem_deref(&list);
    return list;
}

struct buf *buf_trim(const struct buf *buf)
{
    const size_t len = buf_len(buf);
    size_t new_len;
    struct buf *new_buf;
    const u8 *start = buf->d;
    const u8 *back;

    if (!len)
        return (struct buf *)buf;

    while(
        (start < (buf->d + buf_len(buf)))
        && isspace(*start)
        && *start != 0
    )
        start++;
    if (start == (buf->d + buf_len(buf)))
        return buf_alloc("buf_trim", 0);

    back = buf->d + len;
    while(isspace(*--back) && back != start);
    new_len = back - start + 1;
    if (new_len <= 0)
        return buf_alloc("buf_trim", 0);

    new_buf = buf_cpy(start, new_len);
    kref_set_name(new_buf, "buf_trim");
    if (!new_buf)
        return NULL;
    buf_put(new_buf, new_len);
    return new_buf;
}

struct buf *buf_sprintf(const char* format, ...)
{
    size_t len;
    struct buf *buf;
    va_list args;
    va_start(args, format);
    len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    buf = buf_alloc("buf_sprintf", len + 1);
    if (!buf)
        return NULL;

    va_start(args, format);
    vsprintf((char *)buf->d, format, args);
    va_end(args);

    buf_put(buf, len);
    return buf;
}

void buf_erase(struct buf *buf)
{
    memset(buf->d, 0, buf->len);
    buf_put(buf, 0);
}

struct buf *buf_list_join(const struct list *list, char sep)
{
    struct buf *buf = NULL;
    struct le *le;
    size_t buf_size = 0;
    u8 *data = NULL;

    if (!list)
        return NULL;

    int sep_cnt = list_count(list) - 1;
    if (sep_cnt < 0)
        return NULL;

    LIST_FOREACH(list, le) {
        struct buf *item = list_ledata(le);
        buf_size += buf_len(item);
    }
    if (sep)
        buf_size += sep_cnt;

    buf = buf_alloc("buf_list_join", buf_size);
    if (!buf)
        return NULL;

    data = buf->d;

    LIST_FOREACH(list, le) {
        struct buf *item = list_ledata(le);
        size_t item_size = buf_len(item);
        memcpy(data, item->d, item_size);
        data += item_size;
        if (!sep || !sep_cnt)
            continue;

        *data = sep;
        data++;
        sep_cnt--;
    }
    buf_put(buf, buf_size);

    return buf;
}

int buf_cmp(const struct buf *b1, const struct buf *b2)
{
    const size_t l1 = buf_len(b1);
    const size_t l2 = buf_len(b2);
    if (l1 != l2)
        return l1 - l2;

    return memcmp(b1->d, b2->d, l1);
}

void buf_memset(const struct buf *buf, int val)
{
    memset(buf->d, val, buf->len);
}
