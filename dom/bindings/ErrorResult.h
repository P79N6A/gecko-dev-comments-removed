









#ifndef mozilla_ErrorResult_h
#define mozilla_ErrorResult_h

#include <stdarg.h>

#include "js/Value.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "mozilla/Assertions.h"

namespace mozilla {

namespace dom {

enum ErrNum {
#define MSG_DEF(_name, _argc, _str) \
  _name,
#include "mozilla/dom/Errors.msg"
#undef MSG_DEF
  Err_Limit
};

} 

class ErrorResult {
public:
  ErrorResult() {
    mResult = NS_OK;
#ifdef DEBUG
    mMightHaveUnreportedJSException = false;
#endif
  }

#ifdef DEBUG
  ~ErrorResult() {
    MOZ_ASSERT_IF(IsTypeError(), !mMessage);
    MOZ_ASSERT(!mMightHaveUnreportedJSException);
  }
#endif

  void Throw(nsresult rv) {
    MOZ_ASSERT(NS_FAILED(rv), "Please don't try throwing success");
    MOZ_ASSERT(rv != NS_ERROR_TYPE_ERR, "Use ThrowTypeError()");
    MOZ_ASSERT(!IsTypeError(), "Don't overwite TypeError");
    MOZ_ASSERT(rv != NS_ERROR_DOM_JS_EXCEPTION, "Use ThrowJSException()");
    MOZ_ASSERT(!IsJSException(), "Don't overwrite JS exceptions");
    MOZ_ASSERT(rv != NS_ERROR_XPC_NOT_ENOUGH_ARGS, "Use ThrowNotEnoughArgsError()");
    MOZ_ASSERT(!IsNotEnoughArgsError(), "Don't overwrite not enough args error");
    mResult = rv;
  }

  void ThrowTypeError(const dom::ErrNum errorNumber, ...);
  void ReportTypeError(JSContext* cx);
  void ClearMessage();
  bool IsTypeError() const { return ErrorCode() == NS_ERROR_TYPE_ERR; }

  
  
  
  
  
  
  void ThrowJSException(JSContext* cx, JS::Handle<JS::Value> exn);
  void ReportJSException(JSContext* cx);
  bool IsJSException() const { return ErrorCode() == NS_ERROR_DOM_JS_EXCEPTION; }

  void ThrowNotEnoughArgsError() { mResult = NS_ERROR_XPC_NOT_ENOUGH_ARGS; }
  void ReportNotEnoughArgsError(JSContext* cx,
                                const char* ifaceName,
                                const char* memberName);
  bool IsNotEnoughArgsError() const { return ErrorCode() == NS_ERROR_XPC_NOT_ENOUGH_ARGS; }

  
  
  
  void StealJSException(JSContext* cx, JS::MutableHandle<JS::Value> value);

  void MOZ_ALWAYS_INLINE MightThrowJSException()
  {
#ifdef DEBUG
    mMightHaveUnreportedJSException = true;
#endif
  }
  void MOZ_ALWAYS_INLINE WouldReportJSException()
  {
#ifdef DEBUG
    mMightHaveUnreportedJSException = false;
#endif
  }

  
  
  

  
  
  
  void operator=(nsresult rv) {
    MOZ_ASSERT(rv != NS_ERROR_TYPE_ERR, "Use ThrowTypeError()");
    MOZ_ASSERT(!IsTypeError(), "Don't overwite TypeError");
    MOZ_ASSERT(rv != NS_ERROR_DOM_JS_EXCEPTION, "Use ThrowJSException()");
    MOZ_ASSERT(!IsJSException(), "Don't overwrite JS exceptions");
    MOZ_ASSERT(rv != NS_ERROR_XPC_NOT_ENOUGH_ARGS, "Use ThrowNotEnoughArgsError()");
    MOZ_ASSERT(!IsNotEnoughArgsError(), "Don't overwrite not enough args error");
    mResult = rv;
  }

  bool Failed() const {
    return NS_FAILED(mResult);
  }

  nsresult ErrorCode() const {
    return mResult;
  }

private:
  nsresult mResult;
  struct Message;
  
  
  
  
  union {
    Message* mMessage; 
    JS::Value mJSException; 
  };

#ifdef DEBUG
  
  
  bool mMightHaveUnreportedJSException;
#endif

  
  
  ErrorResult(const ErrorResult&) MOZ_DELETE;
};





#define ENSURE_SUCCESS(res, ret)                                          \
  do {                                                                    \
    if (res.Failed()) {                                                   \
      nsCString msg;                                                      \
      msg.AppendPrintf("ENSURE_SUCCESS(%s, %s) failed with "              \
                       "result 0x%X", #res, #ret, res.ErrorCode());       \
      NS_WARNING(msg.get());                                              \
      return ret;                                                         \
    }                                                                     \
  } while(0)

#define ENSURE_SUCCESS_VOID(res)                                          \
  do {                                                                    \
    if (res.Failed()) {                                                   \
      nsCString msg;                                                      \
      msg.AppendPrintf("ENSURE_SUCCESS_VOID(%s) failed with "             \
                       "result 0x%X", #res, res.ErrorCode());             \
      NS_WARNING(msg.get());                                              \
      return;                                                             \
    }                                                                     \
  } while(0)

} 

#endif 
