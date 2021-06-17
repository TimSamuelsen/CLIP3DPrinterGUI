/*
* Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
* ALL RIGHTS RESERVED
*
*/
#ifndef COMMON_H_
#define COMMON_H_

/**
 * This module provides the common defines used by all the modules
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#ifdef __cplusplus
extern "C" {
#endif

#define TRUE                    1
#define FALSE                   0


/** Minimum of two numbers */
#define MIN(a, b)				((a) < (b) ? (a) : (b))
/** Maximum of two numbers */
#define MAX(a, b)				((a) > (b) ? (a) : (b))
/** Align the value 'x' to the closest 'b' multiple (smaller than 'x'). 
 * 'b' should be power of 2 */
#define ALIGN_BYTES_PREV(x, b)  ((x) & ~(uint32)((b) - 1))

/** Align the value 'x' to the closest 'b' multiple (greater than 'x'). 
 * 'b' should be power of 2 */
#define ALIGN_BYTES_NEXT(x, b)  (((x) + ((b)-1)) & ~(uint32)((b) - 1))


/** Get the nth byte of a 32 bit number */
#define BYTE0(x)				(uint8)(x)
#define BYTE1(x)				(uint8)((x) >> 8)
#define BYTE2(x)				(uint8)((x) >> 16)
#define BYTE3(x)				(uint8)((x) >> 24)

/** Get the nth half-word of a 32 bit number */
#define WORD0(x)				(uint16)(x)
#define WORD1(x)				(uint16)((x) >> 16)

/** Make half-word from an array of 2 bytes (Little-endian) */
#define PARSE_WORD16_LE(Arr)    MAKE_WORD16((Arr)[1], (Arr)[0])
/** Make half-word from an array of 2 bytes (Big-endian) */
#define PARSE_WORD16_BE(Arr)    MAKE_WORD16((Arr)[0], (Arr)[1])

/** Make a word from an array of 3 bytes (Little-endian) */
#define PARSE_WORD24_LE(Arr)    MAKE_WORD32(0, (Arr)[2], (Arr)[1], (Arr)[0])
/** Make a word from an array of 3 bytes (Bit-endian) */
#define PARSE_WORD24_BE(Arr)    MAKE_WORD32(0, (Arr)[0], (Arr)[1], (Arr)[2])

/** Make a word from an array of 4 bytes (Little-endian) */
#define PARSE_WORD32_LE(Arr)    MAKE_WORD32((Arr)[3], (Arr)[2], \
		                                    (Arr)[1], (Arr)[0])
/** Make a word from an array of 4 bytes (Bit-endian) */
#define PARSE_WORD32_BE(Arr)    MAKE_WORD32((Arr)[0], (Arr)[1], \
		                                    (Arr)[2], (Arr)[3])

/** Make a half-word from two bytes */
#define MAKE_WORD16(b1, b0)         (((b1) << 8) | (b0))
/** Make word from four bytes */
#define MAKE_WORD32(b3, b2, b1, b0) (((uint32)(b3)<<24)|((uint32)(b2)<<16)|\
									 ((uint32)(b1)<< 8)|((uint32)(b0)    ) )

/** Get the number of elements in a given array */
#define ARRAY_SIZE(x)               (sizeof(x)/sizeof(*x))

/** Divide x/y and round it to the near by integer */
#define DIV_ROUND(x, y)             (((x)+(y)/2)/(y))

/** Divide x/y and round it to the next integer */
#define DIV_CEIL(x, y)              (((x)+(y)-1)/(y))

/** Find 2 is power 'x' */
#define POW_OF_2(x)                 (1ul << (x))

/** Check if 'x' is power of two */
#define IS_POW_OF_2(x)              (((x) & ((x)-1)) == 0)

/** Generate bit mask of n bits starting from s bit */
#define GEN_BIT_MASK(s, n)          ((0xFFFFFFFFUL >> (32 - (n))) << (s))

/** Merge bits 'b' into 'a' according to bits in 'mask' */
#define MERGE_BITS(a, b, mask)      ((a) ^ (((a) ^ (b)) & (mask)))

/** Initialize all the structure elements to 0 */
#define STRUCT_CLEAR(s)				(memset(&(s), 0, sizeof(s)))

/** Revers bits of a byte */
#define BIT_REVERSE(x)				CMN_BitRevLUT[(uint8)(x)]

/** Convert the given token sequence to a string */
#define STR(x)	STR_(x)
#define STR_(x)	#x

/** Data types of known size */
typedef int BOOL;
typedef unsigned int uint;
typedef unsigned uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long long uint64;
typedef uint8 uint08;
/* Bit masks. */
#define BIT0        0x01
#define BIT1        0x02
#define BIT2        0x04
#define BIT3        0x08
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
#define BIT8      0x0100
#define BIT9      0x0200
#define BIT10     0x0400
#define BIT11     0x0800
#define BIT12     0x1000
#define BIT13     0x2000
#define BIT14     0x4000
#define BIT15     0x8000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

#define KILO_BYTE                      1024
#define MEGA_BYTE                      (KILO_BYTE*KILO_BYTE)

/* Debug purposes */
#define IMG_DEBUG       0
#ifdef __cplusplus
}
#endif


#endif /* COMMON_H_ */
