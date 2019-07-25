
























#ifndef WTF_Assertions_h
#define WTF_Assertions_h

















#include "Platform.h"

#if WTF_COMPILER_MSVC
#include <stddef.h>
#else
#include <inttypes.h>
#endif

#if WTF_PLATFORM_SYMBIAN
#include <e32def.h>
#include <e32debug.h>
#endif

#ifdef NDEBUG
#define ASSERTIONS_DISABLED_DEFAULT 1
#else
#define ASSERTIONS_DISABLED_DEFAULT 0
#endif

#ifndef ASSERT_DISABLED
#define ASSERT_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_ARG_DISABLED
#define ASSERT_ARG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef FATAL_DISABLED
#define FATAL_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ERROR_DISABLED
#define ERROR_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef LOG_DISABLED
#define LOG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#if WTF_COMPILER_GCC
#define WTF_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define WTF_PRETTY_FUNCTION __FUNCTION__
#endif




#if WTF_COMPILER_GCC && !defined(__OBJC__)
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) __attribute__((__format__(printf, formatStringArgument, extraArguments)))
#else
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) 
#endif



#ifdef __cplusplus
extern "C" {
#endif

typedef enum { WTFLogChannelOff, WTFLogChannelOn } WTFLogChannelState;

typedef struct {
    unsigned mask;
    const char *defaultName;
    WTFLogChannelState state;
} WTFLogChannel;

void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion);
void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);
void WTFReportArgumentAssertionFailure(const char* file, int line, const char* function, const char* argName, const char* assertion);
void WTFReportFatalError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
void WTFReportError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
void WTFLog(WTFLogChannel* channel, const char* format, ...) WTF_ATTRIBUTE_PRINTF(2, 3);
void WTFLogVerbose(const char* file, int line, const char* function, WTFLogChannel* channel, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);

#ifdef __cplusplus
}
#endif



#ifndef CRASH
#if WTF_PLATFORM_SYMBIAN
#define CRASH() do { \
    __DEBUGGER(); \
    User::Panic(_L("Webkit CRASH"),0); \
    } while(false)
#else
#define CRASH() do { \
    *(int *)(uintptr_t)0xbbadbeef = 0; \
    ((void(*)())0)(); /* More reliable, but doesn't say BBADBEEF */ \
} while(false)
#endif
#endif



#if WTF_PLATFORM_WIN_OS || WTF_PLATFORM_SYMBIAN

#undef ASSERT
#endif

#if ASSERT_DISABLED

#define ASSERT(assertion) ((void)0)
#if WTF_COMPILER_MSVC7
#define ASSERT_WITH_MESSAGE(assertion) ((void)0)
#elif WTF_COMPILER_WINSCW
#define ASSERT_WITH_MESSAGE(assertion, arg...) ((void)0)
#else
#define ASSERT_WITH_MESSAGE(assertion, ...) ((void)0)
#endif 
#define ASSERT_NOT_REACHED() ((void)0)
#define ASSERT_UNUSED(variable, assertion) ((void)variable)

#else

#define ASSERT(assertion) do \
    if (!(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
        CRASH(); \
    } \
while (0)
#if WTF_COMPILER_MSVC7 
#define ASSERT_WITH_MESSAGE(assertion) ((void)0)
#elif WTF_COMPILER_WINSCW
#define ASSERT_WITH_MESSAGE(assertion, arg...) ((void)0)
#else
#define ASSERT_WITH_MESSAGE(assertion, ...) do \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        CRASH(); \
    } \
while (0)
#endif 
#define ASSERT_NOT_REACHED() do { \
    WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 0); \
    CRASH(); \
} while (0)

#define ASSERT_UNUSED(variable, assertion) ASSERT(assertion)

#endif



#if ASSERT_ARG_DISABLED

#define ASSERT_ARG(argName, assertion) ((void)0)

#else

#define ASSERT_ARG(argName, assertion) do \
    if (!(assertion)) { \
        WTFReportArgumentAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #argName, #assertion); \
        CRASH(); \
    } \
while (0)

#endif


#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]
#endif



#if FATAL_DISABLED && !WTF_COMPILER_MSVC7 && !WTF_COMPILER_WINSCW
#define FATAL(...) ((void)0)
#elif WTF_COMPILER_MSVC7
#define FATAL() ((void)0)
#else
#define FATAL(...) do { \
    WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__); \
    CRASH(); \
} while (0)
#endif



#if ERROR_DISABLED && !WTF_COMPILER_MSVC7 && !WTF_COMPILER_WINSCW
#define LOG_ERROR(...) ((void)0)
#elif WTF_COMPILER_MSVC7
#define LOG_ERROR() ((void)0)
#elif WTF_COMPILER_WINSCW
#define LOG_ERROR(arg...)  ((void)0)
#else
#define LOG_ERROR(...) WTFReportError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__)
#endif



#if LOG_DISABLED && !WTF_COMPILER_MSVC7 && !WTF_COMPILER_WINSCW
#define LOG(channel, ...) ((void)0)
#elif WTF_COMPILER_MSVC7
#define LOG() ((void)0)
#elif WTF_COMPILER_WINSCW
#define LOG(arg...) ((void)0)
#else
#define LOG(channel, ...) WTFLog(&JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, channel), __VA_ARGS__)
#define JOIN_LOG_CHANNEL_WITH_PREFIX(prefix, channel) JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel)
#define JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel) prefix ## channel
#endif



#if LOG_DISABLED && !WTF_COMPILER_MSVC7 && !WTF_COMPILER_WINSCW
#define LOG_VERBOSE(channel, ...) ((void)0)
#elif WTF_COMPILER_MSVC7
#define LOG_VERBOSE(channel) ((void)0)
#elif WTF_COMPILER_WINSCW
#define LOG_VERBOSE(channel, arg...) ((void)0)
#else
#define LOG_VERBOSE(channel, ...) WTFLogVerbose(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, &JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, channel), __VA_ARGS__)
#endif

#endif
