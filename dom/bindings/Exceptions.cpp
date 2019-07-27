




#include "mozilla/dom/Exceptions.h"

#include "js/GCAPI.h"
#include "js/OldDebugAPI.h"
#include "jsapi.h"
#include "jsprf.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsPIDOMWindow.h"
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

void
ThrowAndReport(nsPIDOMWindow* aWindow, nsresult aRv, const char* aMessage)
{
  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.InitWithLegacyErrorReporting(aWindow))) {
    return;
  }

  Throw(jsapi.cx(), aRv, aMessage);
  (void) JS_ReportPendingException(jsapi.cx());
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

protected:
  ~StackDescriptionOwner()
  {
    
    
    
    if (mDescription) {
      JS::FreeStackDescription(nullptr, mDescription);
      mDescription = nullptr;
    }
    mozilla::DropJSObjects(this);
  }

public:
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

class StackFrame : public nsIStackFrame
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(StackFrame)
  NS_DECL_NSISTACKFRAME

  StackFrame(uint32_t aLanguage,
             const char* aFilename,
             const char* aFunctionName,
             int32_t aLineNumber,
             nsIStackFrame* aCaller);

  StackFrame()
    : mLineno(0)
    , mLanguage(nsIProgrammingLanguage::UNKNOWN)
  {
  }

  static already_AddRefed<nsIStackFrame>
  CreateStackFrameLocation(uint32_t aLanguage,
                           const char* aFilename,
                           const char* aFunctionName,
                           int32_t aLineNumber,
                           nsIStackFrame* aCaller);
protected:
  virtual ~StackFrame();

  virtual bool IsJSFrame() const
  {
    return false;
  }

  virtual int32_t GetLineno()
  {
    return mLineno;
  }

  nsCOMPtr<nsIStackFrame> mCaller;
  nsString mFilename;
  nsString mFunname;
  int32_t mLineno;
  uint32_t mLanguage;
};

StackFrame::StackFrame(uint32_t aLanguage,
                       const char* aFilename,
                       const char* aFunctionName,
                       int32_t aLineNumber,
                       nsIStackFrame* aCaller)
  : mCaller(aCaller)
  , mLineno(aLineNumber)
  , mLanguage(aLanguage)
{
  CopyUTF8toUTF16(aFilename, mFilename);
  CopyUTF8toUTF16(aFunctionName, mFunname);
}

StackFrame::~StackFrame()
{
}

NS_IMPL_CYCLE_COLLECTION(StackFrame, mCaller)
NS_IMPL_CYCLE_COLLECTING_ADDREF(StackFrame)
NS_IMPL_CYCLE_COLLECTING_RELEASE(StackFrame)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(StackFrame)
  NS_INTERFACE_MAP_ENTRY(nsIStackFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

class JSStackFrame : public StackFrame
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(JSStackFrame, StackFrame)

  
  
  JSStackFrame(StackDescriptionOwner* aStackDescription, size_t aIndex);

  static already_AddRefed<nsIStackFrame>
  CreateStack(JSContext* aCx, int32_t aMaxDepth = -1);

  NS_IMETHOD GetLanguageName(nsACString& aLanguageName) MOZ_OVERRIDE;
  NS_IMETHOD GetFilename(nsAString& aFilename) MOZ_OVERRIDE;
  NS_IMETHOD GetName(nsAString& aFunction) MOZ_OVERRIDE;
  NS_IMETHOD GetCaller(nsIStackFrame** aCaller) MOZ_OVERRIDE;

protected:
  virtual bool IsJSFrame() const MOZ_OVERRIDE {
    return true;
  }

  virtual int32_t GetLineno() MOZ_OVERRIDE;

private:
  virtual ~JSStackFrame();

  nsRefPtr<StackDescriptionOwner> mStackDescription;
  size_t mIndex;

  bool mFilenameInitialized;
  bool mFunnameInitialized;
  bool mLinenoInitialized;
  bool mCallerInitialized;
};

JSStackFrame::JSStackFrame(StackDescriptionOwner* aStackDescription,
                           size_t aIndex)
  : mStackDescription(aStackDescription)
  , mIndex(aIndex)
  , mFilenameInitialized(false)
  , mFunnameInitialized(false)
  , mLinenoInitialized(false)
  , mCallerInitialized(false)
{
  MOZ_ASSERT(aStackDescription && aIndex < aStackDescription->NumFrames());

  mLineno = 0;
  mLanguage = nsIProgrammingLanguage::JAVASCRIPT;
}

JSStackFrame::~JSStackFrame()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(JSStackFrame, StackFrame, mStackDescription)

NS_IMPL_ADDREF_INHERITED(JSStackFrame, StackFrame)
NS_IMPL_RELEASE_INHERITED(JSStackFrame, StackFrame)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(JSStackFrame)
NS_INTERFACE_MAP_END_INHERITING(StackFrame)


NS_IMETHODIMP StackFrame::GetLanguage(uint32_t* aLanguage)
{
  *aLanguage = mLanguage;
  return NS_OK;
}


NS_IMETHODIMP StackFrame::GetLanguageName(nsACString& aLanguageName)
{
  aLanguageName.AssignLiteral("C++");
  return NS_OK;
}

NS_IMETHODIMP JSStackFrame::GetLanguageName(nsACString& aLanguageName)
{
  aLanguageName.AssignLiteral("JavaScript");
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

  return StackFrame::GetFilename(aFilename);
}

NS_IMETHODIMP StackFrame::GetFilename(nsAString& aFilename)
{
  
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

  return StackFrame::GetName(aFunction);
}

NS_IMETHODIMP StackFrame::GetName(nsAString& aFunction)
{
  
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

  return StackFrame::GetLineno();
}


NS_IMETHODIMP StackFrame::GetLineNumber(int32_t* aLineNumber)
{
  *aLineNumber = GetLineno();
  return NS_OK;
}


NS_IMETHODIMP StackFrame::GetSourceLine(nsACString& aSourceLine)
{
  aSourceLine.Truncate();
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetCaller(nsIStackFrame** aCaller)
{
  if (!mCallerInitialized) {
    if (mIndex + 1 < mStackDescription->NumFrames()) {
      mCaller = new JSStackFrame(mStackDescription, mIndex+1);
    } else {
      
      
      
      mCaller = new StackFrame();
    }
    mCallerInitialized = true;
  }
  return StackFrame::GetCaller(aCaller);
}

NS_IMETHODIMP StackFrame::GetCaller(nsIStackFrame** aCaller)
{
  NS_IF_ADDREF(*aCaller = mCaller);
  return NS_OK;
}


NS_IMETHODIMP StackFrame::ToString(nsACString& _retval)
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

  nsCOMPtr<nsIStackFrame> first;
  if (desc->nframes == 0) {
    first = new StackFrame();
  } else {
    nsRefPtr<StackDescriptionOwner> descOwner = new StackDescriptionOwner(desc);
    first = new JSStackFrame(descOwner, 0);
  }
  return first.forget();
}

 already_AddRefed<nsIStackFrame>
StackFrame::CreateStackFrameLocation(uint32_t aLanguage,
                                     const char* aFilename,
                                     const char* aFunctionName,
                                     int32_t aLineNumber,
                                     nsIStackFrame* aCaller)
{
  nsRefPtr<StackFrame> self =
    new StackFrame(aLanguage, aFilename, aFunctionName, aLineNumber, aCaller);
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
  return StackFrame::CreateStackFrameLocation(aLanguage, aFilename,
                                              aFunctionName, aLineNumber,
                                              aCaller);
}

} 
} 
} 
