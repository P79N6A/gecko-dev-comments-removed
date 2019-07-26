




#include "mozilla/dom/DOMException.h"

#include "jsapi.h"
#include "jsprf.h"
#include "js/OldDebugAPI.h"
#include "mozilla/Util.h"
#include "nsContentUtils.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionHoldDrop.h"
#include "nsIClassInfoImpl.h"
#include "nsIDocument.h"
#include "nsIDOMDOMException.h"
#include "nsIException.h"
#include "nsMemory.h"
#include "prprf.h"
#include "xpcprivate.h"

#include "mozilla/dom/DOMExceptionBinding.h"

using namespace mozilla;

enum DOM4ErrorTypeCodeMap {
  
  IndexSizeError             = nsIDOMDOMException::INDEX_SIZE_ERR,
  HierarchyRequestError      = nsIDOMDOMException::HIERARCHY_REQUEST_ERR,
  WrongDocumentError         = nsIDOMDOMException::WRONG_DOCUMENT_ERR,
  InvalidCharacterError      = nsIDOMDOMException::INVALID_CHARACTER_ERR,
  NoModificationAllowedError = nsIDOMDOMException::NO_MODIFICATION_ALLOWED_ERR,
  NotFoundError              = nsIDOMDOMException::NOT_FOUND_ERR,
  NotSupportedError          = nsIDOMDOMException::NOT_SUPPORTED_ERR,
  
  InUseAttributeError        = nsIDOMDOMException::INUSE_ATTRIBUTE_ERR,
  InvalidStateError          = nsIDOMDOMException::INVALID_STATE_ERR,
  SyntaxError                = nsIDOMDOMException::SYNTAX_ERR,
  InvalidModificationError   = nsIDOMDOMException::INVALID_MODIFICATION_ERR,
  NamespaceError             = nsIDOMDOMException::NAMESPACE_ERR,
  InvalidAccessError         = nsIDOMDOMException::INVALID_ACCESS_ERR,
  TypeMismatchError          = nsIDOMDOMException::TYPE_MISMATCH_ERR,
  SecurityError              = nsIDOMDOMException::SECURITY_ERR,
  NetworkError               = nsIDOMDOMException::NETWORK_ERR,
  AbortError                 = nsIDOMDOMException::ABORT_ERR,
  URLMismatchError           = nsIDOMDOMException::URL_MISMATCH_ERR,
  QuotaExceededError         = nsIDOMDOMException::QUOTA_EXCEEDED_ERR,
  TimeoutError               = nsIDOMDOMException::TIMEOUT_ERR,
  InvalidNodeTypeError       = nsIDOMDOMException::INVALID_NODE_TYPE_ERR,
  DataCloneError             = nsIDOMDOMException::DATA_CLONE_ERR,
  EncodingError              = 0,

  
  TypeError                  = 0,
  RangeError                 = 0,

  
  UnknownError             = 0,
  ConstraintError          = 0,
  DataError                = 0,
  TransactionInactiveError = 0,
  ReadOnlyError            = 0,
  VersionError             = 0,

  
  NotReadableError         = 0,

  
  LockedFileInactiveError = 0,
};

#define DOM4_MSG_DEF(name, message, nsresult) {(nsresult), name, #name, message},
#define DOM_MSG_DEF(val, message) {(val), NS_ERROR_GET_CODE(val), #val, message},

static const struct ResultStruct
{
  nsresult mNSResult;
  uint16_t mCode;
  const char* mName;
  const char* mMessage;
} sDOMErrorMsgMap[] = {
#include "domerr.msg"
};

#undef DOM4_MSG_DEF
#undef DOM_MSG_DEF

static void
NSResultToNameAndMessage(nsresult aNSResult,
                         const char** aName,
                         const char** aMessage,
                         uint16_t* aCode)
{
  *aName = nullptr;
  *aMessage = nullptr;
  *aCode = 0;
  for (uint32_t idx = 0; idx < ArrayLength(sDOMErrorMsgMap); idx++) {
    if (aNSResult == sDOMErrorMsgMap[idx].mNSResult) {
      *aName = sDOMErrorMsgMap[idx].mName;
      *aMessage = sDOMErrorMsgMap[idx].mMessage;
      *aCode = sDOMErrorMsgMap[idx].mCode;
      return;
    }
  }

  NS_WARNING("Huh, someone is throwing non-DOM errors using the DOM module!");

  return;
}

