





































#include "nsCOMPtr.h"
#include "nsCRTGlue.h"
#include "nsDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsDOMException.h"
#include "nsIDOMDOMException.h"
#include "nsIDOMRangeException.h"
#ifdef MOZ_SVG
#include "nsIDOMSVGException.h"
#endif
#include "nsIDOMXPathException.h"
#include "nsString.h"
#include "prprf.h"

#define DOM_MSG_DEF(val, message) {(val), #val, message},

#define IMPL_INTERNAL_DOM_EXCEPTION_HEAD(classname, ifname)                  \
class classname : public nsBaseDOMException,                                 \
                  public ifname                                              \
{                                                                            \
public:                                                                      \
  classname();                                                               \
  virtual ~classname();                                                      \
                                                                             \
  NS_DECL_ISUPPORTS_INHERITED

#define IMPL_INTERNAL_DOM_EXCEPTION_TAIL(classname, ifname, domname, module, \
                                         mapping_function)                   \
};                                                                           \
                                                                             \
classname::classname() {}                                                    \
classname::~classname() {}                                                   \
                                                                             \
NS_IMPL_ADDREF_INHERITED(classname, nsBaseDOMException)                      \
NS_IMPL_RELEASE_INHERITED(classname, nsBaseDOMException)                     \
NS_CLASSINFO_MAP_BEGIN(domname)                                              \
  NS_CLASSINFO_MAP_ENTRY(nsIException)                                       \
  NS_CLASSINFO_MAP_ENTRY(ifname)                                             \
NS_CLASSINFO_MAP_END                                                         \
NS_INTERFACE_MAP_BEGIN(classname)                                            \
  NS_INTERFACE_MAP_ENTRY(ifname)                                             \
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(domname)                              \
NS_INTERFACE_MAP_END_INHERITING(nsBaseDOMException)                          \
                                                                             \
nsresult                                                                     \
NS_New##domname(nsresult aNSResult, nsIException* aDefaultException,         \
                nsIException** aException)                                   \
{                                                                            \
  if (!(NS_ERROR_GET_MODULE(aNSResult) == module)) {                         \
    NS_WARNING("Trying to create an exception for the wrong error module."); \
    return NS_ERROR_FAILURE;                                                 \
  }                                                                          \
  const char* name;                                                          \
  const char* message;                                                       \
  mapping_function(aNSResult, &name, &message);                              \
  classname* inst = new classname();                                         \
  NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);                              \
  inst->Init(aNSResult, name, message, aDefaultException);                   \
  *aException = inst;                                                        \
  NS_ADDREF(*aException);                                                    \
  return NS_OK;                                                              \
}

static struct ResultStruct
{
  nsresult mNSResult;
  const char* mName;
  const char* mMessage;
} gDOMErrorMsgMap[] = {
#include "domerr.msg"
  {0, nsnull, nsnull}   
};

#undef DOM_MSG_DEF

static void
NSResultToNameAndMessage(nsresult aNSResult,
                         const char** aName,
                         const char** aMessage)
{
  ResultStruct* result_struct = gDOMErrorMsgMap;

  while (result_struct->mName) {
    if (aNSResult == result_struct->mNSResult) {
      *aName = result_struct->mName;
      *aMessage = result_struct->mMessage;
      return;
    }

    ++result_struct;
  }

  NS_WARNING("Huh, someone is throwing non-DOM errors using the DOM module!");

  return;
}

IMPL_INTERNAL_DOM_EXCEPTION_HEAD(nsDOMException, nsIDOMDOMException)
  NS_DECL_NSIDOMDOMEXCEPTION
IMPL_INTERNAL_DOM_EXCEPTION_TAIL(nsDOMException, nsIDOMDOMException,
                                 DOMException, NS_ERROR_MODULE_DOM,
                                 NSResultToNameAndMessage)

NS_IMETHODIMP
nsDOMException::GetCode(PRUint32* aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  nsresult result;
  GetResult(&result);
  *aCode = NS_ERROR_GET_CODE(result);

  return NS_OK;
}

IMPL_INTERNAL_DOM_EXCEPTION_HEAD(nsRangeException, nsIDOMRangeException)
  NS_DECL_NSIDOMRANGEEXCEPTION
IMPL_INTERNAL_DOM_EXCEPTION_TAIL(nsRangeException, nsIDOMRangeException,
                                 RangeException, NS_ERROR_MODULE_DOM_RANGE,
                                 NSResultToNameAndMessage)

