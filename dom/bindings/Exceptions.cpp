




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

namespace {



bool
IsCallerChrome()
{
  nsCOMPtr<nsIScriptSecurityManager> secMan;
  secMan = XPCWrapper::GetSecurityManager();

  if (!secMan) {
    return false;
  }

  bool isChrome;
  return NS_SUCCEEDED(secMan->SubjectPrincipalIsSystem(&isChrome)) && isChrome;
}

} 

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
  if (!WrapObject(aCx, glob, aException, &NS_GET_IID(nsIException), &val)) {
    return false;
  }

  JS_SetPendingException(aCx, val);

  return true;
}

bool
ThrowExceptionObject(JSContext* aCx, Exception* aException)
{
  JS::Rooted<JS::Value> thrown(aCx);

  
  
  
  
  
  if (NS_IsMainThread() && !IsCallerChrome() &&
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

  if (!WrapNewBindingObject(aCx, glob, aException, &thrown)) {
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

  nsRefPtr<Exception> finalException;

  
  switch (NS_ERROR_GET_MODULE(aRv)) {
  case NS_ERROR_MODULE_DOM:
  case NS_ERROR_MODULE_SVG:
  case NS_ERROR_MODULE_DOM_XPATH:
  case NS_ERROR_MODULE_DOM_INDEXEDDB:
  case NS_ERROR_MODULE_DOM_FILEHANDLE:
    finalException = DOMException::Create(aRv);
    break;

  default:
      break;
  }

  
  if (!finalException) {
    finalException = new Exception(aMessage, aRv, nullptr, nullptr, nullptr);
  }

  MOZ_ASSERT(finalException);
  if (!ThrowExceptionObject(aCx, finalException)) {
    
    
    JS_ReportOutOfMemory(aCx);
  }

  return false;
}

already_AddRefed<nsIStackFrame>
GetCurrentJSStack()
{
  
  JSContext* cx = nullptr;

  if (NS_IsMainThread()) {
    
    
    if (nsContentUtils::XPConnect()) {
      cx = nsContentUtils::XPConnect()->GetCurrentJSContext();
    } else {
      nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
      cx = xpc->GetCurrentJSContext();
    }
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

class JSStackFrame : public nsIStackFrame
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTACKFRAME

  JSStackFrame();
  virtual ~JSStackFrame();

  static already_AddRefed<nsIStackFrame>
  CreateStack(JSContext* cx);
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

  nsCOMPtr<nsIStackFrame> mCaller;

  char* mFilename;
  char* mFunname;
  int32_t mLineno;
  uint32_t mLanguage;
};

JSStackFrame::JSStackFrame()
  : mFilename(nullptr),
    mFunname(nullptr),
    mLineno(0),
    mLanguage(nsIProgrammingLanguage::UNKNOWN)
{}

JSStackFrame::~JSStackFrame()
{
  if (mFilename) {
    nsMemory::Free(mFilename);
  }
  if (mFunname) {
    nsMemory::Free(mFunname);
  }
}

NS_IMPL_ISUPPORTS1(JSStackFrame, nsIStackFrame)


NS_IMETHODIMP JSStackFrame::GetLanguage(uint32_t* aLanguage)
{
  *aLanguage = mLanguage;
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetLanguageName(char** aLanguageName)
{
  static const char js[] = "JavaScript";
  static const char cpp[] = "C++";

  if (IsJSFrame()) {
    *aLanguageName = (char*) nsMemory::Clone(js, sizeof(js));
  } else {
    *aLanguageName = (char*) nsMemory::Clone(cpp, sizeof(cpp));
  }

  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetFilename(char** aFilename)
{
  NS_ENSURE_ARG_POINTER(aFilename);

  if (mFilename) {
    *aFilename = (char*) nsMemory::Clone(mFilename,
                                         sizeof(char)*(strlen(mFilename)+1));
  } else {
    *aFilename = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetName(char** aFunction)
{
  NS_ENSURE_ARG_POINTER(aFunction);

  if (mFunname) {
    *aFunction = (char*) nsMemory::Clone(mFunname,
                                         sizeof(char)*(strlen(mFunname)+1));
  } else {
    *aFunction = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetLineNumber(int32_t* aLineNumber)
{
  *aLineNumber = mLineno;
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetSourceLine(char** aSourceLine)
{
  *aSourceLine = nullptr;
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::GetCaller(nsIStackFrame** aCaller)
{
  NS_IF_ADDREF(*aCaller = mCaller);
  return NS_OK;
}


NS_IMETHODIMP JSStackFrame::ToString(char** _retval)
{
  const char* frametype = IsJSFrame() ? "JS" : "native";
  const char* filename = mFilename ? mFilename : "<unknown filename>";
  const char* funname = mFunname ? mFunname : "<TOP_LEVEL>";
  static const char format[] = "%s frame :: %s :: %s :: line %d";
  int len = sizeof(char)*
              (strlen(frametype) + strlen(filename) + strlen(funname)) +
            sizeof(format) + 3 * sizeof(mLineno);

  char* buf = (char*) nsMemory::Alloc(len);
  JS_snprintf(buf, len, format, frametype, filename, funname, mLineno);
  *_retval = buf;
  return NS_OK;
}

 already_AddRefed<nsIStackFrame>
JSStackFrame::CreateStack(JSContext* cx)
{
  static const unsigned MAX_FRAMES = 100;

  nsRefPtr<JSStackFrame> first = new JSStackFrame();
  nsRefPtr<JSStackFrame> self = first;

  JS::StackDescription* desc = JS::DescribeStack(cx, MAX_FRAMES);
  if (!desc) {
    return nullptr;
  }

  for (size_t i = 0; i < desc->nframes && self; i++) {
    self->mLanguage = nsIProgrammingLanguage::JAVASCRIPT;

    JSAutoCompartment ac(cx, desc->frames[i].script());
    const char* filename = JS_GetScriptFilename(cx, desc->frames[i].script());
    if (filename) {
      self->mFilename =
        (char*)nsMemory::Clone(filename, sizeof(char)*(strlen(filename)+1));
    }

    self->mLineno = desc->frames[i].lineno();

    JSFunction* fun = desc->frames[i].fun();
    if (fun) {
      JS::Rooted<JSString*> funid(cx, JS_GetFunctionDisplayId(fun));
      if (funid) {
        size_t length = JS_GetStringEncodingLength(cx, funid);
        if (length != size_t(-1)) {
          self->mFunname = static_cast<char *>(nsMemory::Alloc(length + 1));
          if (self->mFunname) {
            JS_EncodeStringToBuffer(cx, funid, self->mFunname, length);
            self->mFunname[length] = '\0';
          }
        }
      }
    }

    nsRefPtr<JSStackFrame> frame = new JSStackFrame();
    self->mCaller = frame;
    self.swap(frame);
  }

  JS::FreeStackDescription(cx, desc);

  return first.forget();
}

 already_AddRefed<nsIStackFrame>
JSStackFrame::CreateStackFrameLocation(uint32_t aLanguage,
                                       const char* aFilename,
                                       const char* aFunctionName,
                                       int32_t aLineNumber,
                                       nsIStackFrame* aCaller)
{
  nsRefPtr<JSStackFrame> self = new JSStackFrame();

  self->mLanguage = aLanguage;
  self->mLineno = aLineNumber;

  if (aFilename) {
    self->mFilename =
      (char*)nsMemory::Clone(aFilename, sizeof(char)*(strlen(aFilename)+1));
  }

  if (aFunctionName) {
    self->mFunname = 
      (char*)nsMemory::Clone(aFunctionName,
                             sizeof(char)*(strlen(aFunctionName)+1));
  }

  self->mCaller = aCaller;

  return self.forget();
}

already_AddRefed<nsIStackFrame>
CreateStack(JSContext* cx)
{
  return JSStackFrame::CreateStack(cx);
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
