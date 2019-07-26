


















#ifndef jstypes_h___
#define jstypes_h___

#include "mozilla/Attributes.h"
#include "mozilla/Util.h"

#include "js-config.h"






















#define JS_EXTERN_API(type)  extern MOZ_EXPORT type
#define JS_EXPORT_API(type)  MOZ_EXPORT type
#define JS_EXPORT_DATA(type) MOZ_EXPORT type
#define JS_IMPORT_API(type)  MOZ_IMPORT_API type
#define JS_IMPORT_DATA(type) MOZ_IMPORT_DATA type







#if defined(STATIC_JS_API)
#  define JS_PUBLIC_API(t)   t
#  define JS_PUBLIC_DATA(t)  t
#elif defined(EXPORT_JS_API) || defined(STATIC_EXPORTABLE_JS_API)
#  define JS_PUBLIC_API(t)   MOZ_EXPORT t
#  define JS_PUBLIC_DATA(t)  MOZ_EXPORT t
#else
#  define JS_PUBLIC_API(t)   MOZ_IMPORT_API t
#  define JS_PUBLIC_DATA(t)  MOZ_IMPORT_DATA t
#endif

#define JS_FRIEND_API(t)    JS_PUBLIC_API(t)
#define JS_FRIEND_DATA(t)   JS_PUBLIC_DATA(t)

#if defined(_MSC_VER) && defined(_M_IX86)
#define JS_FASTCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__)
#define JS_FASTCALL __attribute__((fastcall))
#else
#define JS_FASTCALL
#define JS_NO_FASTCALL
#endif

#ifndef JS_INLINE
#define JS_INLINE MOZ_INLINE
#endif

#ifndef JS_ALWAYS_INLINE
#define JS_ALWAYS_INLINE MOZ_ALWAYS_INLINE
#endif

#ifndef JS_NEVER_INLINE
#define JS_NEVER_INLINE MOZ_NEVER_INLINE
#endif

#ifndef JS_WARN_UNUSED_RESULT
# if defined __GNUC__
#  define JS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
# else
#  define JS_WARN_UNUSED_RESULT
# endif
#endif








#define JS_BEGIN_MACRO  do {

#if defined(_MSC_VER) && _MSC_VER >= 1400
# define JS_END_MACRO                                                         \
    } __pragma(warning(push)) __pragma(warning(disable:4127))                 \
    while (0) __pragma(warning(pop))
#else
# define JS_END_MACRO   } while (0)
#endif







#define JS_BEGIN_EXTERN_C      MOZ_BEGIN_EXTERN_C
#define JS_END_EXTERN_C        MOZ_END_EXTERN_C







#define JS_BIT(n)       ((uint32_t)1 << (n))
#define JS_BITMASK(n)   (JS_BIT(n) - 1)









#define JS_HOWMANY(x,y) (((x)+(y)-1)/(y))
#define JS_ROUNDUP(x,y) (JS_HOWMANY(x,y)*(y))

#include "jscpucfg.h"





#ifdef _MSC_VER
# if defined(_M_X64) || defined(_M_AMD64)
#  define JS_64BIT
# endif
#elif defined(__GNUC__)

# if defined(__x86_64__) || defined(__sparcv9) || \
        defined(__64BIT__) || defined(__LP64__)
#  define JS_64BIT
# endif
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) 
# if defined(__x86_64) || defined(__sparcv9)
#  define JS_64BIT
# endif
#elif defined(__xlc__) || defined(__xlC__)        
# if defined(__64BIT__)
#  define JS_64BIT
# endif
#elif defined(__HP_cc) || defined(__HP_aCC)       
# if defined(__LP64__)
#  define JS_64BIT
# endif
#else
# error "Implement me"
#endif










typedef int JSBool;
#define JS_TRUE (int)1
#define JS_FALSE (int)0

















#ifdef __GNUC__

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

















#ifdef __GNUC__
# define JS_FUNC_TO_DATA_PTR(type, fun) (__extension__ (type) (size_t) (fun))
# define JS_DATA_TO_FUNC_PTR(type, ptr) (__extension__ (type) (size_t) (ptr))
#else

# define JS_FUNC_TO_DATA_PTR(type, fun) ((type) (void *) (fun))
# define JS_DATA_TO_FUNC_PTR(type, ptr) ((type) (void *) (ptr))
#endif

#ifdef __GNUC__
# define JS_EXTENSION __extension__
# define JS_EXTENSION_(s) __extension__ ({ s; })
#else
# define JS_EXTENSION
# define JS_EXTENSION_(s) s
#endif

#endif 