nsresult
NS_GetNameAndMessageForDOMNSResult(nsresult aNSResult, const char** aName,
                                   const char** aMessage, uint16_t* aCode)
{
  const char* name = nullptr;
  const char* message = nullptr;
  uint16_t code = 0;
  NSResultToNameAndMessage(aNSResult, &name, &message, &code);

  if (name && message) {
    *aName = name;
    *aMessage = message;
    if (aCode) {
      *aCode = code;
    }
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

namespace mozilla {
namespace dom {

bool Exception::sEverMadeOneFromFactory = false;

NS_IMPL_CLASSINFO(Exception, nullptr, nsIClassInfo::DOM_OBJECT,
                  NS_XPCEXCEPTION_CID)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Exception)
  NS_INTERFACE_MAP_ENTRY(nsIException)
  NS_INTERFACE_MAP_ENTRY(nsIXPCException)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIException)
  NS_IMPL_QUERY_CLASSINFO(Exception)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Exception)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Exception)

NS_IMPL_CYCLE_COLLECTION_CLASS(Exception)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Exception)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Exception)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mThrownJSVal);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Exception)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CI_INTERFACE_GETTER1(Exception, nsIXPCException)

Exception::Exception(const char *aMessage,
                     nsresult aResult,
                     const char *aName,
                     nsIStackFrame *aLocation,
                     nsISupports *aData)
: mMessage(nullptr),
  mResult(NS_OK),
  mName(nullptr),
  mLocation(nullptr),
  mData(nullptr),
  mFilename(nullptr),
  mLineNumber(0),
  mInner(nullptr),
  mInitialized(false),
  mHoldingJSVal(false)
{
  SetIsDOMBinding();

  
  
  
  
  
  
  
  if (!sEverMadeOneFromFactory) {
    nsCOMPtr<nsIXPCException> e =
        do_CreateInstance(XPC_EXCEPTION_CONTRACTID);
    sEverMadeOneFromFactory = true;
  }

  nsCOMPtr<nsIStackFrame> location;
  if (aLocation) {
    location = aLocation;
  } else {
    nsXPConnect* xpc = nsXPConnect::XPConnect();
    xpc->GetCurrentJSStack(getter_AddRefs(location));
    
    
    
    
    
  }
  
  if (location) {
    while (1) {
      uint32_t language;
      int32_t lineNumber;
      if (NS_FAILED(location->GetLanguage(&language)) ||
          language == nsIProgrammingLanguage::JAVASCRIPT ||
          NS_FAILED(location->GetLineNumber(&lineNumber)) ||
          lineNumber) {
        break;
      }
      nsCOMPtr<nsIStackFrame> caller;
      if (NS_FAILED(location->GetCaller(getter_AddRefs(caller))) || !caller) {
        break;
      }
      location = caller;
    }
  }

  Initialize(aMessage, aResult, aName, location, aData, nullptr);
}

Exception::Exception()
  : mMessage(nullptr),
    mResult(NS_OK),
    mName(nullptr),
    mFilename(nullptr),
    mLineNumber(-1),
    mInitialized(false),
    mHoldingJSVal(false)
{
}

Exception::~Exception()
{
  if (mMessage) {
    nsMemory::Free(mMessage);
    mMessage = nullptr;
  }
  if (mName) {
    nsMemory::Free(mName);
    mName = nullptr;
  }
  if (mFilename) {
    nsMemory::Free(mFilename);
    mFilename = nullptr;
  }

  if (mHoldingJSVal) {
    MOZ_ASSERT(NS_IsMainThread());

    mozilla::DropJSObjects(this);
  }
}

bool
Exception::StealJSVal(JS::Value* aVp)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mHoldingJSVal) {
    *aVp = mThrownJSVal;
    mThrownJSVal = JSVAL_NULL;

    mozilla::DropJSObjects(this);
    mHoldingJSVal = false;
    return true;
  }

  return false;
}

void
Exception::StowJSVal(JS::Value& aVp)
{
  MOZ_ASSERT(NS_IsMainThread());

  mThrownJSVal = aVp;
  if (!mHoldingJSVal) {
    mozilla::HoldJSObjects(this);
    mHoldingJSVal = true;
  }
}


