







#include "nsString.h"
#include "nsNetCID.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCID.h"
#include "nsCycleCollectionParticipant.h"
#include "nsServiceManagerUtils.h"
#include "nsProxyRelease.h"

#include "nsINativeOSFileInternals.h"
#include "NativeOSFileInternals.h"
#include "mozilla/dom/NativeOSFileInternalsBinding.h"

#include "nsIUnicodeDecoder.h"
#include "nsIEventTarget.h"

#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Scoped.h"
#include "mozilla/HoldDropJSObjects.h"
#include "mozilla/TimeStamp.h"

#include "prio.h"
#include "prerror.h"
#include "private/pprio.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "js/Utility.h"
#include "xpcpublic.h"

#include <algorithm>
#if defined(XP_UNIX)
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#endif 

#if defined(XP_WIN)
#include <windows.h>
#endif 

namespace mozilla {

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPRFileDesc, PRFileDesc, PR_Close)

namespace {









struct ArrayBufferContents {
  



  uint8_t* data;
  


  size_t nbytes;
};




struct ScopedArrayBufferContentsTraits {
  typedef ArrayBufferContents type;
  const static type empty() {
    type result = {0, 0};
    return result;
  }
  const static void release(type ptr) {
    js_free(ptr.data);
    ptr.data = nullptr;
    ptr.nbytes = 0;
  }
};

struct ScopedArrayBufferContents: public Scoped<ScopedArrayBufferContentsTraits> {
  explicit ScopedArrayBufferContents(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM):
    Scoped<ScopedArrayBufferContentsTraits>(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_TO_PARENT)
  { }
  explicit ScopedArrayBufferContents(const ArrayBufferContents& v
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM):
    Scoped<ScopedArrayBufferContentsTraits>(v MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  { }
  ScopedArrayBufferContents& operator=(ArrayBufferContents ptr) {
    Scoped<ScopedArrayBufferContentsTraits>::operator=(ptr);
    return *this;
  }

  






  bool Allocate(uint32_t length) {
    dispose();
    ArrayBufferContents& value = rwget();
    void *ptr = calloc(1, length);
    if (ptr) {
      value.data = (uint8_t *) ptr;
      value.nbytes = length;
      return true;
    }
    return false;
  }
private:
  explicit ScopedArrayBufferContents(ScopedArrayBufferContents& source) MOZ_DELETE;
  ScopedArrayBufferContents& operator=(ScopedArrayBufferContents& source) MOZ_DELETE;
};






#if defined(XP_UNIX)
#define OS_ERROR_NOMEM ENOMEM
#define OS_ERROR_INVAL EINVAL
#define OS_ERROR_TOO_LARGE EFBIG
#define OS_ERROR_RACE EIO
#elif defined(XP_WIN)
#define OS_ERROR_NOMEM ERROR_NOT_ENOUGH_MEMORY
#define OS_ERROR_INVAL ERROR_BAD_ARGUMENTS
#define OS_ERROR_TOO_LARGE ERROR_FILE_TOO_LARGE
#define OS_ERROR_RACE ERROR_SHARING_VIOLATION
#else
#error "We do not have platform-specific constants for this platform"
#endif













class AbstractResult: public nsINativeOSFileResult {
public:
  NS_DECL_NSINATIVEOSFILERESULT
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AbstractResult)

  






  explicit AbstractResult(TimeStamp aStartDate)
    : mStartDate(aStartDate)
  {
    MOZ_ASSERT(NS_IsMainThread());
    mozilla::HoldJSObjects(this);
  }

  







  void Init(TimeStamp aDispatchDate,
            TimeDuration aExecutionDuration) {
    MOZ_ASSERT(!NS_IsMainThread());

    mDispatchDuration = (aDispatchDate - mStartDate);
    mExecutionDuration = aExecutionDuration;
  }

  


  void DropJSData() {
    mCachedResult = JS::UndefinedValue();
  }

protected:
  virtual ~AbstractResult() {
    MOZ_ASSERT(NS_IsMainThread());
    DropJSData();
    mozilla::DropJSObjects(this);
  }

