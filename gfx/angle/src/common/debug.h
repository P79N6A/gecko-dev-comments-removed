







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


#if defined(ANGLE_ENABLE_TRACE) || defined(ANGLE_ENABLE_PERF)
#define TRACE(message, ...) gl::trace(true, "trace: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define TRACE(message, ...) (void(0))
#endif


#if defined(ANGLE_ENABLE_TRACE) || defined(ANGLE_ENABLE_PERF)
#define FIXME(message, ...) gl::trace(false, "fixme: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define FIXME(message, ...) (void(0))
#endif


#if defined(ANGLE_ENABLE_TRACE) || defined(ANGLE_ENABLE_PERF)
#define ERR(message, ...) gl::trace(false, "err: %s(%d): " message "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define ERR(message, ...) (void(0))
#endif


#if defined(ANGLE_ENABLE_TRACE) || defined(ANGLE_ENABLE_PERF)
#if defined(_MSC_VER)
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper ## __LINE__("%s" message "\n", __FUNCTION__, __VA_ARGS__);
#else
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper(message "\n", ##__VA_ARGS__);
#endif 
#else
#define EVENT(message, ...) (void(0))
#endif


#if !defined(NDEBUG)
#define ASSERT(expression) do { \
    if(!(expression)) \
        ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
        assert(expression); \
    } while(0)
#define UNUSED_ASSERTION_VARIABLE(variable)
#else
#define ASSERT(expression) (void(0))
#define UNUSED_ASSERTION_VARIABLE(variable) ((void)variable)
#endif

#ifndef ANGLE_ENABLE_TRACE
#define UNUSED_TRACE_VARIABLE(variable) ((void)variable)
#else
#define UNUSED_TRACE_VARIABLE(variable)
#endif





#ifndef NOASSERT_UNIMPLEMENTED
#define NOASSERT_UNIMPLEMENTED 0
#endif

#if !defined(NDEBUG)
#define UNIMPLEMENTED() do { \
    FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(NOASSERT_UNIMPLEMENTED); \
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


#if !defined(NDEBUG) && (!defined(_MSC_VER) || defined(_CPPRTTI)) && (!defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3) || defined(__GXX_RTTI))
#define HAS_DYNAMIC_TYPE(type, obj) (dynamic_cast<type >(obj) != NULL)
#else
#define HAS_DYNAMIC_TYPE(type, obj) true
#endif


#if defined(_MSC_VER) && _MSC_VER >= 1600
#define META_ASSERT_MSG(condition, msg) static_assert(condition, msg)
#else
#define META_ASSERT_CONCAT(a, b) a ## b
#define META_ASSERT_CONCAT2(a, b) META_ASSERT_CONCAT(a, b)
#define META_ASSERT_MSG(condition, msg) typedef int META_ASSERT_CONCAT2(COMPILE_TIME_ASSERT_, __LINE__)[static_cast<bool>(condition)?1:-1]
#endif
#define META_ASSERT(condition) META_ASSERT_MSG(condition, "compile time assertion failed.")

#endif   
