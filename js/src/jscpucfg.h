






































#ifndef js_cpucfg___
#define js_cpucfg___

#define JS_HAVE_LONG_LONG

#if defined(XP_WIN) || defined(XP_OS2)

#if defined(_WIN64)

#if defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   8L
#define JS_BITS_PER_WORD_LOG2   6
#define JS_ALIGN_OF_POINTER 8L
#else  
#error "CPU type is unknown"
#endif 

#elif defined(_WIN32) || defined(XP_OS2)

#ifdef __WATCOMC__
#define HAVE_VA_LIST_AS_ARRAY 1
#endif

#define IS_LITTLE_ENDIAN 1
#undef  IS_BIG_ENDIAN
#define JS_BYTES_PER_DOUBLE 8L
#define JS_BYTES_PER_WORD   4L
#define JS_BITS_PER_WORD_LOG2   5
#define JS_ALIGN_OF_POINTER 4L

#endif 

#elif defined(XP_UNIX)

#error "This file is supposed to be auto-generated on UNIX platforms, but the"
#error "static version for Mac and Windows platforms is being used."
#error "Something's probably wrong with paths/headers/dependencies/Makefiles."

#else

#error "Must define one of XP_OS2, XP_WIN, or XP_UNIX"

#endif

#ifndef JS_STACK_GROWTH_DIRECTION
#define JS_STACK_GROWTH_DIRECTION (-1)
#endif

#endif 
