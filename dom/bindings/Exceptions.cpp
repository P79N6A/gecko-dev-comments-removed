




#include "mozilla/dom/Exceptions.h"

#include "js/GCAPI.h"
#include "js/OldDebugAPI.h"
#include "jsapi.h"
#include "jsprf.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMException.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "XPCWrapper.h"
#include "WorkerPrivate.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

bool
ThrowExceptionObject(JSContext* aCx, nsIException* aException)
{
  
  nsCOMPtr<Exception> exception = do_QueryInterface(aException);
  if (exception) {
    return ThrowExceptionObject(aCx, exception);
  }

  
  
  MOZ_ASSERT(NS_IsMainThread());

  JS::Rooted<JSObject*> glob(aCx, JS::CurrentGlobalOrNull(aCx));
  if (!glob) {
    
    return false;
  }

  JS::Rooted<JS::Value> val(aCx);
  if (!WrapObject(aCx, aException, &NS_GET_IID(nsIException), &val)) {
    return false;
  }

  JS_SetPendingException(aCx, val);

  return true;
}

bool
ThrowExceptionObject(JSContext* aCx, Exception* aException)
{
  JS::Rooted<JS::Value> thrown(aCx);

  
  
  
  
  
  if (NS_IsMainThread() && !nsContentUtils::IsCallerChrome() &&
      aException->StealJSVal(thrown.address())) {
    if (!JS_WrapValue(aCx, &thrown)) {
      return false;
    }
    JS_SetPendingException(aCx, thrown);
    return true;
  }

  JS::Rooted<JSObject*> glob(aCx, JS::CurrentGlobalOrNull(aCx));
  if (!glob) {
    
    return false;
  }

  if (!WrapNewBindingObject(aCx, aException, &thrown)) {
    return false;
  }

  JS_SetPendingException(aCx, thrown);
  return true;
}

bool
Throw(JSContext* aCx, nsresult aRv, const char* aMessage)
{
  if (JS_IsExceptionPending(aCx)) {
    
    return false;
  }

  CycleCollectedJSRuntime* runtime = CycleCollectedJSRuntime::Get();
  nsCOMPtr<nsIException> existingException = runtime->GetPendingException();
  if (existingException) {
    nsresult nr;
    if (NS_SUCCEEDED(existingException->GetResult(&nr)) && 
        aRv == nr) {
      

      
      runtime->SetPendingException(nullptr);

      if (!ThrowExceptionObject(aCx, existingException)) {
        
        
        JS_ReportOutOfMemory(aCx);
      }
      return false;
    }
  }

  nsRefPtr<Exception> finalException = CreateException(aCx, aRv, aMessage);

  MOZ_ASSERT(finalException);
  if (!ThrowExceptionObject(aCx, finalException)) {
    
    
    JS_ReportOutOfMemory(aCx);
  }

  return false;
}

already_AddRefed<Exception>
CreateException(JSContext* aCx, nsresult aRv, const char* aMessage)
{
  
  switch (NS_ERROR_GET_MODULE(aRv)) {
  case NS_ERROR_MODULE_DOM:
  case NS_ERROR_MODULE_SVG:
  case NS_ERROR_MODULE_DOM_XPATH:
  case NS_ERROR_MODULE_DOM_INDEXEDDB:
  case NS_ERROR_MODULE_DOM_FILEHANDLE:
    return DOMException::Create(aRv);
  default:
    break;
  }

  
  
  nsRefPtr<Exception> exception =
    new Exception(nsCString(aMessage), aRv,
                  EmptyCString(), nullptr, nullptr);
  return exception.forget();
}

already_AddRefed<nsIStackFrame>
GetCurrentJSStack()
{
  
  JSContext* cx = nullptr;

  if (NS_IsMainThread()) {
    MOZ_ASSERT(nsContentUtils::XPConnect());
    cx = nsContentUtils::GetCurrentJSContext();
  } else {
    cx = workers::GetCurrentThreadJSContext();
  }

  if (!cx) {
    return nullptr;
  }

  nsCOMPtr<nsIStackFrame> stack = exceptions::CreateStack(cx);
  if (!stack) {
    return nullptr;
  }

  
  uint32_t language;
  nsCOMPtr<nsIStackFrame> caller;
  while (stack &&
         NS_SUCCEEDED(stack->GetLanguage(&language)) &&
         language != nsIProgrammingLanguage::JAVASCRIPT &&
         NS_SUCCEEDED(stack->GetCaller(getter_AddRefs(caller))) &&
         caller) {
    stack = caller;
  }
  return stack.forget();
}

