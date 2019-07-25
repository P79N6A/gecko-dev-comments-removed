







































#ifndef mozilla_Assertions_h_
#define mozilla_Assertions_h_

#include "mozilla/Types.h"















#ifdef __cplusplus
extern "C" {
#endif

extern MFBT_API(void)
JS_Assert(const char* s, const char* file, int ln);

#ifdef __cplusplus
} 
#endif












#ifdef DEBUG
#  define MOZ_ASSERT(expr_)                                      \
     ((expr_) ? ((void)0) : JS_Assert(#expr_, __FILE__, __LINE__))
#else
#  define MOZ_ASSERT(expr_) ((void)0)
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