  virtual nsresult GetCacheableResult(JSContext *cx, JS::MutableHandleValue aResult) = 0;

private:
  TimeStamp mStartDate;
  TimeDuration mDispatchDuration;
  TimeDuration mExecutionDuration;
  JS::Heap<JS::Value> mCachedResult;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(AbstractResult)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AbstractResult)

NS_IMPL_CYCLE_COLLECTION_CLASS(AbstractResult)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AbstractResult)
  NS_INTERFACE_MAP_ENTRY(nsINativeOSFileResult)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(AbstractResult)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mCachedResult)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AbstractResult)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AbstractResult)
  tmp->DropJSData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMETHODIMP
AbstractResult::GetDispatchDurationMS(double *aDispatchDuration)
{
  *aDispatchDuration = mDispatchDuration.ToMilliseconds();
  return NS_OK;
}

NS_IMETHODIMP
AbstractResult::GetExecutionDurationMS(double *aExecutionDuration)
{
  *aExecutionDuration = mExecutionDuration.ToMilliseconds();
  return NS_OK;
}

NS_IMETHODIMP
AbstractResult::GetResult(JSContext *cx, JS::MutableHandleValue aResult)
{
  if (mCachedResult.isUndefined()) {
    nsresult rv = GetCacheableResult(cx, aResult);
    if (NS_FAILED(rv)) {
      return rv;
    }
    mCachedResult = aResult;
    return NS_OK;
  }
  aResult.set(mCachedResult);
  return NS_OK;
}







class StringResult MOZ_FINAL : public AbstractResult
{
public:
  explicit StringResult(TimeStamp aStartDate)
    : AbstractResult(aStartDate)
  {
  }

  






  void Init(TimeStamp aDispatchDate,
            TimeDuration aExecutionDuration,
            nsString& aContents) {
    AbstractResult::Init(aDispatchDate, aExecutionDuration);
    mContents = aContents;
  }

protected:
  nsresult GetCacheableResult(JSContext* cx, JS::MutableHandleValue aResult) MOZ_OVERRIDE;

private:
  nsString mContents;
};

nsresult
StringResult::GetCacheableResult(JSContext* cx, JS::MutableHandleValue aResult)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mContents.get());

  
  
  
  if (!xpc::StringToJsval(cx, mContents, aResult)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}








class TypedArrayResult MOZ_FINAL : public AbstractResult
{
public:
  explicit TypedArrayResult(TimeStamp aStartDate)
    : AbstractResult(aStartDate)
  {
  }

  




  void Init(TimeStamp aDispatchDate,
            TimeDuration aExecutionDuration,
            ArrayBufferContents aContents) {
    AbstractResult::Init(aDispatchDate, aExecutionDuration);
    mContents = aContents;
  }

protected:
  nsresult GetCacheableResult(JSContext* cx, JS::MutableHandleValue aResult) MOZ_OVERRIDE;
private:
  ScopedArrayBufferContents mContents;
};

nsresult
TypedArrayResult::GetCacheableResult(JSContext* cx, JS::MutableHandle<JS::Value> aResult)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  

  const ArrayBufferContents& contents = mContents.get();
  MOZ_ASSERT(contents.data);

  JS::Rooted<JSObject*>
    arrayBuffer(cx, JS_NewArrayBufferWithContents(cx, contents.nbytes, contents.data));
  if (!arrayBuffer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Rooted<JSObject*>
    result(cx, JS_NewUint8ArrayWithBuffer(cx, arrayBuffer,
                                          0, contents.nbytes));
  if (!result) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  
  
  JS_updateMallocCounter(cx, contents.nbytes);
  mContents.forget();

  aResult.setObject(*result);
  return NS_OK;
}






class ErrorEvent MOZ_FINAL : public nsRunnable {
public:
  














  ErrorEvent(already_AddRefed<nsINativeOSFileSuccessCallback>&& aOnSuccess,
             already_AddRefed<nsINativeOSFileErrorCallback>&& aOnError,
             already_AddRefed<AbstractResult>& aDiscardedResult,
             const nsACString& aOperation,
             int32_t aOSError)
    : mOnSuccess(aOnSuccess)
    , mOnError(aOnError)
    , mDiscardedResult(aDiscardedResult)
    , mOSError(aOSError)
    , mOperation(aOperation)
    {
      MOZ_ASSERT(!NS_IsMainThread());
    }

  NS_METHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    (void)mOnError->Complete(mOperation, mOSError);

    
    mOnSuccess = nullptr;
    mOnError = nullptr;
    mDiscardedResult = nullptr;

