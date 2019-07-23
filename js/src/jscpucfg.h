






































#ifndef js_cpucfg___
#define js_cpucfg___

#include "jsosdep.h"

#if defined(XP_WIN) || defined(XP_OS2) || defined(WINCE)

#if defined(_WIN64)

#if defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define JS_BYTES_PER_BYTE   1L
#define JS_BYTES_PER_SHORT  2L
#define JS_BYTES_PER_INT    4L
#define JS_BYTES_PER_INT64  8L
#define JS_BYTES_PER_LONG   4L
#define JS_BYTES_PER_FLOAT  4L
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   8L
#define JS_BYTES_PER_DWORD  8L

#define JS_BITS_PER_BYTE    8L
#define JS_BITS_PER_SHORT   16L
#define JS_BITS_PER_INT     32L
#define JS_BITS_PER_INT64   64L
#define JS_BITS_PER_LONG    32L
#define JS_BITS_PER_FLOAT   32L
#define JS_BITS_PER_DOUBLE  64L
#define JS_BITS_PER_WORD    64L

#define JS_BITS_PER_BYTE_LOG2   3L
#define JS_BITS_PER_SHORT_LOG2  4L
#define JS_BITS_PER_INT_LOG2    5L
#define JS_BITS_PER_INT64_LOG2  6L
#define JS_BITS_PER_LONG_LOG2   5L
#define JS_BITS_PER_FLOAT_LOG2  5L
#define JS_BITS_PER_DOUBLE_LOG2 6L
#define JS_BITS_PER_WORD_LOG2   6L

#define JS_ALIGN_OF_SHORT   2L
#define JS_ALIGN_OF_INT     4L
#define JS_ALIGN_OF_LONG    4L
#define JS_ALIGN_OF_INT64   8L
#define JS_ALIGN_OF_FLOAT   4L
#define JS_ALIGN_OF_DOUBLE  8L
#define JS_ALIGN_OF_POINTER 8L
#define JS_ALIGN_OF_WORD    8L

#define JS_BYTES_PER_WORD_LOG2   3L
#define JS_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  0L
#else  
#error "CPU type is unknown"
#endif 

#elif defined(_WIN32) || defined(XP_OS2) || defined(WINCE)

#ifdef __WATCOMC__
#define HAVE_VA_LIST_AS_ARRAY
#endif

#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define JS_BYTES_PER_BYTE   1L
#define JS_BYTES_PER_SHORT  2L
#define JS_BYTES_PER_INT    4L
#define JS_BYTES_PER_INT64  8L
#define JS_BYTES_PER_LONG   4L
#define JS_BYTES_PER_FLOAT  4L
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   4L
#define JS_BYTES_PER_DWORD  8L

#define JS_BITS_PER_BYTE    8L
#define JS_BITS_PER_SHORT   16L
#define JS_BITS_PER_INT     32L
#define JS_BITS_PER_INT64   64L
#define JS_BITS_PER_LONG    32L
#define JS_BITS_PER_FLOAT   32L
#define JS_BITS_PER_DOUBLE  64L
#define JS_BITS_PER_WORD    32L

#define JS_BITS_PER_BYTE_LOG2   3L
#define JS_BITS_PER_SHORT_LOG2  4L
#define JS_BITS_PER_INT_LOG2    5L
#define JS_BITS_PER_INT64_LOG2  6L
#define JS_BITS_PER_LONG_LOG2   5L
#define JS_BITS_PER_FLOAT_LOG2  5L
#define JS_BITS_PER_DOUBLE_LOG2 6L
#define JS_BITS_PER_WORD_LOG2   5L

#define JS_ALIGN_OF_SHORT   2L
#define JS_ALIGN_OF_INT     4L
#define JS_ALIGN_OF_LONG    4L
#define JS_ALIGN_OF_INT64   8L
#define JS_ALIGN_OF_FLOAT   4L
#define JS_ALIGN_OF_DOUBLE  4L
#define JS_ALIGN_OF_POINTER 4L
#define JS_ALIGN_OF_WORD    4L

#define JS_BYTES_PER_WORD_LOG2   2L
#define JS_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L
#endif 

#if defined(_WINDOWS) && !defined(_WIN32) 

#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN

#define JS_BYTES_PER_BYTE   1L
#define JS_BYTES_PER_SHORT  2L
#define JS_BYTES_PER_INT    2L
#define JS_BYTES_PER_INT64  8L
#define JS_BYTES_PER_LONG   4L
#define JS_BYTES_PER_FLOAT  4L
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   4L
#define JS_BYTES_PER_DWORD  8L

#define JS_BITS_PER_BYTE    8L
#define JS_BITS_PER_SHORT   16L
#define JS_BITS_PER_INT     16L
#define JS_BITS_PER_INT64   64L
#define JS_BITS_PER_LONG    32L
#define JS_BITS_PER_FLOAT   32L
#define JS_BITS_PER_DOUBLE  64L
#define JS_BITS_PER_WORD    32L

#define JS_BITS_PER_BYTE_LOG2   3L
#define JS_BITS_PER_SHORT_LOG2  4L
#define JS_BITS_PER_INT_LOG2    4L
#define JS_BITS_PER_INT64_LOG2  6L
#define JS_BITS_PER_LONG_LOG2   5L
#define JS_BITS_PER_FLOAT_LOG2  5L
#define JS_BITS_PER_DOUBLE_LOG2 6L
#define JS_BITS_PER_WORD_LOG2   5L

#define JS_ALIGN_OF_SHORT   2L
#define JS_ALIGN_OF_INT     2L
#define JS_ALIGN_OF_LONG    2L
#define JS_ALIGN_OF_INT64   2L
#define JS_ALIGN_OF_FLOAT   2L
#define JS_ALIGN_OF_DOUBLE  2L
#define JS_ALIGN_OF_POINTER 2L
#define JS_ALIGN_OF_WORD    2L

#define JS_BYTES_PER_WORD_LOG2   2L
#define JS_BYTES_PER_DWORD_LOG2  3L
#define PR_WORDS_PER_DWORD_LOG2  1L

#endif 

#elif defined(XP_UNIX) || defined(XP_BEOS)

#error "This file is supposed to be auto-generated on UNIX platforms, but the"
#error "static version for Mac and Windows platforms is being used."
#error "Something's probably wrong with paths/headers/dependencies/Makefiles."

#else

#error "Must define one of XP_BEOS, XP_OS2, XP_WIN, or XP_UNIX"

#endif

#ifndef JS_STACK_GROWTH_DIRECTION
#define JS_STACK_GROWTH_DIRECTION (-1)
#endif

#endif 
