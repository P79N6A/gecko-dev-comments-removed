















#ifndef ANDROID_UTILS_DEBUG_H
#define ANDROID_UTILS_DEBUG_H

#include <stdint.h>
#include <sys/types.h>

namespace android {


#ifdef __cplusplus
template<bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};
#define COMPILE_TIME_ASSERT(_exp) \
    template class CompileTimeAssert< (_exp) >;
#endif
#define COMPILE_TIME_ASSERT_FUNCTION_SCOPE(_exp) \
    CompileTimeAssert<( _exp )>();



#ifdef __cplusplus
template<bool C, typename LSH, typename RHS> struct CompileTimeIfElse;
template<typename LHS, typename RHS> 
struct CompileTimeIfElse<true,  LHS, RHS> { typedef LHS TYPE; };
template<typename LHS, typename RHS> 
struct CompileTimeIfElse<false, LHS, RHS> { typedef RHS TYPE; };
#endif


}; 

#endif 
