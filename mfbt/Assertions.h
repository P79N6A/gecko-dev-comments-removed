









































#ifndef mozilla_Assertions_h_
#define mozilla_Assertions_h_

#include "mozilla/Types.h"

















#ifdef __cplusplus
#  if defined(__clang__)
#    ifndef __has_extension
#      define __has_extension __has_feature /* compatibility, for older versions of clang */
#    endif
#    if __has_extension(cxx_static_assert)
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(__GNUC__)
#    if (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L) && \
        (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(_MSC_VER)
#    if _MSC_VER >= 1600 
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(__HP_aCC)
#    if __HP_aCC >= 62500 && defined(_HP_CXX0x_SOURCE)
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  endif
#endif
#ifndef MOZ_STATIC_ASSERT
#  define MOZ_STATIC_ASSERT_GLUE1(x, y)          x##y
#  define MOZ_STATIC_ASSERT_GLUE(x, y)           MOZ_STATIC_ASSERT_GLUE1(x, y)
#  if defined(__SUNPRO_CC)
     










#    define MOZ_STATIC_ASSERT(cond, reason) \
       extern char MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __LINE__)[(cond) ? 1 : -1]
#  elif defined(__COUNTER__)
     













#    define MOZ_STATIC_ASSERT(cond, reason) \
       typedef int MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __COUNTER__)[(cond) ? 1 : -1]
#  else
#    define MOZ_STATIC_ASSERT(cond, reason) \
       extern void MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __LINE__)(int arg[(cond) ? 1 : -1])
#  endif
#endif

#define MOZ_STATIC_ASSERT_IF(cond, expr, reason)  MOZ_STATIC_ASSERT(!(cond) || (expr), reason)















#ifdef __cplusplus
extern "C" {
#endif

extern MFBT_API(void)
JS_Assert(const char* s, const char* file, int ln);

#ifdef __cplusplus
} 
#endif

































#ifdef DEBUG
   
#  define MOZ_ASSERT_HELPER1(expr) \
     ((expr) ? ((void)0) : JS_Assert(#expr, __FILE__, __LINE__))
   
#  define MOZ_ASSERT_HELPER2(expr, explain) \
     ((expr) ? ((void)0) : JS_Assert(#expr " (" explain ")", __FILE__, __LINE__))
   
   
#  define MOZ_COUNT_ASSERT_ARGS(...) \
     MOZ_COUNT_ASSERT_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#  define MOZ_COUNT_ASSERT_ARGS_IMPL(_1, _2, count, ...) \
     count
   
#  define MOZ_ASSERT_VAHELP2(count, ...) MOZ_ASSERT_HELPER##count(__VA_ARGS__)
#  define MOZ_ASSERT_VAHELP(count, ...) MOZ_ASSERT_VAHELP2(count, __VA_ARGS__)
   
#  define MOZ_ASSERT(...) \
     MOZ_ASSERT_VAHELP(MOZ_COUNT_ASSERT_ARGS(__VA_ARGS__), __VA_ARGS__)
#else
#  define MOZ_ASSERT(...) ((void)0)
#endif 










#ifdef DEBUG
#  define MOZ_ASSERT_IF(cond, expr)  ((cond) ? MOZ_ASSERT(expr) : ((void)0))
#else
#  define MOZ_ASSERT_IF(cond, expr)  ((void)0)
#endif


















#ifdef DEBUG
#  define MOZ_NOT_REACHED(reason)    JS_Assert(reason, __FILE__, __LINE__)
#else
#  define MOZ_NOT_REACHED(reason)    ((void)0)
#endif







#ifdef DEBUG
#  define MOZ_ALWAYS_TRUE(expr)      MOZ_ASSERT((expr))
#  define MOZ_ALWAYS_FALSE(expr)     MOZ_ASSERT(!(expr))
#else
#  define MOZ_ALWAYS_TRUE(expr)      ((void)(expr))
#  define MOZ_ALWAYS_FALSE(expr)     ((void)(expr))
#endif

#endif
