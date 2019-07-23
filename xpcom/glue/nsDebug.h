




































#ifndef nsDebug_h___
#define nsDebug_h___

#ifndef nscore_h___
#include "nscore.h"
#endif

#ifndef nsError_h__
#include "nsError.h"
#endif 

#include "nsXPCOM.h"

#ifdef DEBUG
#define NS_DEBUG
#endif

#ifdef DEBUG

















#define NS_ABORT_IF_FALSE(_expr, _msg)                        \
  PR_BEGIN_MACRO                                              \
    if (!(_expr)) {                                           \
      NS_DebugBreak(NS_DEBUG_ASSERTION, _msg, #_expr, __FILE__, __LINE__); \
    }                                                         \
  PR_END_MACRO









#define NS_WARN_IF_FALSE(_expr,_msg)                          \
  PR_BEGIN_MACRO                                              \
    if (!(_expr)) {                                           \
      NS_DebugBreak(NS_DEBUG_WARNING, _msg, #_expr, __FILE__, __LINE__); \
    }                                                         \
  PR_END_MACRO





#define NS_PRECONDITION(expr, str)                            \
  PR_BEGIN_MACRO                                              \
    if (!(expr)) {                                            \
      NS_DebugBreak(NS_DEBUG_ASSERTION, str, #expr, __FILE__, __LINE__); \
    }                                                         \
  PR_END_MACRO





#define NS_ASSERTION(expr, str)                               \
  PR_BEGIN_MACRO                                              \
    if (!(expr)) {                                            \
      NS_DebugBreak(NS_DEBUG_ASSERTION, str, #expr, __FILE__, __LINE__); \
    }                                                         \
  PR_END_MACRO





#define NS_POSTCONDITION(expr, str)                           \
  PR_BEGIN_MACRO                                              \
    if (!(expr)) {                                            \
      NS_DebugBreak(NS_DEBUG_ASSERTION, str, #expr, __FILE__, __LINE__); \
    }                                                         \
  PR_END_MACRO





#define NS_NOTYETIMPLEMENTED(str)                             \
  NS_DebugBreak(NS_DEBUG_ASSERTION, str, "NotYetImplemented", __FILE__, __LINE__)





#define NS_NOTREACHED(str)                                    \
  NS_DebugBreak(NS_DEBUG_ASSERTION, str, "Not Reached", __FILE__, __LINE__)




#define NS_ERROR(str)                                         \
  NS_DebugBreak(NS_DEBUG_ASSERTION, str, "Error", __FILE__, __LINE__)




#define NS_WARNING(str)                                       \
  NS_DebugBreak(NS_DEBUG_WARNING, str, nsnull, __FILE__, __LINE__)




#define NS_ABORT()                                            \
  NS_DebugBreak(NS_DEBUG_ABORT, nsnull, nsnull, __FILE__, __LINE__)




#define NS_BREAK()                                            \
  NS_DebugBreak(NS_DEBUG_BREAK, nsnull, nsnull, __FILE__, __LINE__)

#else 





#define NS_ABORT_IF_FALSE(_expr, _msg) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_WARN_IF_FALSE(_expr, _msg)  PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_PRECONDITION(expr, str)     PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_ASSERTION(expr, str)        PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_POSTCONDITION(expr, str)    PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_NOTYETIMPLEMENTED(str)      PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_NOTREACHED(str)             PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_ERROR(str)                  PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_WARNING(str)                PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_ABORT()                     PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define NS_BREAK()                     PR_BEGIN_MACRO /* nothing */ PR_END_MACRO

#endif 






#define NS_ENSURE_TRUE(x, ret)                                \
  PR_BEGIN_MACRO                                              \
    if (NS_UNLIKELY(!(x))) {                                  \
       NS_WARNING("NS_ENSURE_TRUE(" #x ") failed");           \
       return ret;                                            \
    }                                                         \
  PR_END_MACRO

#define NS_ENSURE_FALSE(x, ret)                               \
  NS_ENSURE_TRUE(!(x), ret)





#define NS_ENSURE_SUCCESS(res, ret) \
  NS_ENSURE_TRUE(NS_SUCCEEDED(res), ret)





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

#define NS_ENSURE_PROPER_AGGREGATION(outer, iid)              \
  NS_ENSURE_FALSE(outer && !iid.Equals(NS_GET_IID(nsISupports)), NS_ERROR_INVALID_ARG)



#ifdef XPCOM_GLUE
#define NS_CheckThreadSafe
#else
#define NS_CheckThreadSafe(owningThread, msg)                 \
  NS_ASSERTION(owningThread == PR_GetCurrentThread(), msg)
#endif






PR_BEGIN_EXTERN_C

NS_COM_GLUE void
printf_stderr(const char *fmt, ...);

PR_END_EXTERN_C

#endif 
