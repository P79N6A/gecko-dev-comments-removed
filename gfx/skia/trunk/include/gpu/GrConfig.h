









#ifndef GrConfig_DEFINED
#define GrConfig_DEFINED

#include "SkTypes.h"












#if defined(__APPLE_CPP__) || defined(__APPLE_CC__)
    #include <TargetConditionals.h>
#endif





#if !defined(GR_CACHE_STATS)
    #define GR_CACHE_STATS      0
#endif




#if defined(SK_BUILD_FOR_WIN32)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else





#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <stdint.h>
#endif











#if !defined(GR_USER_CONFIG_FILE)
    #include "GrUserConfig.h"
#else
    #include GR_USER_CONFIG_FILE
#endif











#define GrPrintf SkDebugf





#define GR_STRING(X) GR_STRING_IMPL(X)
#define GR_STRING_IMPL(X) #X





#define GR_CONCAT(X,Y) GR_CONCAT_IMPL(X,Y)
#define GR_CONCAT_IMPL(X,Y) X##Y




#define GR_FILE_AND_LINE_STR __FILE__ "(" GR_STRING(__LINE__) ") : "







#if defined(_MSC_VER) && _MSC_VER
    #define GR_WARN(MSG) (GR_FILE_AND_LINE_STR "WARNING: " MSG)
#else
    #define GR_WARN(MSG) ("WARNING: " MSG)
#endif




#if !defined(GR_ALWAYSBREAK)
    #if     defined(SK_BUILD_FOR_WIN32)
        #define GR_ALWAYSBREAK SkNO_RETURN_HINT(); __debugbreak()
    #else
        
        
        
        #define GR_ALWAYSBREAK SkNO_RETURN_HINT(); *((int*)(int64_t)(int32_t)0xbeefcafe) = 0;
    #endif
#endif




#if !defined(GR_DEBUGBREAK)
    #ifdef SK_DEBUG
        #define GR_DEBUGBREAK GR_ALWAYSBREAK
    #else
        #define GR_DEBUGBREAK
    #endif
#endif




#if !defined(GR_ALWAYSASSERT)
    #define GR_ALWAYSASSERT(COND)                                        \
        do {                                                             \
            if (!(COND)) {                                               \
                GrPrintf("%s %s failed\n", GR_FILE_AND_LINE_STR, #COND); \
                GR_ALWAYSBREAK;                                          \
            }                                                            \
        } while (false)
#endif




#if !defined(GR_DEBUGASSERT)
    #ifdef SK_DEBUG
        #define GR_DEBUGASSERT(COND) GR_ALWAYSASSERT(COND)
    #else
        #define GR_DEBUGASSERT(COND)
    #endif
#endif




#define GrAlwaysAssert(COND) GR_ALWAYSASSERT(COND)








#if !defined(GR_STATIC_ASSERT)
    #if (defined(_MSC_VER) && _MSC_VER >= 1600) || (defined(__GXX_EXPERIMENTAL_CXX0X__) && __GXX_EXPERIMENTAL_CXX0X__)
        #define GR_STATIC_ASSERT(CONDITION) static_assert(CONDITION, "bug")
    #else
        template <bool> class GR_STATIC_ASSERT_FAILURE;
        template <> class GR_STATIC_ASSERT_FAILURE<true> {};
        #define GR_STATIC_ASSERT(CONDITION) \
            enum {GR_CONCAT(X,__LINE__) = \
            sizeof(GR_STATIC_ASSERT_FAILURE<CONDITION>)}
    #endif
#endif







#if !defined(GR_GEOM_BUFFER_MAP_THRESHOLD)
    #define GR_GEOM_BUFFER_MAP_THRESHOLD (1 << 15)
#endif






#if !defined(GR_DEFAULT_RESOURCE_CACHE_MB_LIMIT)
    #define GR_DEFAULT_RESOURCE_CACHE_MB_LIMIT 96
#endif






#if !defined(GR_DEFAULT_RESOURCE_CACHE_COUNT_LIMIT)
    #define GR_DEFAULT_RESOURCE_CACHE_COUNT_LIMIT 2048
#endif





#if !defined(GR_STROKE_PATH_RENDERING)
    #define GR_STROKE_PATH_RENDERING                 0
#endif









#if !defined(GR_ALWAYS_ALLOCATE_ON_HEAP)
    #define GR_ALWAYS_ALLOCATE_ON_HEAP 0
#endif

#endif
