







#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#include <stdio.h>
#include <assert.h>

namespace gl
{
    
    void trace(const char *format, ...);
}


#ifndef NDEBUG
    #define TRACE(message, ...) gl::trace("trace: %s"message"\n", __FUNCTION__, __VA_ARGS__)
#else
    #define TRACE(...) ((void)0)
#endif


#define FIXME(message, ...) gl::trace("fixme: %s"message"\n", __FUNCTION__, __VA_ARGS__)


#define ERR(message, ...) gl::trace("err: %s"message"\n", __FUNCTION__, __VA_ARGS__)


#define ASSERT(expression) do { \
    if(!(expression)) \
        ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
    assert(expression); \
    } while(0)



#ifndef NDEBUG
    #define UNIMPLEMENTED() do { \
        FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
        assert(false); \
        } while(0)
#else
    #define UNIMPLEMENTED() FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__)
#endif


#ifndef NDEBUG
    #define UNREACHABLE() do { \
        ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
        assert(false); \
        } while(0)
#else
    #define UNREACHABLE() ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__)
#endif


#define META_ASSERT(condition) typedef int COMPILE_TIME_ASSERT_##__LINE__[static_cast<bool>(condition)?1:-1]

#endif   
