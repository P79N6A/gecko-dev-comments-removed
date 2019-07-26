






#ifndef mozilla_Assertions_h_
#define mozilla_Assertions_h_

#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"

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

















#ifdef __cplusplus
#  if defined(__clang__)
#    ifndef __has_extension
#      define __has_extension __has_feature /* compatibility, for older versions of clang */
#    endif
#    if __has_extension(cxx_static_assert)
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(__GNUC__)
#    if (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L)
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(_MSC_VER)
#    if _MSC_VER >= 1600 
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  elif defined(__HP_aCC)
#    if __HP_aCC >= 62500 && defined(_HP_CXX0x_SOURCE)
#      define MOZ_STATIC_ASSERT(cond, reason)    static_assert((cond), reason)
#    endif
#  endif
#endif
#ifndef MOZ_STATIC_ASSERT
   




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
#endif

#define MOZ_STATIC_ASSERT_IF(cond, expr, reason)  MOZ_STATIC_ASSERT(!(cond) || (expr), reason)

#ifdef __cplusplus
extern "C" {
#endif











#if defined(_MSC_VER)
   














#  ifdef __cplusplus
#    define MOZ_CRASH() \
       do { \
         __debugbreak(); \
         *((volatile int*) NULL) = 123; \
         ::TerminateProcess(::GetCurrentProcess(), 3); \
       } while (0)
#  else
#    define MOZ_CRASH() \
       do { \
         __debugbreak(); \
         *((volatile int*) NULL) = 123; \
         TerminateProcess(GetCurrentProcess(), 3); \
       } while (0)
#  endif
#else
#  ifdef __cplusplus
#    define MOZ_CRASH() \
       do { \
         *((volatile int*) NULL) = 123; \
         ::abort(); \
       } while (0)
#  else
#    define MOZ_CRASH() \
       do { \
         *((volatile int*) NULL) = 123; \
         abort(); \
       } while (0)
#  endif
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

#ifdef __cplusplus
} 
#endif

































#ifdef DEBUG
   
#  define MOZ_ASSERT_HELPER1(expr) \
     do { \
       if (!(expr)) { \
         MOZ_ReportAssertionFailure(#expr, __FILE__, __LINE__); \
         MOZ_CRASH(); \
       } \
     } while (0)
   
#  define MOZ_ASSERT_HELPER2(expr, explain) \
     do { \
       if (!(expr)) { \
         MOZ_ReportAssertionFailure(#expr " (" explain ")", __FILE__, __LINE__); \
         MOZ_CRASH(); \
       } \
     } while (0)
   
   







#  define MOZ_COUNT_ASSERT_ARGS_IMPL2(_1, _2, count, ...) \
     count
#  define MOZ_COUNT_ASSERT_ARGS_IMPL(args) \
	 MOZ_COUNT_ASSERT_ARGS_IMPL2 args
#  define MOZ_COUNT_ASSERT_ARGS(...) \
     MOZ_COUNT_ASSERT_ARGS_IMPL((__VA_ARGS__, 2, 1, 0))
   
#  define MOZ_ASSERT_CHOOSE_HELPER2(count) MOZ_ASSERT_HELPER##count
#  define MOZ_ASSERT_CHOOSE_HELPER1(count) MOZ_ASSERT_CHOOSE_HELPER2(count)
#  define MOZ_ASSERT_CHOOSE_HELPER(count) MOZ_ASSERT_CHOOSE_HELPER1(count)
   
#  define MOZ_ASSERT_GLUE(x, y) x y
#  define MOZ_ASSERT(...) \
     MOZ_ASSERT_GLUE(MOZ_ASSERT_CHOOSE_HELPER(MOZ_COUNT_ASSERT_ARGS(__VA_ARGS__)), \
                     (__VA_ARGS__))
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
#  define MOZ_NOT_REACHED_MARKER() __builtin_unreachable()
#elif defined(__GNUC__)
   




#  if MOZ_GCC_VERSION_AT_LEAST(4, 5, 0)
#    define MOZ_NOT_REACHED_MARKER() __builtin_unreachable()
#  else
#    ifdef __cplusplus
#      define MOZ_NOT_REACHED_MARKER() ::abort()
#    else
#      define MOZ_NOT_REACHED_MARKER() abort()
#    endif
#  endif
#elif defined(_MSC_VER)
#  define MOZ_NOT_REACHED_MARKER() __assume(0)
#else
#  ifdef __cplusplus
#    define MOZ_NOT_REACHED_MARKER() ::abort()
#  else
#    define MOZ_NOT_REACHED_MARKER() abort()
#  endif
#endif


















#if defined(DEBUG)
#  define MOZ_NOT_REACHED(reason) \
     do { \
       MOZ_ASSERT(false, reason); \
       MOZ_NOT_REACHED_MARKER(); \
     } while (0)
#else
#  define MOZ_NOT_REACHED(reason)  MOZ_NOT_REACHED_MARKER()
#endif







#ifdef DEBUG
#  define MOZ_ALWAYS_TRUE(expr)      MOZ_ASSERT((expr))
#  define MOZ_ALWAYS_FALSE(expr)     MOZ_ASSERT(!(expr))
#else
#  define MOZ_ALWAYS_TRUE(expr)      ((void)(expr))
#  define MOZ_ALWAYS_FALSE(expr)     ((void)(expr))
#endif

#endif
