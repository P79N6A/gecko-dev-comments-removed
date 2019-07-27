









#ifndef mozilla_ErrorResult_h
#define mozilla_ErrorResult_h

#include <stdarg.h>

#include "js/Value.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"

namespace IPC {
class Message;
template <typename> struct ParamTraits;
}

namespace mozilla {

namespace dom {

enum ErrNum {
#define MSG_DEF(_name, _argc, _exn, _str) \
  _name,
#include "mozilla/dom/Errors.msg"
#undef MSG_DEF
  Err_Limit
};

bool
ThrowErrorMessage(JSContext* aCx, const ErrNum aErrorNumber, ...);

} 

class ErrorResult {
public:
  ErrorResult() {
    mResult = NS_OK;

#ifdef DEBUG
    mMightHaveUnreportedJSException = false;
    mHasMessage = false;
#endif
  }

#ifdef DEBUG
  ~ErrorResult() {
    MOZ_ASSERT_IF(IsErrorWithMessage(), !mMessage);
    MOZ_ASSERT(!mMightHaveUnreportedJSException);
    MOZ_ASSERT(!mHasMessage);
  }
#endif

  ErrorResult(ErrorResult&& aRHS)
  {
    *this = Move(aRHS);
  }
  ErrorResult& operator=(ErrorResult&& aRHS);

  explicit ErrorResult(nsresult aRv)
    : ErrorResult()
  {
    AssignErrorCode(aRv);
  }

  void Throw(nsresult rv) {
    MOZ_ASSERT(NS_FAILED(rv), "Please don't try throwing success");
    AssignErrorCode(rv);
  }

  
  
  
  void SuppressException();

  
  
  
  nsresult StealNSResult() {
    nsresult rv = ErrorCode();
    SuppressException();
    return rv;
  }

  void ThrowTypeError(const dom::ErrNum errorNumber, ...);
  void ThrowRangeError(const dom::ErrNum errorNumber, ...);
  void ReportErrorWithMessage(JSContext* cx);
  bool IsErrorWithMessage() const { return ErrorCode() == NS_ERROR_TYPE_ERR || ErrorCode() == NS_ERROR_RANGE_ERR; }

  
  
  
  
  
  
  
  
  
  
  void ThrowJSException(JSContext* cx, JS::Handle<JS::Value> exn);
  void ReportJSException(JSContext* cx);
  
  
  void ReportJSExceptionFromJSImplementation(JSContext* aCx);
  bool IsJSException() const { return ErrorCode() == NS_ERROR_DOM_JS_EXCEPTION; }

  void ThrowNotEnoughArgsError() { mResult = NS_ERROR_XPC_NOT_ENOUGH_ARGS; }
  void ReportNotEnoughArgsError(JSContext* cx,
                                const char* ifaceName,
                                const char* memberName);
  bool IsNotEnoughArgsError() const { return ErrorCode() == NS_ERROR_XPC_NOT_ENOUGH_ARGS; }

  
  
  void ReportGenericError(JSContext* cx);

  
  void ThrowUncatchableException() {
    Throw(NS_ERROR_UNCATCHABLE_EXCEPTION);
  }
  bool IsUncatchableException() const {
    return ErrorCode() == NS_ERROR_UNCATCHABLE_EXCEPTION;
  }

  
  
  
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
    AssignErrorCode(rv);
  }

  bool Failed() const {
    return NS_FAILED(mResult);
  }

  bool ErrorCodeIs(nsresult rv) const {
    return mResult == rv;
  }

  
  uint32_t ErrorCodeAsInt() const {
    return static_cast<uint32_t>(ErrorCode());
  }

protected:
  nsresult ErrorCode() const {
    return mResult;
  }

private:
  friend struct IPC::ParamTraits<ErrorResult>;
  void SerializeMessage(IPC::Message* aMsg) const;
  bool DeserializeMessage(const IPC::Message* aMsg, void** aIter);

  void ThrowErrorWithMessage(va_list ap, const dom::ErrNum errorNumber,
                             nsresult errorType);

  void AssignErrorCode(nsresult aRv) {
    MOZ_ASSERT(aRv != NS_ERROR_TYPE_ERR, "Use ThrowTypeError()");
    MOZ_ASSERT(aRv != NS_ERROR_RANGE_ERR, "Use ThrowRangeError()");
    MOZ_ASSERT(!IsErrorWithMessage(), "Don't overwrite errors with message");
    MOZ_ASSERT(aRv != NS_ERROR_DOM_JS_EXCEPTION, "Use ThrowJSException()");
    MOZ_ASSERT(!IsJSException(), "Don't overwrite JS exceptions");
    MOZ_ASSERT(aRv != NS_ERROR_XPC_NOT_ENOUGH_ARGS, "Use ThrowNotEnoughArgsError()");
    MOZ_ASSERT(!IsNotEnoughArgsError(), "Don't overwrite not enough args error");
    mResult = aRv;
  }

  void ClearMessage();

  nsresult mResult;
  struct Message;
  
  
  
  
  union {
    Message* mMessage; 
    JS::Value mJSException; 
  };

#ifdef DEBUG
  
  
  bool mMightHaveUnreportedJSException;
  
  
  
  bool mHasMessage;
#endif

  
  
  ErrorResult(const ErrorResult&) = delete;
  void operator=(const ErrorResult&) = delete;
};





#define ENSURE_SUCCESS(res, ret)                                          \
  do {                                                                    \
    if (res.Failed()) {                                                   \
      nsCString msg;                                                      \
      msg.AppendPrintf("ENSURE_SUCCESS(%s, %s) failed with "              \
                       "result 0x%X", #res, #ret, res.ErrorCodeAsInt());  \
      NS_WARNING(msg.get());                                              \
      return ret;                                                         \
    }                                                                     \
  } while(0)

#define ENSURE_SUCCESS_VOID(res)                                          \
  do {                                                                    \
    if (res.Failed()) {                                                   \
      nsCString msg;                                                      \
      msg.AppendPrintf("ENSURE_SUCCESS_VOID(%s) failed with "             \
                       "result 0x%X", #res, res.ErrorCodeAsInt());        \
      NS_WARNING(msg.get());                                              \
      return;                                                             \
    }                                                                     \
  } while(0)

} 

#endif 
