







#ifndef mozilla_Assertions_h
#define mozilla_Assertions_h

#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/Likely.h"
#include "mozilla/MacroArgs.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
   





#  ifdef __cplusplus
   extern "C" {
#  endif
   __declspec(dllimport) int __stdcall
   TerminateProcess(void* hProcess, unsigned int uExitCode);
   __declspec(dllimport) void* __stdcall GetCurrentProcess(void);
#  ifdef __cplusplus
   }
#  endif
#else
#  include <signal.h>
#endif
#ifdef ANDROID
#  include <android/log.h>
#endif



















#ifndef __cplusplus
   




#  if defined(__GNUC__)
#    define MOZ_STATIC_ASSERT_UNUSED_ATTRIBUTE __attribute__((unused))
#  else
#    define MOZ_STATIC_ASSERT_UNUSED_ATTRIBUTE
#  endif
#  define MOZ_STATIC_ASSERT_GLUE1(x, y)          x##y
#  define MOZ_STATIC_ASSERT_GLUE(x, y)           MOZ_STATIC_ASSERT_GLUE1(x, y)
#  if defined(__SUNPRO_CC)
     










#    define MOZ_STATIC_ASSERT(cond, reason) \
       extern char MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __LINE__)[(cond) ? 1 : -1]
#  elif defined(__COUNTER__)
     













#    define MOZ_STATIC_ASSERT(cond, reason) \
       typedef int MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __COUNTER__)[(cond) ? 1 : -1] MOZ_STATIC_ASSERT_UNUSED_ATTRIBUTE
#  else
#    define MOZ_STATIC_ASSERT(cond, reason) \
       extern void MOZ_STATIC_ASSERT_GLUE(moz_static_assert, __LINE__)(int arg[(cond) ? 1 : -1]) MOZ_STATIC_ASSERT_UNUSED_ATTRIBUTE
#  endif

#define MOZ_STATIC_ASSERT_IF(cond, expr, reason)  MOZ_STATIC_ASSERT(!(cond) || (expr), reason)
#else
#define MOZ_STATIC_ASSERT_IF(cond, expr, reason)  static_assert(!(cond) || (expr), reason)
#endif

#ifdef __cplusplus
extern "C" {
#endif









static MOZ_ALWAYS_INLINE void
MOZ_ReportAssertionFailure(const char* s, const char* file, int ln)
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_FATAL, "MOZ_Assert",
                      "Assertion failure: %s, at %s:%d\n", s, file, ln);
#else
  fprintf(stderr, "Assertion failure: %s, at %s:%d\n", s, file, ln);
  fflush(stderr);
#endif
}

static MOZ_ALWAYS_INLINE void
MOZ_ReportCrash(const char* s, const char* file, int ln)
{
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_FATAL, "MOZ_CRASH",
                        "Hit MOZ_CRASH(%s) at %s:%d\n", s, file, ln);
#else
  fprintf(stderr, "Hit MOZ_CRASH(%s) at %s:%d\n", s, file, ln);
  fflush(stderr);
#endif
}





#if defined(_MSC_VER)
   






















__declspec(noreturn) __inline void MOZ_NoReturn() {}

#  ifdef __cplusplus
#    define MOZ_REALLY_CRASH() \
       do { \
         ::__debugbreak(); \
         *((volatile int*) NULL) = 123; \
         ::TerminateProcess(::GetCurrentProcess(), 3); \
         ::MOZ_NoReturn(); \
       } while (0)
#  else
#    define MOZ_REALLY_CRASH() \
       do { \
         __debugbreak(); \
         *((volatile int*) NULL) = 123; \
         TerminateProcess(GetCurrentProcess(), 3); \
         MOZ_NoReturn(); \
       } while (0)
#  endif
#else
#  ifdef __cplusplus
#    define MOZ_REALLY_CRASH() \
       do { \
         *((volatile int*) NULL) = 123; \
         ::abort(); \
       } while (0)
#  else
#    define MOZ_REALLY_CRASH() \
       do { \
         *((volatile int*) NULL) = 123; \
         abort(); \
       } while (0)
#  endif
#endif






















#ifndef DEBUG
#  define MOZ_CRASH(...) MOZ_REALLY_CRASH()
#else
#  define MOZ_CRASH(...) \
     do { \
       MOZ_ReportCrash("" __VA_ARGS__, __FILE__, __LINE__); \
       MOZ_REALLY_CRASH(); \
     } while(0)
#endif

#ifdef __cplusplus
} 
#endif








































#ifdef __cplusplus
#  if defined(__clang__)
#    define MOZ_SUPPORT_ASSERT_CONDITION_TYPE_VALIDATION
#  elif defined(__GNUC__)

#    if MOZ_GCC_VERSION_AT_LEAST(4, 5, 0)
#      define MOZ_SUPPORT_ASSERT_CONDITION_TYPE_VALIDATION
#    endif
#  elif defined(_MSC_VER)