    return NS_OK;
  }
 private:
  
  
  
  
  
  nsRefPtr<nsINativeOSFileSuccessCallback> mOnSuccess;
  nsRefPtr<nsINativeOSFileErrorCallback> mOnError;
  nsRefPtr<AbstractResult> mDiscardedResult;
  int32_t mOSError;
  nsCString mOperation;
};




class SuccessEvent MOZ_FINAL : public nsRunnable {
public:
  










  SuccessEvent(already_AddRefed<nsINativeOSFileSuccessCallback>&& aOnSuccess,
               already_AddRefed<nsINativeOSFileErrorCallback>&& aOnError,
               already_AddRefed<nsINativeOSFileResult>& aResult)
    : mOnSuccess(aOnSuccess)
    , mOnError(aOnError)
    , mResult(aResult)
    {
      MOZ_ASSERT(!NS_IsMainThread());
    }

  NS_METHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    (void)mOnSuccess->Complete(mResult);

    
    mOnSuccess = nullptr;
    mOnError = nullptr;
    mResult = nullptr;

    return NS_OK;
  }
 private:
  
  
  
  
  
  nsRefPtr<nsINativeOSFileSuccessCallback> mOnSuccess;
  nsRefPtr<nsINativeOSFileErrorCallback> mOnError;
  nsRefPtr<nsINativeOSFileResult> mResult;
};







class AbstractDoEvent: public nsRunnable {
public:
  AbstractDoEvent(already_AddRefed<nsINativeOSFileSuccessCallback>& aOnSuccess,
                  already_AddRefed<nsINativeOSFileErrorCallback>& aOnError)
    : mOnSuccess(aOnSuccess)
    , mOnError(aOnError)
#if defined(DEBUG)
    , mResolved(false)
#endif 
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  


  void Fail(const nsACString& aOperation,
            already_AddRefed<AbstractResult>&& aDiscardedResult,
            int32_t aOSError = 0) {
    Resolve();
    nsRefPtr<ErrorEvent> event = new ErrorEvent(mOnSuccess.forget(),
                                                mOnError.forget(),
                                                aDiscardedResult,
                                                aOperation,
                                                aOSError);
    nsresult rv = NS_DispatchToMainThread(event);
    if (NS_FAILED(rv)) {
      
      
      
      nsCOMPtr<nsIThread> main = do_GetMainThread();
      NS_ProxyRelease(main, event);
    }
  }

  


  void Succeed(already_AddRefed<nsINativeOSFileResult>&& aResult) {
    Resolve();
    nsRefPtr<SuccessEvent> event = new SuccessEvent(mOnSuccess.forget(),
                                                    mOnError.forget(),
                                                    aResult);
    nsresult rv = NS_DispatchToMainThread(event);
    if (NS_FAILED(rv)) {
      
      
      
      nsCOMPtr<nsIThread> main = do_GetMainThread();
      NS_ProxyRelease(main, event);
    }

  }

private:

  


  void Resolve() {
#if defined(DEBUG)
    MOZ_ASSERT(!mResolved);
    mResolved = true;
#endif 
  }

private:
  nsRefPtr<nsINativeOSFileSuccessCallback> mOnSuccess;
  nsRefPtr<nsINativeOSFileErrorCallback> mOnError;
#if defined(DEBUG)
  
  bool mResolved;
#endif 
};







class AbstractReadEvent: public AbstractDoEvent {
public:
  


  AbstractReadEvent(const nsAString& aPath,
                    const uint64_t aBytes,
                    already_AddRefed<nsINativeOSFileSuccessCallback>& aOnSuccess,
                    already_AddRefed<nsINativeOSFileErrorCallback>& aOnError)
    : AbstractDoEvent(aOnSuccess, aOnError)
    , mPath(aPath)
    , mBytes(aBytes)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_METHOD Run() MOZ_OVERRIDE {
    MOZ_ASSERT(!NS_IsMainThread());
    TimeStamp dispatchDate = TimeStamp::Now();

    nsresult rv = BeforeRead();
    if (NS_FAILED(rv)) {
      
      return NS_OK;
    }

    ScopedArrayBufferContents buffer;
    rv = Read(buffer);
    if (NS_FAILED(rv)) {
      
      return NS_OK;
    }

    AfterRead(dispatchDate, buffer);
    return NS_OK;
  }

 private:
  






