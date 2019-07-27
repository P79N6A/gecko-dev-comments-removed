





#ifndef nsDebug_h___
#define nsDebug_h___

#include "nscore.h"
#include "nsError.h"

#include "nsXPCOM.h"
#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"
#include <stdarg.h>

#ifdef DEBUG
#include "prprf.h"
#endif














#ifdef __cplusplus
#ifdef DEBUG
inline bool NS_warn_if_impl(bool aCondition, const char* aExpr,
                            const char* aFile, int32_t aLine)
{
  if (MOZ_UNLIKELY(aCondition)) {
    NS_DebugBreak(NS_DEBUG_WARNING, nullptr, aExpr, aFile, aLine);
  }
  return aCondition;
}
#define NS_WARN_IF(condition) \
  NS_warn_if_impl(condition, #condition, __FILE__, __LINE__)
#else
#define NS_WARN_IF(condition) (bool)(condition)
#endif
#endif









#ifdef DEBUG
#define NS_WARN_IF_FALSE(_expr,_msg)                          \
  do {                                                        \
    if (!(_expr)) {                                           \
      NS_DebugBreak(NS_DEBUG_WARNING, _msg, #_expr, __FILE__, __LINE__); \
    }                                                         \
  } while(0)
#else
#define NS_WARN_IF_FALSE(_expr, _msg)  do { /* nothing */ } while(0)
#endif









#ifdef DEBUG
inline void MOZ_PretendNoReturn()
  MOZ_PRETEND_NORETURN_FOR_STATIC_ANALYSIS {}
#define NS_ASSERTION(expr, str)                               \
  do {                                                        \
    if (!(expr)) {                                            \
      NS_DebugBreak(NS_DEBUG_ASSERTION, str, #expr, __FILE__, __LINE__); \
      MOZ_PretendNoReturn();                                         \
    }                                                         \
  } while(0)
#else
#define NS_ASSERTION(expr, str)        do { /* nothing */ } while(0)
#endif




#define NS_PRECONDITION(expr, str) NS_ASSERTION(expr, str)
#define NS_POSTCONDITION(expr, str) NS_ASSERTION(expr, str)





#ifdef DEBUG
#define NS_NOTYETIMPLEMENTED(str)                             \
  do {                                                        \
    NS_DebugBreak(NS_DEBUG_ASSERTION, str, "NotYetImplemented", __FILE__, __LINE__); \
    MOZ_PretendNoReturn();                                    \
  } while(0)
#else
#define NS_NOTYETIMPLEMENTED(str)      do { /* nothing */ } while(0)
#endif





#ifdef DEBUG
#define NS_NOTREACHED(str)                                    \
  do {                                                        \
    NS_DebugBreak(NS_DEBUG_ASSERTION, str, "Not Reached", __FILE__, __LINE__); \
    MOZ_PretendNoReturn();                                    \
  } while(0)
#else
#define NS_NOTREACHED(str)             do { /* nothing */ } while(0)
#endif




#ifdef DEBUG
#define NS_ERROR(str)                                         \
  do {                                                        \
    NS_DebugBreak(NS_DEBUG_ASSERTION, str, "Error", __FILE__, __LINE__); \
    MOZ_PretendNoReturn();                                    \
  } while(0)
#else
#define NS_ERROR(str)                  do { /* nothing */ } while(0)
#endif




#ifdef DEBUG
#define NS_WARNING(str)                                       \
  NS_DebugBreak(NS_DEBUG_WARNING, str, nullptr, __FILE__, __LINE__)
#else
#define NS_WARNING(str)                do { /* nothing */ } while(0)
#endif






#ifdef DEBUG
#define NS_ABORT()                                            \
  do {                                                        \
    NS_DebugBreak(NS_DEBUG_ABORT, nullptr, nullptr, __FILE__, __LINE__); \
    MOZ_PretendNoReturn();                                    \
  } while(0)
#else
#define NS_ABORT()                     do { /* nothing */ } while(0)
#endif




#ifdef DEBUG
#define NS_BREAK()                                            \
  do {                                                        \
    NS_DebugBreak(NS_DEBUG_BREAK, nullptr, nullptr, __FILE__, __LINE__); \
    MOZ_PretendNoReturn();                                    \
  } while(0)
#else
#define NS_BREAK()                     do { /* nothing */ } while(0)
#endif







#ifndef HAVE_STATIC_ANNOTATIONS
#define HAVE_STATIC_ANNOTATIONS

#ifdef XGILL_PLUGIN

#define STATIC_PRECONDITION(COND)         __attribute__((precondition(#COND)))
#define STATIC_PRECONDITION_ASSUME(COND)  __attribute__((precondition_assume(#COND)))
#define STATIC_POSTCONDITION(COND)        __attribute__((postcondition(#COND)))
#define STATIC_POSTCONDITION_ASSUME(COND) __attribute__((postcondition_assume(#COND)))
#define STATIC_INVARIANT(COND)            __attribute__((invariant(#COND)))
#define STATIC_INVARIANT_ASSUME(COND)     __attribute__((invariant_assume(#COND)))


#define STATIC_PASTE2(X,Y) X ## Y
#define STATIC_PASTE1(X,Y) STATIC_PASTE2(X,Y)

#define STATIC_ASSUME(COND)                          \
  do {                                               \
    __attribute__((assume_static(#COND), unused))    \
    int STATIC_PASTE1(assume_static_, __COUNTER__);  \
  } while(0)

#define STATIC_ASSERT_RUNTIME(COND)                         \
  do {                                                      \
    __attribute__((assert_static_runtime(#COND), unused))   \
    int STATIC_PASTE1(assert_static_runtime_, __COUNTER__); \
  } while(0)

#else 

#define STATIC_PRECONDITION(COND)
#define STATIC_PRECONDITION_ASSUME(COND)
#define STATIC_POSTCONDITION(COND)
#define STATIC_POSTCONDITION_ASSUME(COND)
#define STATIC_INVARIANT(COND)
#define STATIC_INVARIANT_ASSUME(COND)

#define STATIC_ASSUME(COND)          do { /* nothing */ } while(0)
#define STATIC_ASSERT_RUNTIME(COND)  do { /* nothing */ } while(0)

#endif 

#define STATIC_SKIP_INFERENCE STATIC_INVARIANT(skip_inference())

#endif 

#ifdef XGILL_PLUGIN





#undef NS_PRECONDITION
#undef NS_ASSERTION
#undef NS_POSTCONDITION

#define NS_PRECONDITION(expr, str)   STATIC_ASSERT_RUNTIME(expr)
#define NS_ASSERTION(expr, str)      STATIC_ASSERT_RUNTIME(expr)
#define NS_POSTCONDITION(expr, str)  STATIC_ASSERT_RUNTIME(expr)

#endif 











#define NS_RUNTIMEABORT(msg)                                    \
  NS_DebugBreak(NS_DEBUG_ABORT, msg, nullptr, __FILE__, __LINE__)








#define NS_ENSURE_TRUE(x, ret)                                \
  do {                                                        \
    if (MOZ_UNLIKELY(!(x))) {                                 \
       NS_WARNING("NS_ENSURE_TRUE(" #x ") failed");           \
       return ret;                                            \
    }                                                         \
  } while(0)

#define NS_ENSURE_FALSE(x, ret)                               \
  NS_ENSURE_TRUE(!(x), ret)

#define NS_ENSURE_TRUE_VOID(x)                                \
  do {                                                        \
    if (MOZ_UNLIKELY(!(x))) {                                 \
       NS_WARNING("NS_ENSURE_TRUE(" #x ") failed");           \
       return;                                                \
    }                                                         \
  } while(0)

#define NS_ENSURE_FALSE_VOID(x)                               \
  NS_ENSURE_TRUE_VOID(!(x))





#if defined(DEBUG) && !defined(XPCOM_GLUE_AVOID_NSPR)

#define NS_ENSURE_SUCCESS_BODY(res, ret)                                  \
    char *msg = PR_smprintf("NS_ENSURE_SUCCESS(%s, %s) failed with "      \
                            "result 0x%X", #res, #ret, __rv);             \
    NS_WARNING(msg);                                                      \
    PR_smprintf_free(msg);

#define NS_ENSURE_SUCCESS_BODY_VOID(res)                                  \
    char *msg = PR_smprintf("NS_ENSURE_SUCCESS_VOID(%s) failed with "     \
                            "result 0x%X", #res, __rv);                   \
    NS_WARNING(msg);                                                      \
    PR_smprintf_free(msg);

#else

#define NS_ENSURE_SUCCESS_BODY(res, ret)                                  \
    NS_WARNING("NS_ENSURE_SUCCESS(" #res ", " #ret ") failed");

#define NS_ENSURE_SUCCESS_BODY_VOID(res)                                  \
    NS_WARNING("NS_ENSURE_SUCCESS_VOID(" #res ") failed");

#endif

#define NS_ENSURE_SUCCESS(res, ret)                                       \
  do {                                                                    \
    nsresult __rv = res; /* Don't evaluate |res| more than once */        \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                    \
      return ret;                                                         \
    }                                                                     \
  } while(0)

#define NS_ENSURE_SUCCESS_VOID(res)                                       \
  do {                                                                    \
    nsresult __rv = res;                                                  \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY_VOID(res)                                    \
      return;                                                             \
    }                                                                     \
  } while(0)





#define NS_ENSURE_ARG(arg)                                    \
  NS_ENSURE_TRUE(arg, NS_ERROR_INVALID_ARG)

#define NS_ENSURE_ARG_POINTER(arg)                            \
  NS_ENSURE_TRUE(arg, NS_ERROR_INVALID_POINTER)

#define NS_ENSURE_ARG_MIN(arg, min)                           \
  NS_ENSURE_TRUE((arg) >= min, NS_ERROR_INVALID_ARG)

#define NS_ENSURE_ARG_MAX(arg, max)                           \
  NS_ENSURE_TRUE((arg) <= max, NS_ERROR_INVALID_ARG)

#define NS_ENSURE_ARG_RANGE(arg, min, max)                    \
  NS_ENSURE_TRUE(((arg) >= min) && ((arg) <= max), NS_ERROR_INVALID_ARG)

#define NS_ENSURE_STATE(state)                                \
  NS_ENSURE_TRUE(state, NS_ERROR_UNEXPECTED)

#define NS_ENSURE_NO_AGGREGATION(outer)                       \
  NS_ENSURE_FALSE(outer, NS_ERROR_NO_AGGREGATION)



#ifdef XPCOM_GLUE
  #define NS_CheckThreadSafe(owningThread, msg)
#else
  #define NS_CheckThreadSafe(owningThread, msg)                 \
    if (MOZ_UNLIKELY(owningThread != PR_GetCurrentThread())) {  \
      MOZ_CRASH(msg);                                           \
    }
#endif

#ifdef MOZILLA_INTERNAL_API
void NS_ABORT_OOM(size_t aSize);
#else
inline void NS_ABORT_OOM(size_t)
{
  MOZ_CRASH();
}
#endif

typedef void (*StderrCallback)(const char* aFmt, va_list aArgs);





#ifdef __cplusplus
extern "C" {
#endif











void printf_stderr(const char* aFmt, ...) MOZ_FORMAT_PRINTF(1, 2);




void vprintf_stderr(const char* aFmt, va_list aArgs);
















void fprintf_stderr(FILE* aFile, const char* aFmt, ...) MOZ_FORMAT_PRINTF(2, 3);



void set_stderr_callback(StderrCallback aCallback);

#if defined(ANDROID) && !defined(RELEASE_BUILD)























void copy_stderr_to_file(const char* aFile);
#endif

#ifdef __cplusplus
}
#endif

#endif