#  endif
#endif

#ifdef MOZ_SUPPORT_ASSERT_CONDITION_TYPE_VALIDATION
#  include "mozilla/TypeTraits.h"
namespace mozilla {
namespace detail {

template<typename T>
struct IsFunction
{
    static const bool value = false;
};

template<typename R, typename... A>
struct IsFunction<R(A...)>
{
    static const bool value = true;
};

template<typename T>
void ValidateAssertConditionType()
{
  typedef typename RemoveReference<T>::Type ValueT;
  static_assert(!IsArray<ValueT>::value,
                "Expected boolean assertion condition, got an array or a string!");
  static_assert(!IsFunction<ValueT>::value,
                "Expected boolean assertion condition, got a function! Did you intend to call that function?");
  static_assert(!IsFloatingPoint<ValueT>::value,
                "It's often a bad idea to assert that a floating-point number is nonzero, "
                "because such assertions tend to intermittently fail. Shouldn't your code gracefully handle "
                "this case instead of asserting? Anyway, if you really want to "
                "do that, write an explicit boolean condition, like !!x or x!=0.");
}

} 
} 
#  define MOZ_VALIDATE_ASSERT_CONDITION_TYPE(x) mozilla::detail::ValidateAssertConditionType<decltype(x)>()
#else
#  define MOZ_VALIDATE_ASSERT_CONDITION_TYPE(x)
#endif


#define MOZ_ASSERT_HELPER1(expr) \
   do { \
     MOZ_VALIDATE_ASSERT_CONDITION_TYPE(expr); \
     if (MOZ_UNLIKELY(!(expr))) { \
       MOZ_ReportAssertionFailure(#expr, __FILE__, __LINE__); \
       MOZ_REALLY_CRASH(); \
     } \
   } while (0)

#define MOZ_ASSERT_HELPER2(expr, explain) \
   do { \
     MOZ_VALIDATE_ASSERT_CONDITION_TYPE(expr); \
     if (MOZ_UNLIKELY(!(expr))) { \
       MOZ_ReportAssertionFailure(#expr " (" explain ")", __FILE__, __LINE__); \
       MOZ_REALLY_CRASH(); \
     } \
   } while (0)

#define MOZ_RELEASE_ASSERT_GLUE(a, b) a b
#define MOZ_RELEASE_ASSERT(...) \
   MOZ_RELEASE_ASSERT_GLUE( \
     MOZ_PASTE_PREFIX_AND_ARG_COUNT(MOZ_ASSERT_HELPER, __VA_ARGS__), \
     (__VA_ARGS__))

#ifdef DEBUG
#  define MOZ_ASSERT(...) MOZ_RELEASE_ASSERT(__VA_ARGS__)
#else
#  define MOZ_ASSERT(...) do { } while(0)
#endif 










#ifdef DEBUG
#  define MOZ_ASSERT_IF(cond, expr) \
     do { \
       if (cond) \
         MOZ_ASSERT(expr); \
     } while (0)
#else
#  define MOZ_ASSERT_IF(cond, expr)  do { } while (0)
#endif








#if defined(__clang__)
#  define MOZ_ASSUME_UNREACHABLE_MARKER() __builtin_unreachable()
#elif defined(__GNUC__)
   




#  if MOZ_GCC_VERSION_AT_LEAST(4, 5, 0)
#    define MOZ_ASSUME_UNREACHABLE_MARKER() __builtin_unreachable()
#  else
#    ifdef __cplusplus
#      define MOZ_ASSUME_UNREACHABLE_MARKER() ::abort()
#    else
#      define MOZ_ASSUME_UNREACHABLE_MARKER() abort()
#    endif
#  endif
#elif defined(_MSC_VER)
#  define MOZ_ASSUME_UNREACHABLE_MARKER() __assume(0)
#else
#  ifdef __cplusplus
#    define MOZ_ASSUME_UNREACHABLE_MARKER() ::abort()
#  else
#    define MOZ_ASSUME_UNREACHABLE_MARKER() abort()
#  endif
#endif








































#if defined(DEBUG)
#  define MOZ_ASSUME_UNREACHABLE(...) \
     do { \
       MOZ_ASSERT(false, "MOZ_ASSUME_UNREACHABLE(" __VA_ARGS__ ")"); \
       MOZ_ASSUME_UNREACHABLE_MARKER(); \
     } while (0)
#else
#  define MOZ_ASSUME_UNREACHABLE(reason)  MOZ_ASSUME_UNREACHABLE_MARKER()
#endif







#ifdef DEBUG
#  define MOZ_ALWAYS_TRUE(expr)      MOZ_ASSERT((expr))
#  define MOZ_ALWAYS_FALSE(expr)     MOZ_ASSERT(!(expr))
#else
#  define MOZ_ALWAYS_TRUE(expr)      ((void)(expr))
#  define MOZ_ALWAYS_FALSE(expr)     ((void)(expr))
#endif

#endif