  nsresult Read(ScopedArrayBufferContents& aBuffer)
  {
    MOZ_ASSERT(!NS_IsMainThread());

    ScopedPRFileDesc file;
#if defined(XP_WIN)
    
    
    
    
    HANDLE handle =
      ::CreateFileW(mPath.get(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                     nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
      Fail(NS_LITERAL_CSTRING("open"), nullptr, ::GetLastError());
      return NS_ERROR_FAILURE;
    }

    file = PR_ImportFile((PROsfd)handle);
    if (!file) {
      
      Fail(NS_LITERAL_CSTRING("ImportFile"), nullptr, PR_GetOSError());
      return NS_ERROR_FAILURE;
    }

#else
    
    NS_ConvertUTF16toUTF8 path(mPath);
    file = PR_OpenFile(path.get(), PR_RDONLY, 0);
    if (!file) {
      Fail(NS_LITERAL_CSTRING("open"), nullptr, PR_GetOSError());
      return NS_ERROR_FAILURE;
    }

#endif 

    PRFileInfo64 stat;
    if (PR_GetOpenFileInfo64(file, &stat) != PR_SUCCESS) {
      Fail(NS_LITERAL_CSTRING("stat"), nullptr, PR_GetOSError());
      return NS_ERROR_FAILURE;
    }

    uint64_t bytes = std::min((uint64_t)stat.size, mBytes);
    if (bytes > UINT32_MAX) {
      Fail(NS_LITERAL_CSTRING("Arithmetics"), nullptr, OS_ERROR_INVAL);
      return NS_ERROR_FAILURE;
    }

    if (!aBuffer.Allocate(bytes)) {
      Fail(NS_LITERAL_CSTRING("allocate"), nullptr, OS_ERROR_NOMEM);
      return NS_ERROR_FAILURE;
    }

    uint64_t total_read = 0;
    int32_t just_read = 0;
    char* dest_chars = reinterpret_cast<char*>(aBuffer.rwget().data);
    do {
      just_read = PR_Read(file, dest_chars + total_read,
                          std::min(uint64_t(PR_INT32_MAX), bytes - total_read));
      if (just_read == -1) {
        Fail(NS_LITERAL_CSTRING("read"), nullptr, PR_GetOSError());
        return NS_ERROR_FAILURE;
      }
      total_read += just_read;
    } while (just_read != 0 && total_read < bytes);
    if (total_read != bytes) {
      
      Fail(NS_LITERAL_CSTRING("read"), nullptr, OS_ERROR_RACE);
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

protected:
  





  virtual
  nsresult BeforeRead() {
    return NS_OK;
  }

  


  virtual
  void AfterRead(TimeStamp aDispatchDate, ScopedArrayBufferContents& aBuffer) = 0;

 protected:
  const nsString mPath;
  const uint64_t mBytes;
};





class DoReadToTypedArrayEvent MOZ_FINAL : public AbstractReadEvent {
public:
  DoReadToTypedArrayEvent(const nsAString& aPath,
                          const uint32_t aBytes,
                          already_AddRefed<nsINativeOSFileSuccessCallback>&& aOnSuccess,
                          already_AddRefed<nsINativeOSFileErrorCallback>&& aOnError)
    : AbstractReadEvent(aPath, aBytes,
                        aOnSuccess, aOnError)
    , mResult(new TypedArrayResult(TimeStamp::Now()))
  { }

  ~DoReadToTypedArrayEvent() {
    
    
    if (!mResult) {
      return;
    }
    nsCOMPtr<nsIThread> main = do_GetMainThread();
    (void)NS_ProxyRelease(main, mResult);
  }

protected:
  void AfterRead(TimeStamp aDispatchDate,
                 ScopedArrayBufferContents& aBuffer) MOZ_OVERRIDE {
    MOZ_ASSERT(!NS_IsMainThread());
    mResult->Init(aDispatchDate, TimeStamp::Now() - aDispatchDate, aBuffer.forget());
    Succeed(mResult.forget());
  }

 private:
  nsRefPtr<TypedArrayResult> mResult;
};





class DoReadToStringEvent MOZ_FINAL : public AbstractReadEvent {
public:
  DoReadToStringEvent(const nsAString& aPath,
                      const nsACString& aEncoding,
                      const uint32_t aBytes,
                      already_AddRefed<nsINativeOSFileSuccessCallback>&& aOnSuccess,
                      already_AddRefed<nsINativeOSFileErrorCallback>&& aOnError)
    : AbstractReadEvent(aPath, aBytes, aOnSuccess, aOnError)
    , mEncoding(aEncoding)
    , mResult(new StringResult(TimeStamp::Now()))
  { }

  ~DoReadToStringEvent() {
    
    
    if (!mResult) {
      return;
    }
    nsCOMPtr<nsIThread> main = do_GetMainThread();
    (void)NS_ProxyRelease(main, mResult);
  }

protected:
  nsresult BeforeRead() MOZ_OVERRIDE {
    
    
    MOZ_ASSERT(!NS_IsMainThread());
    nsAutoCString encodingName;
    if (!dom::EncodingUtils::FindEncodingForLabel(mEncoding, encodingName)) {
      Fail(NS_LITERAL_CSTRING("Decode"), mResult.forget(), OS_ERROR_INVAL);
      return NS_ERROR_FAILURE;
    }
    mDecoder = dom::EncodingUtils::DecoderForEncoding(encodingName);
    if (!mDecoder) {
      Fail(NS_LITERAL_CSTRING("DecoderForEncoding"), mResult.forget(), OS_ERROR_INVAL);
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  void AfterRead(TimeStamp aDispatchDate,
                 ScopedArrayBufferContents& aBuffer) MOZ_OVERRIDE {
    MOZ_ASSERT(!NS_IsMainThread());

    int32_t maxChars;
    const char* sourceChars = reinterpret_cast<const char*>(aBuffer.get().data);
    int32_t sourceBytes = aBuffer.get().nbytes;
    if (sourceBytes < 0) {
      Fail(NS_LITERAL_CSTRING("arithmetics"), mResult.forget(), OS_ERROR_TOO_LARGE);
      return;
    }

    nsresult rv = mDecoder->GetMaxLength(sourceChars, sourceBytes, &maxChars);
    if (NS_FAILED(rv)) {
      Fail(NS_LITERAL_CSTRING("GetMaxLength"), mResult.forget(), OS_ERROR_INVAL);
      return;
    }

    if (maxChars < 0) {
      Fail(NS_LITERAL_CSTRING("arithmetics"), mResult.forget(), OS_ERROR_TOO_LARGE);
      return;
    }

    nsString resultString;
    resultString.SetLength(maxChars);
    if (resultString.Length() != (nsString::size_type)maxChars) {
      Fail(NS_LITERAL_CSTRING("allocation"), mResult.forget(), OS_ERROR_TOO_LARGE);
      return;
    }


    rv = mDecoder->Convert(sourceChars, &sourceBytes,
                           resultString.BeginWriting(), &maxChars);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    resultString.SetLength(maxChars);

    mResult->Init(aDispatchDate, TimeStamp::Now() - aDispatchDate, resultString);
    Succeed(mResult.forget());
  }

 private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  nsRefPtr<StringResult> mResult;
};

} 



NS_IMPL_ISUPPORTS(NativeOSFileInternalsService, nsINativeOSFileInternalsService);

NS_IMETHODIMP
NativeOSFileInternalsService::Read(const nsAString& aPath,
                                   JS::HandleValue aOptions,
                                   nsINativeOSFileSuccessCallback *aOnSuccess,
                                   nsINativeOSFileErrorCallback *aOnError,
                                   JSContext* cx)
{
  
  nsCString encoding;
  uint64_t bytes = UINT64_MAX;

  if (aOptions.isObject()) {
    dom::NativeOSFileReadOptions dict;
    if (!dict.Init(cx, aOptions)) {
      return NS_ERROR_INVALID_ARG;
    }

    if (dict.mEncoding.WasPassed()) {
      CopyUTF16toUTF8(dict.mEncoding.Value(), encoding);
    }

    if (dict.mBytes.WasPassed() && !dict.mBytes.Value().IsNull()) {
      bytes = dict.mBytes.Value().Value();
    }
  }

  
  nsCOMPtr<nsINativeOSFileSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsINativeOSFileErrorCallback> onError(aOnError);

  nsRefPtr<AbstractDoEvent> event;
  if (encoding.IsEmpty()) {
    event = new DoReadToTypedArrayEvent(aPath, bytes,
                                        onSuccess.forget(),
                                        onError.forget());
  } else {
    event = new DoReadToStringEvent(aPath, encoding, bytes,
                                    onSuccess.forget(),
                                    onError.forget());
  }

  nsresult rv;
  nsCOMPtr<nsIEventTarget> target = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);

  if (NS_FAILED(rv)) {
    return rv;
  }
  return target->Dispatch(event, NS_DISPATCH_NORMAL);
}

} 