NS_IMETHODIMP
Exception::GetMessageMoz(char** aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  if (mMessage) {
    *aMessage =
      (char*) nsMemory::Clone(mMessage, sizeof(char)*(strlen(mMessage)+1));
  } else {
    *aMessage = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP
Exception::GetResult(nsresult* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  *aResult = mResult;
  return NS_OK;
}


NS_IMETHODIMP
Exception::GetName(char** aName)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  const char* name = mName;
  if (!name) {
    nsXPCException::NameAndFormatForNSResult(mResult, &name, nullptr);
  }

  if (name) {
    *aName = (char*) nsMemory::Clone(name, sizeof(char)*(strlen(name)+1));
  } else {
    *aName = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP
Exception::GetFilename(char** aFilename)
{
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
    XPC_STRING_GETTER_BODY(aFilename, mFilename);
}


NS_IMETHODIMP
Exception::GetLineNumber(uint32_t *aLineNumber)
{
  NS_ENSURE_ARG_POINTER(aLineNumber);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  *aLineNumber = mLineNumber;
  return NS_OK;
}


NS_IMETHODIMP
Exception::GetColumnNumber(uint32_t* aColumnNumber)
{
  NS_ENSURE_ARG_POINTER(aColumnNumber);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  *aColumnNumber = 0;
  return NS_OK;
}


NS_IMETHODIMP
Exception::GetLocation(nsIStackFrame** aLocation)
{
  NS_ENSURE_ARG_POINTER(aLocation);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIStackFrame> location = mLocation;
  location.forget(aLocation);
  return NS_OK;
}


NS_IMETHODIMP
Exception::GetData(nsISupports** aData)
{
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISupports> data = mData;
  data.forget(aData);
  return NS_OK;
}


NS_IMETHODIMP
Exception::GetInner(nsIException** aException)
{
  NS_ENSURE_ARG_POINTER(aException);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIException> inner = mInner;
  inner.forget(aException);
  return NS_OK;
}


NS_IMETHODIMP
Exception::ToString(char **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);

  static const char defaultMsg[] = "<no message>";
  static const char defaultLocation[] = "<unknown>";
  static const char format[] =
"[Exception... \"%s\"  nsresult: \"0x%x (%s)\"  location: \"%s\"  data: %s]";

  char* indicatedLocation = nullptr;

  if (mLocation) {
    
    nsresult rv = mLocation->ToString(&indicatedLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  const char* msg = mMessage ? mMessage : nullptr;
  const char* location = indicatedLocation ?
                              indicatedLocation : defaultLocation;
  const char* resultName = mName;
  if (!resultName &&
      !nsXPCException::NameAndFormatForNSResult(mResult, &resultName,
                                                (!msg) ? &msg : nullptr)) {
    if (!msg) {
      msg = defaultMsg;
    }
    resultName = "<unknown>";
  }
  const char* data = mData ? "yes" : "no";

  char* temp = JS_smprintf(format, msg, mResult, resultName, location, data);
  if (indicatedLocation) {
    nsMemory::Free(indicatedLocation);
  }

  char* final = nullptr;
  if (temp) {
    final = (char*) nsMemory::Clone(temp, sizeof(char)*(strlen(temp)+1));
    JS_smprintf_free(temp);
  }

  *_retval = final;
  return final ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
Exception::Initialize(const char *aMessage, nsresult aResult, const char *aName, nsIStackFrame *aLocation, nsISupports *aData, nsIException *aInner)
{
  NS_ENSURE_FALSE(mInitialized, NS_ERROR_ALREADY_INITIALIZED);

  if (aMessage) {
    mMessage =
      (char*) nsMemory::Clone(aMessage, sizeof(char)*(strlen(aMessage)+1));
  }

  if (aName) {
    mName = (char*) nsMemory::Clone(aName, sizeof(char)*(strlen(aName)+1));
  }

  mResult = aResult;

  if (aLocation) {
    mLocation = aLocation;
    
    
    nsresult rc;
    if (NS_FAILED(rc = aLocation->GetFilename(&mFilename))) {
      return rc;
    }
    if (NS_FAILED(rc = aLocation->GetLineNumber(&mLineNumber))) {
      return rc;
    }
  } else {
    nsresult rv;
    nsXPConnect* xpc = nsXPConnect::XPConnect();
    rv = xpc->GetCurrentJSStack(getter_AddRefs(mLocation));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  mData = aData;
  mInner = aInner;

  mInitialized = true;
  return NS_OK;
}

JSObject*
Exception::WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
{
  return ExceptionBinding::Wrap(cx, scope, this);
}

void
Exception::GetMessageMoz(nsString& retval)
{
  char* str = nullptr;
#ifdef DEBUG
  DebugOnly<nsresult> rv = 
#endif
  GetMessageMoz(&str);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  CopyUTF8toUTF16(str, retval);
  nsMemory::Free(str);
}

uint32_t
Exception::Result() const
{
  return (uint32_t)mResult;
}

void
Exception::GetName(nsString& retval)
{
  char* str = nullptr;
#ifdef DEBUG
  DebugOnly<nsresult> rv =
#endif
  GetName(&str);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  CopyUTF8toUTF16(str, retval);
  nsMemory::Free(str);
}

void
Exception::GetFilename(nsString& retval)
{
  char* str = nullptr;
#ifdef DEBUG
  DebugOnly<nsresult> rv =
#endif
  GetFilename(&str);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  CopyUTF8toUTF16(str, retval);
  nsMemory::Free(str);
}

uint32_t
Exception::LineNumber() const
{
  return mLineNumber;
}

uint32_t
Exception::ColumnNumber() const
{
  return 0;
}

already_AddRefed<nsIStackFrame>
Exception::GetLocation() const
{
  nsCOMPtr<nsIStackFrame> location = mLocation;
  return location.forget();
}

already_AddRefed<nsISupports>
Exception::GetInner() const
{
  nsCOMPtr<nsIException> inner = mInner;
  return inner.forget();
}

already_AddRefed<nsISupports>
Exception::GetData() const
{
  nsCOMPtr<nsISupports> data = mData;
  return data.forget();
}

void
Exception::Stringify(nsString& retval)
{
  char* str = nullptr;
#ifdef DEBUG
  DebugOnly<nsresult> rv =
#endif
  ToString(&str);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  CopyUTF8toUTF16(str, retval);
  nsMemory::Free(str);
}

NS_IMPL_ADDREF_INHERITED(DOMException, Exception)
NS_IMPL_RELEASE_INHERITED(DOMException, Exception)
NS_INTERFACE_MAP_BEGIN(DOMException)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMException)
NS_INTERFACE_MAP_END_INHERITING(Exception)

DOMException::DOMException(nsresult aRv, const char* aMessage,
                           const char* aName, uint16_t aCode)
  : Exception(nullptr, aRv, nullptr, nullptr, nullptr),
    mName(aName),
    mMessage(aMessage),
    mCode(aCode)
{
  SetIsDOMBinding();
}

NS_IMETHODIMP
DOMException::GetCode(uint16_t* aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  *aCode = mCode;

  
  
  if (NS_ERROR_GET_MODULE(mResult) != NS_ERROR_MODULE_DOM || !mCode) {
    nsCOMPtr<nsIDocument> doc = nsContentUtils::GetDocumentFromCaller();
    if (doc) {
      doc->WarnOnceAbout(nsIDocument::eDOMExceptionCode);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
DOMException::ToString(char **aReturn)
{
  *aReturn = nullptr;

  static const char defaultMsg[] = "<no message>";
  static const char defaultLocation[] = "<unknown>";
  static const char defaultName[] = "<unknown>";
  static const char format[] =
    "[Exception... \"%s\"  code: \"%d\" nsresult: \"0x%x (%s)\"  location: \"%s\"]";

  nsAutoCString location;

  if (mInner) {
    nsXPIDLCString filename;

    mInner->GetFilename(getter_Copies(filename));

    if (!filename.IsEmpty()) {
      uint32_t line_nr = 0;

      mInner->GetLineNumber(&line_nr);

      char *temp = PR_smprintf("%s Line: %d", filename.get(), line_nr);
      if (temp) {
        location.Assign(temp);
        PR_smprintf_free(temp);
      }
    }
  }

  if (location.IsEmpty()) {
    location = defaultLocation;
  }

  const char* msg = mMessage ? mMessage : defaultMsg;
  const char* resultName = mName ? mName : defaultName;

  *aReturn = PR_smprintf(format, msg, mCode, mResult, resultName,
                         location.get());

  return *aReturn ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
DOMException::GetName(nsString& retval)
{
  CopyUTF8toUTF16(mName, retval);
}

void
DOMException::GetMessageMoz(nsString& retval)
{
  CopyUTF8toUTF16(mMessage, retval);
}

JSObject*
DOMException::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DOMExceptionBinding::Wrap(aCx, aScope, this);
}

already_AddRefed<DOMException>
DOMException::Create(nsresult aRv)
{
  const char* name;
  const char* message;
  uint16_t code;
  NSResultToNameAndMessage(aRv, &name, &message, &code);
  nsRefPtr<DOMException> inst =
    new DOMException(aRv, message, name, code);
  return inst.forget();
}

} 
} 
