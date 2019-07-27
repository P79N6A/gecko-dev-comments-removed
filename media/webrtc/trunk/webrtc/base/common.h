









#ifndef WEBRTC_BASE_COMMON_H_  
#define WEBRTC_BASE_COMMON_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"

#if defined(_MSC_VER)

#pragma warning(disable:4355)
#endif





#ifndef RTC_UNUSED
#define RTC_UNUSED(x) RtcUnused(static_cast<const void*>(&x))
#define RTC_UNUSED2(x, y) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y))
#define RTC_UNUSED3(x, y, z) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z))
#define RTC_UNUSED4(x, y, z, a) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a))
#define RTC_UNUSED5(x, y, z, a, b) RtcUnused(static_cast<const void*>(&x)); \
    RtcUnused(static_cast<const void*>(&y)); \
    RtcUnused(static_cast<const void*>(&z)); \
    RtcUnused(static_cast<const void*>(&a)); \
    RtcUnused(static_cast<const void*>(&b))
inline void RtcUnused(const void*) {}
#endif  

#if !defined(WEBRTC_WIN)

#ifndef strnicmp
#define strnicmp(x, y, n) strncasecmp(x, y, n)
#endif

#ifndef stricmp
#define stricmp(x, y) strcasecmp(x, y)
#endif



#define stdmax(x, y) std::max(x, y)
#else
#define stdmax(x, y) rtc::_max(x, y)
#endif

#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x) / sizeof(x[0])))





#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG _DEBUG
#endif  





namespace rtc {




void Break();




void LogAssert(const char* function, const char* file, int line,
               const char* expression);

typedef void (*AssertLogger)(const char* function,
                             const char* file,
                             int line,
                             const char* expression);






void SetCustomAssertLogger(AssertLogger logger);

bool IsOdd(int n);

bool IsEven(int n);

}  

#if ENABLE_DEBUG

namespace rtc {

inline bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression) {
  if (!result) {
    LogAssert(function, file, line, expression);
    Break();
  }
  return result;
}



inline bool AssertNoBreak(bool result, const char* function, const char* file,
                          int line, const char* expression) {
  if (!result)
    LogAssert(function, file, line, expression);
  return result;
}

}  

#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ ""
#endif

#ifndef ASSERT
#if defined(WIN32)



#define ASSERT(x) \
    (rtc::AssertNoBreak((x), __FUNCTION__, __FILE__, __LINE__, #x) ? \
     (void)(1) : __debugbreak())
#else
#define ASSERT(x) \
    (void)rtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif
#endif

#ifndef VERIFY
#if defined(WIN32)
#define VERIFY(x) \
    (rtc::AssertNoBreak((x), __FUNCTION__, __FILE__, __LINE__, #x) ? \
     true : (__debugbreak(), false))
#else
#define VERIFY(x) rtc::Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif
#endif

#else  

namespace rtc {

inline bool ImplicitCastToBool(bool result) { return result; }

}  

#ifndef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef VERIFY
#define VERIFY(x) rtc::ImplicitCastToBool(x)
#endif

#endif  

#define COMPILE_TIME_ASSERT(expr)       char CTA_UNIQUE_NAME[expr]
#define CTA_UNIQUE_NAME                 CTA_MAKE_NAME(__LINE__)
#define CTA_MAKE_NAME(line)             CTA_MAKE_NAME2(line)
#define CTA_MAKE_NAME2(line)            constraint_ ## line


#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline))
#elif defined(WEBRTC_WIN)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif






#if defined(WEBRTC_WIN)
#define OVERRIDE override
#elif defined(__clang__)




#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700

#define OVERRIDE override
#else
#define OVERRIDE
#endif







#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  

#endif  