NS_IMETHODIMP
nsRangeException::GetCode(PRUint16* aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  nsresult result;
  GetResult(&result);
  *aCode = NS_ERROR_GET_CODE(result);

  return NS_OK;
}

#ifdef MOZ_SVG
IMPL_INTERNAL_DOM_EXCEPTION_HEAD(nsSVGException, nsIDOMSVGException)
  NS_DECL_NSIDOMSVGEXCEPTION
IMPL_INTERNAL_DOM_EXCEPTION_TAIL(nsSVGException, nsIDOMSVGException,
                                 SVGException, NS_ERROR_MODULE_SVG,
                                 NSResultToNameAndMessage)

NS_IMETHODIMP
nsSVGException::GetCode(PRUint16* aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  nsresult result;
  GetResult(&result);
  *aCode = NS_ERROR_GET_CODE(result);

  return NS_OK;
}
#endif 

IMPL_INTERNAL_DOM_EXCEPTION_HEAD(nsXPathException, nsIDOMXPathException)
  NS_DECL_NSIDOMXPATHEXCEPTION
IMPL_INTERNAL_DOM_EXCEPTION_TAIL(nsXPathException, nsIDOMXPathException,
                                 XPathException, NS_ERROR_MODULE_DOM_XPATH,
                                 NSResultToNameAndMessage)

NS_IMETHODIMP
nsXPathException::GetCode(PRUint16* aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  nsresult result;
  GetResult(&result);
  *aCode = NS_ERROR_GET_CODE(result);

  return NS_OK;
}

nsBaseDOMException::nsBaseDOMException()
{
}

nsBaseDOMException::~nsBaseDOMException()
{
}

NS_IMPL_ISUPPORTS2(nsBaseDOMException, nsIException, nsIBaseDOMException)

NS_IMETHODIMP
nsBaseDOMException::GetMessage(char **aMessage)
{
  if (mMessage) {
    *aMessage = NS_strdup(mMessage);
  } else {
    *aMessage = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetResult(PRUint32* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = mResult;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetName(char **aName)
{
  NS_ENSURE_ARG_POINTER(aName);

  if (mName) {
    *aName = NS_strdup(mName);
  } else {
    *aName = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetFilename(char **aFilename)
{
  if (mInner) {
    return mInner->GetFilename(aFilename);
  }

  NS_ENSURE_ARG_POINTER(aFilename);

  *aFilename = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetLineNumber(PRUint32 *aLineNumber)
{
  if (mInner) {
    return mInner->GetLineNumber(aLineNumber);
  }

  NS_ENSURE_ARG_POINTER(aLineNumber);

  *aLineNumber = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetColumnNumber(PRUint32 *aColumnNumber)
{
  if (mInner) {
    return mInner->GetColumnNumber(aColumnNumber);
  }

  NS_ENSURE_ARG_POINTER(aColumnNumber);

  *aColumnNumber = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetLocation(nsIStackFrame **aLocation)
{
  if (mInner) {
    return mInner->GetLocation(aLocation);
  }

  NS_ENSURE_ARG_POINTER(aLocation);

  *aLocation = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetInner(nsIException **aInner)
{
  NS_ENSURE_ARG_POINTER(aInner);

  *aInner = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::GetData(nsISupports **aData)
{
  if (mInner) {
    return mInner->GetData(aData);
  }

  NS_ENSURE_ARG_POINTER(aData);

  *aData = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsBaseDOMException::ToString(char **aReturn)
{
  *aReturn = nsnull;

  static const char defaultMsg[] = "<no message>";
  static const char defaultLocation[] = "<unknown>";
  static const char defaultName[] = "<unknown>";
  static const char format[] =
    "[Exception... \"%s\"  code: \"%d\" nsresult: \"0x%x (%s)\"  location: \"%s\"]";

  nsCAutoString location;

  if (mInner) {
    nsXPIDLCString filename;

    mInner->GetFilename(getter_Copies(filename));

    if (!filename.IsEmpty()) {
      PRUint32 line_nr = 0;

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
  PRUint32 code = NS_ERROR_GET_CODE(mResult);

  *aReturn = PR_smprintf(format, msg, code, mResult, resultName,
                         location.get());

  return *aReturn ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsBaseDOMException::Init(nsresult aNSResult, const char* aName,
                         const char* aMessage,
                         nsIException* aDefaultException)
{
  mResult = aNSResult;
  mName = aName;
  mMessage = aMessage;
  mInner = aDefaultException;
  return NS_OK;
}
