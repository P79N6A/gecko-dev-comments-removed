




















































#ifndef jstypes_h___
#define jstypes_h___

#include <stddef.h>
#include "jsstdint.h"





















#ifdef WIN32


# define JS_EXTERN_API(__type)  extern __declspec(dllexport) __type
# define JS_EXPORT_API(__type)  __declspec(dllexport) __type
# define JS_EXTERN_DATA(__type) extern __declspec(dllexport) __type
# define JS_EXPORT_DATA(__type) __declspec(dllexport) __type

#elif defined(XP_OS2) && defined(__declspec)

# define JS_EXTERN_API(__type)  extern __declspec(dllexport) __type
# define JS_EXPORT_API(__type)  __declspec(dllexport) __type
# define JS_EXTERN_DATA(__type) extern __declspec(dllexport) __type
# define JS_EXPORT_DATA(__type) __declspec(dllexport) __type

#else 

# ifdef HAVE_VISIBILITY_ATTRIBUTE
#  define JS_EXTERNAL_VIS __attribute__((visibility ("default")))
# elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#  define JS_EXTERNAL_VIS __global
# else
#  define JS_EXTERNAL_VIS
# endif

# define JS_EXTERN_API(__type)  extern JS_EXTERNAL_VIS __type
# define JS_EXPORT_API(__type)  JS_EXTERNAL_VIS __type
# define JS_EXTERN_DATA(__type) extern JS_EXTERNAL_VIS __type
# define JS_EXPORT_DATA(__type) JS_EXTERNAL_VIS __type

#endif

#ifdef _WIN32
# if defined(__MWERKS__) || defined(__GNUC__)
#  define JS_IMPORT_API(__x)    __x
# else
#  define JS_IMPORT_API(__x)    __declspec(dllimport) __x
# endif
#elif defined(XP_OS2) && defined(__declspec)
# define JS_IMPORT_API(__x)     __declspec(dllimport) __x
#else
# define JS_IMPORT_API(__x)     JS_EXPORT_API (__x)
#endif

#if defined(_WIN32) && !defined(__MWERKS__)
# define JS_IMPORT_DATA(__x)      __declspec(dllimport) __x
#elif defined(XP_OS2) && defined(__declspec)
# define JS_IMPORT_DATA(__x)      __declspec(dllimport) __x
#else
# define JS_IMPORT_DATA(__x)     JS_EXPORT_DATA (__x)
#endif







#if defined(STATIC_JS_API)

# define JS_PUBLIC_API(t)   t
# define JS_PUBLIC_DATA(t)  t

#elif defined(EXPORT_JS_API)

# define JS_PUBLIC_API(t)   JS_EXPORT_API(t)
# define JS_PUBLIC_DATA(t)  JS_EXPORT_DATA(t)

#else

# define JS_PUBLIC_API(t)   JS_IMPORT_API(t)
# define JS_PUBLIC_DATA(t)  JS_IMPORT_DATA(t)

#endif

#define JS_FRIEND_API(t)    JS_PUBLIC_API(t)
#define JS_FRIEND_DATA(t)   JS_PUBLIC_DATA(t)

#if defined(_MSC_VER) && defined(_M_IX86)
#define JS_FASTCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__) &&                         \
  ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define JS_FASTCALL __attribute__((fastcall))
#else
#define JS_FASTCALL
#define JS_NO_FASTCALL
#endif

#ifndef JS_INLINE
# if defined __cplusplus
#  define JS_INLINE          inline
# elif defined _MSC_VER
#  define JS_INLINE          __inline
# elif defined __GNUC__
#  define JS_INLINE          __inline__
# else
#  define JS_INLINE          inline
# endif
#endif

#ifndef JS_ALWAYS_INLINE
# if defined DEBUG
#  define JS_ALWAYS_INLINE   JS_INLINE
# elif defined _MSC_VER
#  define JS_ALWAYS_INLINE   __forceinline
# elif defined __GNUC__
#  define JS_ALWAYS_INLINE   __attribute__((always_inline)) JS_INLINE
# else
#  define JS_ALWAYS_INLINE   JS_INLINE
# endif
#endif

#ifdef NS_STATIC_CHECKING






# define JS_REQUIRES_STACK   __attribute__((user("JS_REQUIRES_STACK")))
# define JS_FORCES_STACK     __attribute__((user("JS_FORCES_STACK")))
#else
# define JS_REQUIRES_STACK
# define JS_FORCES_STACK
#endif








#define JS_BEGIN_MACRO  do {

#if defined(_MSC_VER) && _MSC_VER >= 1400
# define JS_END_MACRO                                                         \
    } __pragma(warning(push)) __pragma(warning(disable:4127))                 \
    while (0) __pragma(warning(pop))
#else
# define JS_END_MACRO   } while (0)
#endif







#ifdef __cplusplus

# define JS_BEGIN_EXTERN_C      extern "C" {
# define JS_END_EXTERN_C        }

#else

# define JS_BEGIN_EXTERN_C
# define JS_END_EXTERN_C

#endif







#define JS_BIT(n)       ((JSUint32)1 << (n))
#define JS_BITMASK(n)   (JS_BIT(n) - 1)









#define JS_PTR_TO_INT32(x)  ((jsint)((char *)(x) - (char *)0))
#define JS_PTR_TO_UINT32(x) ((jsuint)((char *)(x) - (char *)0))
#define JS_INT32_TO_PTR(x)  ((void *)((char *)0 + (jsint)(x)))
#define JS_UINT32_TO_PTR(x) ((void *)((char *)0 + (jsuint)(x)))









#define JS_HOWMANY(x,y) (((x)+(y)-1)/(y))
#define JS_ROUNDUP(x,y) (JS_HOWMANY(x,y)*(y))
#define JS_MIN(x,y)     ((x)<(y)?(x):(y))
#define JS_MAX(x,y)     ((x)>(y)?(x):(y))

#ifdef _MSC_VER
# include "jscpucfg.h"  
#else
# include "jsautocfg.h" 
#endif

JS_BEGIN_EXTERN_C









typedef uint8_t JSUint8;
typedef int8_t JSInt8;








typedef uint16_t JSUint16;
typedef int16_t JSInt16;








typedef uint32_t JSUint32;
typedef int32_t JSInt32;












typedef uint64_t JSUint64;
typedef int64_t JSInt64;











typedef int JSIntn;
typedef unsigned int JSUintn;






typedef double          JSFloat64;






typedef size_t JSSize;







typedef ptrdiff_t JSPtrdiff;







typedef uintptr_t JSUptrdiff;









typedef JSIntn JSBool;
#define JS_TRUE (JSIntn)1
#define JS_FALSE (JSIntn)0







typedef JSUint8 JSPackedBool;




typedef intptr_t JSWord;
typedef uintptr_t JSUword;

#include "jsotypes.h"

















#if defined(__GNUC__) && (__GNUC__ > 2)

# define JS_LIKELY(x)   (__builtin_expect((x), 1))
# define JS_UNLIKELY(x) (__builtin_expect((x), 0))

#else

# define JS_LIKELY(x)   (x)
# define JS_UNLIKELY(x) (x)

#endif


















#define JS_ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])
#define JS_ARRAY_END(array)    ((array) + JS_ARRAY_LENGTH(array))

#define JS_BITS_PER_BYTE 8
#define JS_BITS_PER_BYTE_LOG2 3

#define JS_BITS_PER_WORD (JS_BITS_PER_BYTE * JS_BYTES_PER_WORD)
#define JS_BITS_PER_DOUBLE (JS_BITS_PER_BYTE * JS_BYTES_PER_DOUBLE)

JS_END_EXTERN_C

#endif 
