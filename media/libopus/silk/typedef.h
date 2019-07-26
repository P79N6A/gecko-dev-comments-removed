






























#ifndef SILK_TYPEDEF_H
#define SILK_TYPEDEF_H

#include "opus_types.h"

#ifndef silk_USE_DOUBLE_PRECISION_FLOATS
#define silk_USE_DOUBLE_PRECISION_FLOATS     0
#endif

#include <float.h>
#if defined( __GNUC__ )
#include <stdint.h>
#endif

#define silk_int_ptr_size intptr_t

#if silk_USE_DOUBLE_PRECISION_FLOATS
# define silk_float      double
# define silk_float_MAX  DBL_MAX
#else
# define silk_float      float
# define silk_float_MAX  FLT_MAX
#endif

#ifdef _WIN32
# define silk_STR_CASEINSENSITIVE_COMPARE(x, y) _stricmp(x, y)
#else
# define silk_STR_CASEINSENSITIVE_COMPARE(x, y) strcasecmp(x, y)
#endif

#define silk_int64_MAX   ((opus_int64)0x7FFFFFFFFFFFFFFFLL)   /*  2^63 - 1 */
#define silk_int64_MIN   ((opus_int64)0x8000000000000000LL)   /* -2^63 */
#define silk_int32_MAX   0x7FFFFFFF                           /*  2^31 - 1 =  2147483647 */
#define silk_int32_MIN   ((opus_int32)0x80000000)             /* -2^31     = -2147483648 */
#define silk_int16_MAX   0x7FFF                               /*  2^15 - 1 =  32767 */
#define silk_int16_MIN   ((opus_int16)0x8000)                 /* -2^15     = -32768 */
#define silk_int8_MAX    0x7F                                 /*  2^7 - 1  =  127 */
#define silk_int8_MIN    ((opus_int8)0x80)                    /* -2^7      = -128 */

#define silk_uint32_MAX  0xFFFFFFFF  /* 2^32 - 1 = 4294967295 */
#define silk_uint32_MIN  0x00000000
#define silk_uint16_MAX  0xFFFF      /* 2^16 - 1 = 65535 */
#define silk_uint16_MIN  0x0000
#define silk_uint8_MAX   0xFF        /*  2^8 - 1 = 255 */
#define silk_uint8_MIN   0x00

#define silk_TRUE        1
#define silk_FALSE       0


#if (defined _WIN32 && !defined _WINCE && !defined(__GNUC__) && !defined(NO_ASSERTS))
# ifndef silk_assert
#  include <crtdbg.h>      
#  define silk_assert(COND)   _ASSERTE(COND)
# endif
#else
# ifdef ENABLE_ASSERTIONS
#  include <stdio.h>
#  include <stdlib.h>
#define silk_fatal(str) _silk_fatal(str, __FILE__, __LINE__);
#ifdef __GNUC__
__attribute__((noreturn))
#endif
static inline void _silk_fatal(const char *str, const char *file, int line)
{
   fprintf (stderr, "Fatal (internal) error in %s, line %d: %s\n", file, line, str);
   abort();
}
#  define silk_assert(COND) {if (!(COND)) {silk_fatal("assertion failed: " #COND);}}
# else
#  define silk_assert(COND)
# endif
#endif

#endif 
