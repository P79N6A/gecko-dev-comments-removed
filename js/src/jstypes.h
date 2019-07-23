




















































#ifndef jstypes_h___
#define jstypes_h___

#include <stddef.h>





















#ifdef WIN32

#define JS_EXTERN_API(__type) extern __declspec(dllexport) __type
#define JS_EXPORT_API(__type) __declspec(dllexport) __type
#define JS_EXTERN_DATA(__type) extern __declspec(dllexport) __type
#define JS_EXPORT_DATA(__type) __declspec(dllexport) __type

#define JS_DLL_CALLBACK
#define JS_STATIC_DLL_CALLBACK(__x) static __x

#elif defined(XP_OS2) && defined(__declspec)

#define JS_EXTERN_API(__type) extern __declspec(dllexport) __type
#define JS_EXPORT_API(__type) __declspec(dllexport) __type
#define JS_EXTERN_DATA(__type) extern __declspec(dllexport) __type
#define JS_EXPORT_DATA(__type) __declspec(dllexport) __type

#define JS_DLL_CALLBACK
#define JS_STATIC_DLL_CALLBACK(__x) static __x

#elif defined(WIN16)

#ifdef _WINDLL
#define JS_EXTERN_API(__type) extern __type _cdecl _export _loadds
#define JS_EXPORT_API(__type) __type _cdecl _export _loadds
#define JS_EXTERN_DATA(__type) extern __type _export
#define JS_EXPORT_DATA(__type) __type _export

#define JS_DLL_CALLBACK             __cdecl __loadds
#define JS_STATIC_DLL_CALLBACK(__x) static __x CALLBACK

#else 
#define JS_EXTERN_API(__type) extern __type _cdecl _export
#define JS_EXPORT_API(__type) __type _cdecl _export
#define JS_EXTERN_DATA(__type) extern __type _export
#define JS_EXPORT_DATA(__type) __type _export

#define JS_DLL_CALLBACK             __cdecl __loadds
#define JS_STATIC_DLL_CALLBACK(__x) __x JS_DLL_CALLBACK
#endif 

#else 

#ifdef HAVE_VISIBILITY_ATTRIBUTE
#define JS_EXTERNAL_VIS __attribute__((visibility ("default")))
#else
#define JS_EXTERNAL_VIS
#endif

#define JS_EXTERN_API(__type) extern JS_EXTERNAL_VIS __type
#define JS_EXPORT_API(__type) JS_EXTERNAL_VIS __type
#define JS_EXTERN_DATA(__type) extern JS_EXTERNAL_VIS __type
#define JS_EXPORT_DATA(__type) JS_EXTERNAL_VIS __type

#define JS_DLL_CALLBACK
#define JS_STATIC_DLL_CALLBACK(__x) static __x

#endif

#ifdef _WIN32
#  if defined(__MWERKS__) || defined(__GNUC__)
#    define JS_IMPORT_API(__x)      __x
#  else
#    define JS_IMPORT_API(__x)      __declspec(dllimport) __x
#  endif
#elif defined(XP_OS2) && defined(__declspec)
#    define JS_IMPORT_API(__x)      __declspec(dllimport) __x
#else
#    define JS_IMPORT_API(__x)      JS_EXPORT_API (__x)
#endif

#if defined(_WIN32) && !defined(__MWERKS__)
#    define JS_IMPORT_DATA(__x)      __declspec(dllimport) __x
#elif defined(XP_OS2) && defined(__declspec)
#    define JS_IMPORT_DATA(__x)      __declspec(dllimport) __x
#else
#    define JS_IMPORT_DATA(__x)     JS_EXPORT_DATA (__x)
#endif







#ifdef EXPORT_JS_API
#define JS_PUBLIC_API(t)    JS_EXPORT_API(t)
#define JS_PUBLIC_DATA(t)   JS_EXPORT_DATA(t)
#else
#define JS_PUBLIC_API(t)    JS_IMPORT_API(t)
#define JS_PUBLIC_DATA(t)   JS_IMPORT_DATA(t)
#endif

