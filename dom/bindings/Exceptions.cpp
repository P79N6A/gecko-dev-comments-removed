




#include "mozilla/dom/Exceptions.h"

#include "js/GCAPI.h"
#include "js/TypeDecls.h"
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

  if (!GetOrCreateDOMReflector(aCx, aException, &thrown)) {
    return false;
  }

  JS_SetPendingException(aCx, thrown);
  return true;
}

bool
Throw(JSContext* aCx, nsresult aRv, const char* aMessage)
{
  if (aRv == NS_ERROR_UNCATCHABLE_EXCEPTION) {
    
    JS_ClearPendingException(aCx);
    return false;
  }

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
  MOZ_ASSERT(aRv != NS_ERROR_UNCATCHABLE_EXCEPTION,
             "Doesn't make sense to report uncatchable exceptions!");
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
  case NS_ERROR_MODULE_DOM_BLUETOOTH:
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

AutoForceSetExceptionOnContext::AutoForceSetExceptionOnContext(JSContext* aCx)
  : mCx(aCx)
{
  mOldValue = JS::ContextOptionsRef(mCx).autoJSAPIOwnsErrorReporting();
  JS::ContextOptionsRef(mCx).setAutoJSAPIOwnsErrorReporting(true);
}

AutoForceSetExceptionOnContext::~AutoForceSetExceptionOnContext()
{
  JS::ContextOptionsRef(mCx).setAutoJSAPIOwnsErrorReporting(mOldValue);
}

namespace exceptions {

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
    , mColNo(0)
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

  virtual nsresult GetLineno(int32_t* aLineNo)
  {
    *aLineNo = mLineno;
    return NS_OK;
  }

  virtual nsresult GetColNo(int32_t* aColNo)
  {
    *aColNo = mColNo;
    return NS_OK;
  }

  nsCOMPtr<nsIStackFrame> mCaller;
  nsString mFilename;
  nsString mFunname;
  int32_t mLineno;
  int32_t mColNo;
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
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(JSStackFrame,
                                                         StackFrame)

  
  explicit JSStackFrame(JS::Handle<JSObject*> aStack);

  static already_AddRefed<nsIStackFrame>
  CreateStack(JSContext* aCx, int32_t aMaxDepth = -1);

  NS_IMETHOD GetLanguageName(nsACString& aLanguageName) MOZ_OVERRIDE;
  NS_IMETHOD GetFilename(nsAString& aFilename) MOZ_OVERRIDE;
  NS_IMETHOD GetName(nsAString& aFunction) MOZ_OVERRIDE;
  NS_IMETHOD GetCaller(nsIStackFrame** aCaller) MOZ_OVERRIDE;
  NS_IMETHOD GetFormattedStack(nsAString& aStack) MOZ_OVERRIDE;
  virtual bool CallerSubsumes(JSContext* aCx) MOZ_OVERRIDE;
  NS_IMETHOD GetSanitized(JSContext* aCx,
                          nsIStackFrame** aSanitized) MOZ_OVERRIDE;

protected:
  virtual bool IsJSFrame() const MOZ_OVERRIDE {
    return true;
  }

  virtual nsresult GetLineno(int32_t* aLineNo) MOZ_OVERRIDE;
  virtual nsresult GetColNo(int32_t* aColNo) MOZ_OVERRIDE;

private:
  virtual ~JSStackFrame();

  JS::Heap<JSObject*> mStack;
  nsString mFormattedStack;

  bool mFilenameInitialized;
  bool mFunnameInitialized;
  bool mLinenoInitialized;
  bool mColNoInitialized;
  bool mCallerInitialized;
  bool mFormattedStackInitialized;
};

JSStackFrame::JSStackFrame(JS::Handle<JSObject*> aStack)
  : mStack(aStack)
  , mFilenameInitialized(false)
  , mFunnameInitialized(false)
  , mLinenoInitialized(false)
  , mColNoInitialized(false)
  , mCallerInitialized(false)
  , mFormattedStackInitialized(false)
{
  MOZ_ASSERT(mStack);

  mozilla::HoldJSObjects(this);
  mLineno = 0;
  mLanguage = nsIProgrammingLanguage::JAVASCRIPT;
}

JSStackFrame::~JSStackFrame()
{
  mozilla::DropJSObjects(this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(JSStackFrame)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(JSStackFrame, StackFrame)
  tmp->mStack = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(JSStackFrame, StackFrame)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(JSStackFrame, StackFrame)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mStack)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

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












template<typename ReturnType, typename GetterOutParamType>
static void
GetValueIfNotCached(JSContext* aCx, JSObject* aStack,
                    JS::SavedFrameResult (*aPropGetter)(JSContext*,
                                                        JS::Handle<JSObject*>,
                                                        GetterOutParamType),
                    bool aIsCached, bool* aCanCache, bool* aUseCachedValue,
                    ReturnType aValue)
{
  MOZ_ASSERT(aStack);

  JS::Rooted<JSObject*> stack(aCx, aStack);
  
  
  *aCanCache = js::GetContextCompartment(aCx) == js::GetObjectCompartment(stack);
  if (*aCanCache && aIsCached) {
    *aUseCachedValue = true;
    return;
  }

  *aUseCachedValue = false;
  JS::ExposeObjectToActiveJS(stack);

  aPropGetter(aCx, stack, aValue);
}


NS_IMETHODIMP JSStackFrame::GetFilename(nsAString& aFilename)
{
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  ThreadsafeAutoJSContext cx;
  JS::Rooted<JSString*> filename(cx);
  bool canCache = false, useCachedValue = false;
  GetValueIfNotCached(cx, mStack, JS::GetSavedFrameSource, mFilenameInitialized,
                      &canCache, &useCachedValue, &filename);
  if (useCachedValue) {
    return StackFrame::GetFilename(aFilename);
  }

  nsAutoJSString str;
  if (!str.init(cx, filename)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aFilename = str;

  if (canCache) {
    mFilename = str;
    mFilenameInitialized = true;
  }

  return NS_OK;
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
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  ThreadsafeAutoJSContext cx;
  JS::Rooted<JSString*> name(cx);
  bool canCache = false, useCachedValue = false;
  GetValueIfNotCached(cx, mStack, JS::GetSavedFrameFunctionDisplayName,
                      mFunnameInitialized, &canCache, &useCachedValue,
                      &name);

  if (useCachedValue) {
    return StackFrame::GetName(aFunction);
  }

  if (name) {
    nsAutoJSString str;
    if (!str.init(cx, name)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aFunction = str;
  } else {
    aFunction.SetIsVoid(true);
  }

  if (canCache) {
    mFunname = aFunction;
    mFunnameInitialized = true;
  }

  return NS_OK;
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


nsresult
JSStackFrame::GetLineno(int32_t* aLineNo)
{
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  ThreadsafeAutoJSContext cx;
  uint32_t line;
  bool canCache = false, useCachedValue = false;
  GetValueIfNotCached(cx, mStack, JS::GetSavedFrameLine, mLinenoInitialized,
                      &canCache, &useCachedValue, &line);

  if (useCachedValue) {
    return StackFrame::GetLineno(aLineNo);
  }

  *aLineNo = line;

  if (canCache) {
    mLineno = line;
    mLinenoInitialized = true;
  }

  return NS_OK;
}


NS_IMETHODIMP StackFrame::GetLineNumber(int32_t* aLineNumber)
{
  return GetLineno(aLineNumber);
}


nsresult
JSStackFrame::GetColNo(int32_t* aColNo)
{
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  ThreadsafeAutoJSContext cx;
  uint32_t col;
  bool canCache = false, useCachedValue = false;
  GetValueIfNotCached(cx, mStack, JS::GetSavedFrameColumn, mColNoInitialized,
                      &canCache, &useCachedValue, &col);

  if (useCachedValue) {
    return StackFrame::GetColNo(aColNo);
  }

  *aColNo = col;

  if (canCache) {
    mColNo = col;
    mColNoInitialized = true;
  }

  return NS_OK;
}


NS_IMETHODIMP StackFrame::GetColumnNumber(int32_t* aColumnNumber)
{
  return GetColNo(aColumnNumber);
}


NS_IMETHODIMP StackFrame::GetSourceLine(nsACString& aSourceLine)
{
  aSourceLine.Truncate();
  return NS_OK;
}


NS_IMETHODIMP StackFrame::GetSanitized(JSContext*, nsIStackFrame** aSanitized)
{
  NS_ADDREF(*aSanitized = this);
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetSanitized(JSContext* aCx, nsIStackFrame** aSanitized)
{
  
  
  

  JS::RootedObject savedFrame(aCx, mStack);
  JS::ExposeObjectToActiveJS(mStack);

  savedFrame = js::GetFirstSubsumedSavedFrame(aCx, savedFrame);
  nsCOMPtr<nsIStackFrame> stackFrame;
  if (savedFrame) {
    stackFrame = new JSStackFrame(savedFrame);
  } else {
    stackFrame = new StackFrame();
  }

  stackFrame.forget(aSanitized);
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetCaller(nsIStackFrame** aCaller)
{
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  ThreadsafeAutoJSContext cx;
  JS::Rooted<JSObject*> callerObj(cx);
  bool canCache = false, useCachedValue = false;
  GetValueIfNotCached(cx, mStack, JS::GetSavedFrameParent, mCallerInitialized,
                      &canCache, &useCachedValue, &callerObj);

  if (useCachedValue) {
    return StackFrame::GetCaller(aCaller);
  }

  nsCOMPtr<nsIStackFrame> caller;
  if (callerObj) {
      caller = new JSStackFrame(callerObj);
  } else {
    
    
    
    caller = new StackFrame();
  }
  caller.forget(aCaller);

  if (canCache) {
    mCaller = *aCaller;
    mCallerInitialized = true;
  }

  return NS_OK;
}

NS_IMETHODIMP StackFrame::GetCaller(nsIStackFrame** aCaller)
{
  NS_IF_ADDREF(*aCaller = mCaller);
  return NS_OK;
}

NS_IMETHODIMP JSStackFrame::GetFormattedStack(nsAString& aStack)
{
  NS_ENSURE_TRUE(mStack, NS_ERROR_NOT_AVAILABLE);
  
  
  
  
  ThreadsafeAutoJSContext cx;

  
  
  bool canCache =
    js::GetContextCompartment(cx) == js::GetObjectCompartment(mStack);
  if (canCache && mFormattedStackInitialized) {
    aStack = mFormattedStack;
    return NS_OK;
  }

  JS::ExposeObjectToActiveJS(mStack);
  JS::Rooted<JSObject*> stack(cx, mStack);

  JS::Rooted<JSString*> formattedStack(cx);
  if (!JS::StringifySavedFrameStack(cx, stack, &formattedStack)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoJSString str;
  if (!str.init(cx, formattedStack)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aStack = str;

  if (canCache) {
    mFormattedStack = str;
    mFormattedStackInitialized = true;
  }

  return NS_OK;
}

NS_IMETHODIMP StackFrame::GetFormattedStack(nsAString& aStack)
{
  aStack.Truncate();
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

  int32_t lineno;
  rv = GetLineno(&lineno);
  NS_ENSURE_SUCCESS(rv, rv);

  static const char format[] = "%s frame :: %s :: %s :: line %d";
  _retval.AppendPrintf(format, frametype,
                       NS_ConvertUTF16toUTF8(filename).get(),
                       NS_ConvertUTF16toUTF8(funname).get(),
                       lineno);
  return NS_OK;
}

 bool
StackFrame::CallerSubsumes(JSContext* aCx)
{
  return true;
}

 bool
JSStackFrame::CallerSubsumes(JSContext* aCx)
{
  if (!NS_IsMainThread()) {
    return true;
  }

  if (!mStack) {
    
    return true;
  }

  nsIPrincipal* callerPrincipal = nsContentUtils::SubjectPrincipal();

  JS::Rooted<JSObject*> unwrappedStack(aCx, js::CheckedUnwrap(mStack));
  if (!unwrappedStack) {
    
    return true;
  }

  nsIPrincipal* stackPrincipal =
    nsJSPrincipals::get(js::GetSavedFramePrincipals(unwrappedStack));
  return callerPrincipal->SubsumesConsideringDomain(stackPrincipal);
}

 already_AddRefed<nsIStackFrame>
JSStackFrame::CreateStack(JSContext* aCx, int32_t aMaxDepth)
{
  static const unsigned MAX_FRAMES = 100;
  if (aMaxDepth < 0) {
    aMaxDepth = MAX_FRAMES;
  }

  JS::Rooted<JSObject*> stack(aCx);
  if (!JS::CaptureCurrentStack(aCx, &stack, aMaxDepth)) {
    return nullptr;
  }

  nsCOMPtr<nsIStackFrame> first;
  if (!stack) {
    first = new StackFrame();
  } else {
    first = new JSStackFrame(stack);
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
