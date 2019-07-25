







































#ifndef mozilla_throw_gcc_h
#define mozilla_throw_gcc_h

#include <stdio.h>              
#include <string.h>             





#include "mozilla/mozalloc_abort.h"

namespace std {





NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_bad_exception(void)
{
    mozalloc_abort("fatal: STL threw bad_exception");
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_bad_alloc(void)
{
    mozalloc_abort("fatal: STL threw bad_alloc");
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_bad_cast(void)
{
    mozalloc_abort("fatal: STL threw bad_cast");
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_bad_typeid(void)
{
    mozalloc_abort("fatal: STL threw bad_typeid");
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_logic_error(const char* msg)
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_domain_error(const char* msg)
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_invalid_argument(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_length_error(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_out_of_range(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_runtime_error(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_range_error(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_overflow_error(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_underflow_error(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_ios_failure(const char* msg) 
{
    mozalloc_abort(msg);
}

NS_ALWAYS_INLINE inline void NS_NORETURN
__throw_system_error(int err) 
{
    char error[128];
    snprintf(error, sizeof(error)-1,
             "fatal: STL threw system_error: %s (%d)", strerror(err), err);
    mozalloc_abort(error);
}

} 

#endif  
