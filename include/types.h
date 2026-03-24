/*
 * Duke Nukem 3D - Nintendo DS Port
 * types.h - Core type definitions
 */

#ifndef DUKE3D_TYPES_H
#define DUKE3D_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Standard integer types for the Build engine */
typedef int8_t      int8;
typedef uint8_t     uint8;
typedef int16_t     int16;
typedef uint16_t    uint16;
typedef int32_t     int32;
typedef uint32_t    uint32;
typedef int64_t     int64;
typedef uint64_t    uint64;

typedef uint8_t     byte;
typedef int32_t     BOOL;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define MAXLONG    0x7fffffff

/* Fixed-point math */
typedef int32_t     fixed_t;

#define FRACBITS   16
#define FRACUNIT   (1 << FRACBITS)

static inline fixed_t FixedMul(fixed_t a, fixed_t b)
{
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FRACBITS);
}

static inline fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((abs(a) >> 14) >= abs(b))
        return (a ^ b) < 0 ? -MAXLONG : MAXLONG;
    return (fixed_t)(((int64_t)a << FRACBITS) / b);
}

#endif /* DUKE3D_TYPES_H */
