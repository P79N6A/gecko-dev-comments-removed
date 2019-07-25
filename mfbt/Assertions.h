







































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

#endif
