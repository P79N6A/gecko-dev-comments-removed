







#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#include <stdio.h>
#include <assert.h>

#include "common/angleutils.h"

#if !defined(TRACE_OUTPUT_FILE)
#define TRACE_OUTPUT_FILE "debug.txt"
#endif

namespace gl
{
    
    void trace(bool traceFileDebugOnly, const char *format, ...);

    
    bool perfActive();

    
    class ScopedPerfEventHelper
    {
      public:
        ScopedPerfEventHelper(const char* format, ...);
        ~ScopedPerfEventHelper();

      private:
        DISALLOW_COPY_AND_ASSIGN(ScopedPerfEventHelper);
    };
}


#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define TRACE(message, ...) (void(0))
#else
#define TRACE(message, ...) gl::trace(true, "trace: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif


#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define FIXME(message, ...) (void(0))
#else
#define FIXME(message, ...) gl::trace(false, "fixme: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif


#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define ERR(message, ...) (void(0))
#else
#define ERR(message, ...) gl::trace(false, "err: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif


#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define EVENT(message, ...) (void(0))
#else
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper ## __LINE__(__FUNCTION__ message "\n", __VA_ARGS__);
#endif


#if !defined(NDEBUG)
#define ASSERT(expression) do { \
    if(!(expression)) \
        ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
        assert(expression); \
    } while(0)
#else
#define ASSERT(expression) (void(0))
#endif


#if !defined(NDEBUG)
#define UNIMPLEMENTED() do { \
    FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
    } while(0)
#else
    #define UNIMPLEMENTED() FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__)
#endif


#if !defined(NDEBUG)
#define UNREACHABLE() do { \
    ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
    } while(0)
#else
    #define UNREACHABLE() ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__)
#endif


#define META_ASSERT(condition) typedef int COMPILE_TIME_ASSERT_##__LINE__[static_cast<bool>(condition)?1:-1]

#endif   
