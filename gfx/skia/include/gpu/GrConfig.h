









#ifndef GrConfig_DEFINED
#define GrConfig_DEFINED












#if defined(__APPLE_CPP__) || defined(__APPLE_CC__)
    #include <TargetConditionals.h>
#endif





#if !defined(GR_ANDROID_BUILD)
    #define GR_ANDROID_BUILD    0
#endif
#if !defined(GR_IOS_BUILD)
    #define GR_IOS_BUILD        0
#endif
#if !defined(GR_LINUX_BUILD)
    #define GR_LINUX_BUILD      0
#endif
#if !defined(GR_MAC_BUILD)
    #define GR_MAC_BUILD        0
#endif
#if !defined(GR_WIN32_BUILD)
    #define GR_WIN32_BUILD      0
#endif
#if !defined(GR_QNX_BUILD)
    #define GR_QNX_BUILD        0
#endif




#if !GR_ANDROID_BUILD && !GR_IOS_BUILD && !GR_LINUX_BUILD && !GR_MAC_BUILD && !GR_WIN32_BUILD && !GR_QNX_BUILD
    #if defined(_WIN32)
        #undef GR_WIN32_BUILD
        #define GR_WIN32_BUILD      1

    #elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #undef GR_IOS_BUILD
        #define GR_IOS_BUILD        1

    #elif (defined(ANDROID_NDK) && ANDROID_NDK) || defined(ANDROID)
        #undef GR_ANDROID_BUILD
        #define GR_ANDROID_BUILD    1

    #elif TARGET_OS_MAC
        #undef GR_MAC_BUILD
        #define GR_MAC_BUILD        1

    #elif TARGET_OS_QNX || defined(__QNXNTO__)
        #undef GR_QNX_BUILD
        #define GR_QNX_BUILD        1

    #else
        #undef GR_LINUX_BUILD
        #define GR_LINUX_BUILD      1

    #endif
#endif



#ifndef GR_DEBUG
    #ifdef GR_RELEASE
        #define GR_DEBUG !GR_RELEASE
    #else
        #ifdef NDEBUG
            #define GR_DEBUG    0
        #else
            #define GR_DEBUG    1
        #endif
    #endif
#endif

#ifndef GR_RELEASE
    #define GR_RELEASE  !GR_DEBUG
#endif

#if GR_DEBUG == GR_RELEASE
    #error "GR_DEBUG and GR_RELEASE must not be the same"
#endif




#if GR_WIN32_BUILD

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else





#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif











#if !defined(GR_USER_CONFIG_FILE)
    #include "GrUserConfig.h"
#else
    #include GR_USER_CONFIG_FILE
#endif










#if !defined(GR_IMPLEMENTATION)
    #define GR_IMPLEMENTATION 0
#endif






#if !defined(GR_DLL)
    #define GR_DLL 0
#endif

#if GR_DLL
    #if GR_WIN32_BUILD
        #if GR_IMPLEMENTATION
            #define GR_API __declspec(dllexport)
        #else
            #define GR_API __declspec(dllimport)
        #endif
    #else
        #define GR_API __attribute__((visibility("default")))
    #endif
#else
    #define GR_API
#endif





extern GR_API void GrPrintf(const char format[], ...);





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
    #if     GR_WIN32_BUILD
        #define GR_ALWAYSBREAK __debugbreak()
    #else
        
        
        
        #define GR_ALWAYSBREAK *((int*)(int64_t)(int32_t)0xbeefcafe) = 0;
    #endif
#endif




#if !defined(GR_DEBUGBREAK)
    #if GR_DEBUG
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
    #if GR_DEBUG
        #define GR_DEBUGASSERT(COND) GR_ALWAYSASSERT(COND)
    #else
        #define GR_DEBUGASSERT(COND)
    #endif
#endif




#define GrAssert(COND) GR_DEBUGASSERT(COND)
#define GrAlwaysAssert(COND) GR_ALWAYSASSERT(COND)




inline void GrCrash() { GrAlwaysAssert(false); }
inline void GrCrash(const char* msg) { GrPrintf(msg); GrAlwaysAssert(false); }




#if !defined(GR_DEBUGCODE)
    #if GR_DEBUG
        #define GR_DEBUGCODE(X) X
    #else
        #define GR_DEBUGCODE(X)
    #endif
#endif








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

#if !defined(GR_SCALAR_IS_FLOAT)
    #define GR_SCALAR_IS_FLOAT   0
#endif
#if !defined(GR_SCALAR_IS_FIXED)
    #define GR_SCALAR_IS_FIXED   0
#endif

#if !defined(GR_TEXT_SCALAR_TYPE_IS_USHORT)
    #define GR_TEXT_SCALAR_TYPE_IS_USHORT  0
#endif
#if !defined(GR_TEXT_SCALAR_TYPE_IS_FLOAT)
    #define GR_TEXT_SCALAR_TYPE_IS_FLOAT   0
#endif
#if !defined(GR_TEXT_SCALAR_TYPE_IS_FIXED)
    #define GR_TEXT_SCALAR_TYPE_IS_FIXED   0
#endif

#ifndef GR_DUMP_TEXTURE_UPLOAD
    #define GR_DUMP_TEXTURE_UPLOAD  0
#endif





#if !defined(GR_COLLECT_STATS)
    #define GR_COLLECT_STATS GR_DEBUG
#endif






#if !defined(GR_STATIC_RECT_VB)
    #define GR_STATIC_RECT_VB 0
#endif







#if !defined(GR_AGGRESSIVE_SHADER_OPTS)
    #define GR_AGGRESSIVE_SHADER_OPTS 0
#endif







#if !defined(GR_GEOM_BUFFER_LOCK_THRESHOLD)
    #define GR_GEOM_BUFFER_LOCK_THRESHOLD (1 << 15)
#endif




#if !defined(GR_USE_OFFSCREEN_AA)
    #define GR_USE_OFFSCREEN_AA 1
#endif






#if !defined(GR_MAX_OFFSCREEN_AA_SIZE)
    #define GR_MAX_OFFSCREEN_AA_SIZE    256
#endif












#define GR_BUILD_SUM    (GR_WIN32_BUILD + GR_MAC_BUILD + GR_IOS_BUILD + GR_ANDROID_BUILD + GR_LINUX_BUILD + GR_QNX_BUILD)
#if 0 == GR_BUILD_SUM
    #error "Missing a GR_BUILD define"
#elif 1 != GR_BUILD_SUM
    #error "More than one GR_BUILD defined"
#endif


#if !GR_SCALAR_IS_FLOAT && !GR_SCALAR_IS_FIXED
    #undef  GR_SCALAR_IS_FLOAT
    #define GR_SCALAR_IS_FLOAT              1
    #pragma message GR_WARN("Scalar type not defined, defaulting to float")
#endif

#if !GR_TEXT_SCALAR_IS_FLOAT && \
    !GR_TEXT_SCALAR_IS_FIXED && \
    !GR_TEXT_SCALAR_IS_USHORT
    #undef  GR_TEXT_SCALAR_IS_FLOAT
    #define GR_TEXT_SCALAR_IS_FLOAT         1
    #pragma message GR_WARN("Text scalar type not defined, defaulting to float")
#endif

#if 0
#if GR_WIN32_BUILD

#endif
#if GR_MAC_BUILD

#endif
#if GR_IOS_BUILD

#endif
#if GR_ANDROID_BUILD

#endif
#if GR_LINUX_BUILD

#endif
#if GR_QNX_BUILD

#endif
#endif

#endif