namespace exceptions {

class StackDescriptionOwner {
public:
  StackDescriptionOwner(JS::StackDescription* aDescription)
    : mDescription(aDescription)
  {
    mozilla::HoldJSObjects(this);
  }

  ~StackDescriptionOwner()
  {
    
    
    
    if (mDescription) {
      JS::FreeStackDescription(nullptr, mDescription);
      mDescription = nullptr;
    }
    mozilla::DropJSObjects(this);
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(StackDescriptionOwner)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(StackDescriptionOwner)

  JS::FrameDescription& FrameAt(size_t aIndex)
  {
    MOZ_ASSERT(aIndex < mDescription->nframes);
    return mDescription->frames[aIndex];
  }

  unsigned NumFrames()
  {
    return mDescription->nframes;
  }

private:
  JS::StackDescription* mDescription;
};

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(StackDescriptionOwner, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(StackDescriptionOwner, Release)

NS_IMPL_CYCLE_COLLECTION_CLASS(StackDescriptionOwner)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(StackDescriptionOwner)
  if (tmp->mDescription) {
    JS::FreeStackDescription(nullptr, tmp->mDescription);
    tmp->mDescription = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(StackDescriptionOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(StackDescriptionOwner)
  JS::StackDescription* desc = tmp->mDescription;
  if (tmp->mDescription) {
    for (size_t i = 0; i < desc->nframes; ++i) {
      NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mDescription->frames[i].markedLocation1());
      NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mDescription->frames[i].markedLocation2());
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

class JSStackFrame : public nsIStackFrame
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(JSStackFrame)
  NS_DECL_NSISTACKFRAME

  
  
  
  
  JSStackFrame(StackDescriptionOwner* aStackDescription, size_t aIndex);
  virtual ~JSStackFrame();

  static already_AddRefed<nsIStackFrame>
  CreateStack(JSContext* aCx, int32_t aMaxDepth = -1);
  static already_AddRefed<nsIStackFrame>
  CreateStackFrameLocation(uint32_t aLanguage,
                           const char* aFilename,
                           const char* aFunctionName,
                           int32_t aLineNumber,
                           nsIStackFrame* aCaller);

private:
  bool IsJSFrame() const {
    return mLanguage == nsIProgrammingLanguage::JAVASCRIPT;
  }

  int32_t GetLineno();

  nsRefPtr<StackDescriptionOwner> mStackDescription;
  nsCOMPtr<nsIStackFrame> mCaller;

  
  nsString mFilename;
  nsString mFunname;
  int32_t mLineno;
  uint32_t mLanguage;

  size_t mIndex;

  bool mFilenameInitialized;
  bool mFunnameInitialized;
  bool mLinenoInitialized;
  bool mCallerInitialized;
};

JSStackFrame::JSStackFrame(StackDescriptionOwner* aStackDescription,
                           size_t aIndex)
  : mLineno(0)
{
  if (aStackDescription && aIndex < aStackDescription->NumFrames()) {
    mStackDescription = aStackDescription;
    mIndex = aIndex;
    mFilenameInitialized = false;
    mFunnameInitialized = false;
    mLinenoInitialized = false;
    mCallerInitialized = false;
    mLanguage = nsIProgrammingLanguage::JAVASCRIPT;
  } else {
    MOZ_ASSERT(!mStackDescription);
    mIndex = 0;
    mFilenameInitialized = true;
    mFunnameInitialized = true;
    mLinenoInitialized = true;
    mCallerInitialized = true;
    mLanguage = nsIProgrammingLanguage::UNKNOWN;
  }
}

JSStackFrame::~JSStackFrame()
{
}

NS_IMPL_CYCLE_COLLECTION(JSStackFrame, mStackDescription, mCaller)

NS_IMPL_CYCLE_COLLECTING_ADDREF(JSStackFrame)
NS_IMPL_CYCLE_COLLECTING_RELEASE(JSStackFrame)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(JSStackFrame)
  NS_INTERFACE_MAP_ENTRY(nsIStackFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMETHODIMP JSStackFrame::GetLanguage(uint32_t* aLanguage)
{
  *aLanguage = mLanguage;
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetLanguageName(nsACString& aLanguageName)
{
  static const char js[] = "JavaScript";
  static const char cpp[] = "C++";

  if (IsJSFrame()) {
    aLanguageName.AssignASCII(js);
  } else {
    aLanguageName.AssignASCII(cpp);
  }

  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetFilename(nsAString& aFilename)
{
  if (!mFilenameInitialized) {
    JS::FrameDescription& desc = mStackDescription->FrameAt(mIndex);
    if (const char *filename = desc.filename()) {
      CopyUTF8toUTF16(filename, mFilename);
    }
    mFilenameInitialized = true;
  }

  
  if (mFilename.IsEmpty()) {
    aFilename.SetIsVoid(true);
  } else {
    aFilename.Assign(mFilename);
  }

  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetName(nsAString& aFunction)
{
  if (!mFunnameInitialized) {
    JS::FrameDescription& desc = mStackDescription->FrameAt(mIndex);
    if (JSFlatString *name = desc.funDisplayName()) {
      mFunname.Assign(JS_GetFlatStringChars(name),
                      
                      JS_GetStringLength(JS_FORGET_STRING_FLATNESS(name)));
    }
    mFunnameInitialized = true;
  }

  
  if (mFunname.IsEmpty()) {
    aFunction.SetIsVoid(true);
  } else {
    aFunction.Assign(mFunname);
  }

  return NS_OK;
}

int32_t
JSStackFrame::GetLineno()
{
  if (!mLinenoInitialized) {
    JS::FrameDescription& desc = mStackDescription->FrameAt(mIndex);
    mLineno = desc.lineno();
    mLinenoInitialized = true;
  }

  return mLineno;
}


NS_IMETHODIMP JSStackFrame::GetLineNumber(int32_t* aLineNumber)
{
  *aLineNumber = GetLineno();
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetSourceLine(nsACString& aSourceLine)
{
  aSourceLine.Truncate();
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetCaller(nsIStackFrame** aCaller)
{
  if (!mCallerInitialized) {
    mCaller = new JSStackFrame(mStackDescription, mIndex+1);
    mCallerInitialized = true;
  }
  NS_IF_ADDREF(*aCaller = mCaller);
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::ToString(nsACString& _retval)
{
  _retval.Truncate();

  const char* frametype = IsJSFrame() ? "JS" : "native";

  nsString filename;
  nsresult rv = GetFilename(filename);
  NS_ENSURE_SUCCESS(rv, rv);

  if (filename.IsEmpty()) {
    filename.AssignLiteral("<unknown filename>");
  }

  nsString funname;
  rv = GetName(funname);
  NS_ENSURE_SUCCESS(rv, rv);

  if (funname.IsEmpty()) {
    funname.AssignLiteral("<TOP_LEVEL>");
  }
  static const char format[] = "%s frame :: %s :: %s :: line %d";
  _retval.AppendPrintf(format, frametype,
                       NS_ConvertUTF16toUTF8(filename).get(),
                       NS_ConvertUTF16toUTF8(funname).get(),
                       GetLineno());
  return NS_OK;
}

 already_AddRefed<nsIStackFrame>
JSStackFrame::CreateStack(JSContext* aCx, int32_t aMaxDepth)
{
  static const unsigned MAX_FRAMES = 100;
  if (aMaxDepth < 0) {
    aMaxDepth = MAX_FRAMES;
  }

  JS::StackDescription* desc = JS::DescribeStack(aCx, aMaxDepth);
  if (!desc) {
    return nullptr;
  }

  nsRefPtr<StackDescriptionOwner> descOwner = new StackDescriptionOwner(desc);

  nsRefPtr<JSStackFrame> first = new JSStackFrame(descOwner, 0);
  return first.forget();
}

 already_AddRefed<nsIStackFrame>
JSStackFrame::CreateStackFrameLocation(uint32_t aLanguage,
                                       const char* aFilename,
                                       const char* aFunctionName,
                                       int32_t aLineNumber,
                                       nsIStackFrame* aCaller)
{
  nsRefPtr<JSStackFrame> self = new JSStackFrame(nullptr, 0);

  self->mLanguage = aLanguage;
  self->mLineno = aLineNumber;
  CopyUTF8toUTF16(aFilename, self->mFilename);
  CopyUTF8toUTF16(aFunctionName, self->mFunname);

  self->mCaller = aCaller;

  return self.forget();
}

already_AddRefed<nsIStackFrame>
CreateStack(JSContext* aCx, int32_t aMaxDepth)
{
  return JSStackFrame::CreateStack(aCx, aMaxDepth);
}

already_AddRefed<nsIStackFrame>
CreateStackFrameLocation(uint32_t aLanguage,
                         const char* aFilename,
                         const char* aFunctionName,
                         int32_t aLineNumber,
                         nsIStackFrame* aCaller)
{
  return JSStackFrame::CreateStackFrameLocation(aLanguage, aFilename,
                                                aFunctionName, aLineNumber,
                                                aCaller);
}

} 
} 
} 
