







#ifndef COMPILER_DEBUG_H_
#define COMPILER_DEBUG_H_

#include <assert.h>

#ifdef _DEBUG
#define TRACE_ENABLED
#endif  


#ifdef TRACE_ENABLED

#ifdef  __cplusplus
extern "C" {
#endif
void Trace(const char* format, ...);
#ifdef  __cplusplus
}
#endif  

#else

#define Trace(...) ((void)0)

#endif


#define ASSERT(expression) do { \
    if(!(expression)) \
        Trace("Assert failed: %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
    assert(expression); \
} while(0)

#define UNIMPLEMENTED() do { \
    Trace("Unimplemented invoked: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
} while(0)

#define UNREACHABLE() do { \
    Trace("Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
} while(0)

#endif

