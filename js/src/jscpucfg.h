





#ifndef jscpucfg_h
#define jscpucfg_h

#define JS_HAVE_LONG_LONG

#if defined(_WIN64)

# if defined(_M_X64) || defined(_M_AMD64) || defined(_AMD64_)
#  define IS_LITTLE_ENDIAN 1
#  undef  IS_BIG_ENDIAN
# else  
#  error "CPU type is unknown"
# endif 

#elif defined(_WIN32)

# ifdef __WATCOMC__
#  define HAVE_VA_LIST_AS_ARRAY 1
# endif

# define IS_LITTLE_ENDIAN 1
# undef  IS_BIG_ENDIAN

#elif defined(__APPLE__) || defined(__powerpc__) || defined(__ppc__)
# if __LITTLE_ENDIAN__
#  define IS_LITTLE_ENDIAN 1
#  undef  IS_BIG_ENDIAN
# elif __BIG_ENDIAN__
#  undef  IS_LITTLE_ENDIAN
#  define IS_BIG_ENDIAN 1
# endif

#elif defined(JS_HAVE_ENDIAN_H)
# include <endian.h>










# if defined(__BYTE_ORDER) || defined(BYTE_ORDER)
#  if defined(__BYTE_ORDER)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#    define IS_LITTLE_ENDIAN 1
#    undef  IS_BIG_ENDIAN
#   elif __BYTE_ORDER == __BIG_ENDIAN
#    undef  IS_LITTLE_ENDIAN
#    define IS_BIG_ENDIAN 1
#   endif
#  endif
#  if defined(BYTE_ORDER)
#   if BYTE_ORDER == LITTLE_ENDIAN
#    define IS_LITTLE_ENDIAN 1
#    undef  IS_BIG_ENDIAN
#   elif BYTE_ORDER == BIG_ENDIAN
#    undef  IS_LITTLE_ENDIAN
#    define IS_BIG_ENDIAN 1
#   endif
#  endif
# else 
#  error "endian.h does not define __BYTE_ORDER nor BYTE_ORDER. Cannot determine endianness."
# endif


#elif defined(JS_HAVE_MACHINE_ENDIAN_H)
# include <sys/types.h>
# include <machine/endian.h>

# if defined(_BYTE_ORDER)
#  if _BYTE_ORDER == _LITTLE_ENDIAN
#   define IS_LITTLE_ENDIAN 1
#   undef  IS_BIG_ENDIAN
#  elif _BYTE_ORDER == _BIG_ENDIAN
#   undef  IS_LITTLE_ENDIAN
#   define IS_BIG_ENDIAN 1
#  endif
# else 
#  error "machine/endian.h does not define _BYTE_ORDER. Cannot determine endianness."
# endif

#elif defined(JS_HAVE_SYS_ISA_DEFS_H)
# include <sys/isa_defs.h>

# if defined(_BIG_ENDIAN)
#  undef IS_LITTLE_ENDIAN
#  define IS_BIG_ENDIAN 1
# elif defined(_LITTLE_ENDIAN)
#  define IS_LITTLE_ENDIAN 1
#  undef IS_BIG_ENDIAN
# else 
#  error "sys/isa_defs.h does not define _BIG_ENDIAN or _LITTLE_ENDIAN. Cannot determine endianness."
# endif
# if !defined(JS_STACK_GROWTH_DIRECTION)
#  if defined(_STACK_GROWS_UPWARD)
#   define JS_STACK_GROWTH_DIRECTION (1)
#  elif defined(_STACK_GROWS_DOWNWARD)
#   define JS_STACK_GROWTH_DIRECTION (-1)
#  endif
# endif

#elif defined(__sparc) || defined(__sparc__) || \
      defined(_POWER) || defined(__hppa) || \
      defined(_MIPSEB) || defined(_BIG_ENDIAN)



# undef IS_LITTLE_ENDIAN
# define IS_BIG_ENDIAN 1

#else 
# error "Cannot determine endianness of your platform. Please add support to jscpucfg.h."
#endif

#ifndef JS_STACK_GROWTH_DIRECTION
# ifdef __hppa
#  define JS_STACK_GROWTH_DIRECTION (1)
# else
#  define JS_STACK_GROWTH_DIRECTION (-1)
# endif
#endif

#endif 