#define JS_FRIEND_API(t)    JS_PUBLIC_API(t)
#define JS_FRIEND_DATA(t)   JS_PUBLIC_DATA(t)

#ifdef _WIN32
#   define JS_INLINE __inline
#elif defined(__GNUC__)
#   define JS_INLINE
#else
#   define JS_INLINE
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
#define JS_BEGIN_EXTERN_C       extern "C" {
#define JS_END_EXTERN_C         }
#else
#define JS_BEGIN_EXTERN_C
#define JS_END_EXTERN_C
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

#if (defined(XP_WIN) && !defined(CROSS_COMPILE)) || defined (WINCE)
#    include "jscpucfg.h"        
#elif defined(XP_UNIX) || defined(XP_BEOS) || defined(XP_OS2) || defined(CROSS_COMPILE)
#    include "jsautocfg.h"       
#    include "jsosdep.h"         
#else
#    error "Must define one of XP_BEOS, XP_OS2, XP_WIN or XP_UNIX"
#endif

JS_BEGIN_EXTERN_C








#if JS_BYTES_PER_BYTE == 1
typedef unsigned char JSUint8;
typedef signed char JSInt8;
#else
#error No suitable type for JSInt8/JSUint8
#endif







#if JS_BYTES_PER_SHORT == 2
typedef unsigned short JSUint16;
typedef short JSInt16;
#else
#error No suitable type for JSInt16/JSUint16
#endif







#if JS_BYTES_PER_INT == 4
typedef unsigned int JSUint32;
typedef int JSInt32;
#define JS_INT32(x)  x
#define JS_UINT32(x) x ## U
#elif JS_BYTES_PER_LONG == 4
typedef unsigned long JSUint32;
typedef long JSInt32;
#define JS_INT32(x)  x ## L
#define JS_UINT32(x) x ## UL
#else
#error No suitable type for JSInt32/JSUint32
#endif











#ifdef JS_HAVE_LONG_LONG
#if JS_BYTES_PER_LONG == 8
typedef long JSInt64;
typedef unsigned long JSUint64;
#elif defined(WIN16)
typedef __int64 JSInt64;
typedef unsigned __int64 JSUint64;
#elif defined(WIN32) && !defined(__GNUC__)
typedef __int64  JSInt64;
typedef unsigned __int64 JSUint64;
#else
typedef long long JSInt64;
typedef unsigned long long JSUint64;
#endif 
#else  
typedef struct {
#ifdef IS_LITTLE_ENDIAN
    JSUint32 lo, hi;
#else
    JSUint32 hi, lo;
#endif
} JSInt64;
typedef JSInt64 JSUint64;
#endif 










#if JS_BYTES_PER_INT >= 2
typedef int JSIntn;
typedef unsigned int JSUintn;
#else
#error 'sizeof(int)' not sufficient for platform use
#endif






typedef double          JSFloat64;






typedef size_t JSSize;







typedef ptrdiff_t JSPtrdiff;







#if JS_BYTES_PER_WORD == 8 && JS_BYTES_PER_LONG != 8
typedef JSUint64 JSUptrdiff;
#else
typedef unsigned long JSUptrdiff;
#endif









typedef JSIntn JSBool;
#define JS_TRUE (JSIntn)1
#define JS_FALSE (JSIntn)0







typedef JSUint8 JSPackedBool;




#if JS_BYTES_PER_WORD == 8 && JS_BYTES_PER_LONG != 8
typedef JSInt64 JSWord;
typedef JSUint64 JSUword;
#else
typedef long JSWord;
typedef unsigned long JSUword;
#endif

#include "jsotypes.h"

















#if defined(__GNUC__) && (__GNUC__ > 2)
#define JS_LIKELY(x)    (__builtin_expect((x), 1))
#define JS_UNLIKELY(x)  (__builtin_expect((x), 0))
#else
#define JS_LIKELY(x)    (x)
#define JS_UNLIKELY(x)  (x)
#endif


















#define JS_ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])
#define JS_ARRAY_END(array)    ((array) + JS_ARRAY_LENGTH(array))

JS_END_EXTERN_C

#endif 
