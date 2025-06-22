#ifndef LIBKMEM_TYPES_H_
#define LIBKMEM_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRUE  1
#define FALSE 0

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u8 byte;
typedef unsigned int uint;
typedef unsigned long ulong;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        typeof( ((type *)0)->member ) *__mptr = (ptr);  \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif
