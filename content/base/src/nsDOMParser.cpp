




































#include "jsapi.h"
#include "nsDOMParser.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsIInputStream.h"
#include "nsNetUtil.h"
#include "nsStringStream.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIDOMClassInfo.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsNetCID.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsDOMError.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "mozilla/AutoRestore.h"

using namespace mozilla;

nsDOMParser::nsDOMParser()
  : mAttemptedInit(PR_FALSE)
{
}

nsDOMParser::~nsDOMParser()
{
}

DOMCI_DATA(DOMParser, nsDOMParser)


NS_INTERFACE_MAP_BEGIN(nsDOMParser)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsIDOMParserJS)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMParser)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMParser)
NS_IMPL_RELEASE(nsDOMParser)

NS_IMETHODIMP 
nsDOMParser::ParseFromString(const PRUnichar *str, 
                             const char *contentType,
                             nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG(str);
  NS_ENSURE_ARG_POINTER(aResult);

  NS_ConvertUTF16toUTF8 data(str);

  
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
                                      data.get(), data.Length(),
                                      NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv))
    return rv;

  return ParseFromStream(stream, "UTF-8", data.Length(), contentType, aResult);
}

NS_IMETHODIMP 
nsDOMParser::ParseFromBuffer(const PRUint8 *buf,
                             PRUint32 bufLen,
                             const char *contentType,
                             nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG_POINTER(buf);
  NS_ENSURE_ARG_POINTER(aResult);

  
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream),
                                      reinterpret_cast<const char *>(buf),
                                      bufLen, NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv))
    return rv;

  return ParseFromStream(stream, nsnull, bufLen, contentType, aResult);
}


NS_IMETHODIMP 
nsDOMParser::ParseFromStream(nsIInputStream *stream, 
                             const char *charset, 
                             PRInt32 contentLength,
                             const char *contentType,
                             nsIDOMDocument **aResult)
{
  NS_ENSURE_ARG(stream);
  NS_ENSURE_ARG(contentType);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  
  if ((nsCRT::strcmp(contentType, "text/xml") != 0) &&
      (nsCRT::strcmp(contentType, "application/xml") != 0) &&
      (nsCRT::strcmp(contentType, "application/xhtml+xml") != 0))
    return NS_ERROR_NOT_IMPLEMENTED;

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptHandlingObject);
  nsresult rv;
  if (!mPrincipal) {
    NS_ENSURE_TRUE(!mAttemptedInit, NS_ERROR_NOT_INITIALIZED);
    AttemptedInitMarker marker(&mAttemptedInit);
    
    nsCOMPtr<nsIPrincipal> prin =
      do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = Init(prin, nsnull, nsnull, scriptHandlingObject);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(mPrincipal, "Must have principal by now");
  NS_ASSERTION(mDocumentURI, "Must have document URI by now");
  
  
  nsCOMPtr<nsIInputStream> bufferedStream;
  if (!NS_InputStreamIsBuffered(stream)) {
    rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream), stream,
                                   4096);
    NS_ENSURE_SUCCESS(rv, rv);

    stream = bufferedStream;
  }

  
  
  
  
  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = nsContentUtils::CreateDocument(EmptyString(), EmptyString(), nsnull,
                                      mDocumentURI, mBaseURI,
                                      mOriginalPrincipal,
                                      scriptHandlingObject,
                                      getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> parserChannel;
  NS_NewInputStreamChannel(getter_AddRefs(parserChannel), mDocumentURI, nsnull,
                           nsDependentCString(contentType), nsnull);
  NS_ENSURE_STATE(parserChannel);

  
  parserChannel->SetOwner(mOriginalPrincipal);

  if (charset) {
    parserChannel->SetContentCharset(nsDependentCString(charset));
  }

  
  nsCOMPtr<nsIStreamListener> listener;

  
  
  
  
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  if (!document) return NS_ERROR_FAILURE;

  if (nsContentUtils::IsSystemPrincipal(mOriginalPrincipal)) {
    document->ForceEnableXULXBL();
  }

  rv = document->StartDocumentLoad(kLoadAsData, parserChannel, 
                                   nsnull, nsnull, 
                                   getter_AddRefs(listener),
                                   PR_FALSE);

  
  document->SetBaseURI(mBaseURI);

  
  document->SetPrincipal(mPrincipal);

  if (NS_FAILED(rv) || !listener) {
    return NS_ERROR_FAILURE;
  }

  
  nsresult status;

  rv = listener->OnStartRequest(parserChannel, nsnull);
  if (NS_FAILED(rv))
    parserChannel->Cancel(rv);
  parserChannel->GetStatus(&status);

  if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(status)) {
    rv = listener->OnDataAvailable(parserChannel, nsnull, stream, 0,
                                   contentLength);
    if (NS_FAILED(rv))
      parserChannel->Cancel(rv);
    parserChannel->GetStatus(&status);
  }

  rv = listener->OnStopRequest(parserChannel, nsnull, status);
  
  

  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  domDocument.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMParser::Init(nsIPrincipal* principal, nsIURI* documentURI,
                  nsIURI* baseURI, nsIScriptGlobalObject* aScriptObject)
{
  NS_ENSURE_STATE(!mAttemptedInit);
  mAttemptedInit = PR_TRUE;
  
  NS_ENSURE_ARG(principal || documentURI);

  mDocumentURI = documentURI;
  
  if (!mDocumentURI) {
    principal->GetURI(getter_AddRefs(mDocumentURI));
    
    
    if (!mDocumentURI && !nsContentUtils::IsSystemPrincipal(principal)) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  mScriptHandlingObject = do_GetWeakReference(aScriptObject);
  mPrincipal = principal;
  nsresult rv;
  if (!mPrincipal) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_NOT_AVAILABLE);
    rv =
      secMan->GetCodebasePrincipal(mDocumentURI, getter_AddRefs(mPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
    mOriginalPrincipal = mPrincipal;
  } else {
    mOriginalPrincipal = mPrincipal;
    if (nsContentUtils::IsSystemPrincipal(mPrincipal)) {
      
      
      mPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!mDocumentURI) {
        rv = mPrincipal->GetURI(getter_AddRefs(mDocumentURI));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  
  mBaseURI = baseURI;
  
  
  

  NS_POSTCONDITION(mPrincipal, "Must have principal");
  NS_POSTCONDITION(mOriginalPrincipal, "Must have original principal");
  NS_POSTCONDITION(mDocumentURI, "Must have document URI");
  return NS_OK;
}
  
static nsQueryInterface
JSvalToInterface(JSContext* cx, jsval val, nsIXPConnect* xpc, PRBool* wasNull)
{
  if (val == JSVAL_NULL) {
    *wasNull = PR_TRUE;
    return nsQueryInterface(nsnull);
  }
  
  *wasNull = PR_FALSE;
  if (JSVAL_IS_OBJECT(val)) {
    JSObject* arg = JSVAL_TO_OBJECT(val);

    nsCOMPtr<nsIXPConnectWrappedNative> native;
    xpc->GetWrappedNativeOfJSObject(cx, arg, getter_AddRefs(native));

    
    if (native) {
      return do_QueryWrappedNative(native);
    }
  }
  
  return nsQueryInterface(nsnull);
}

static nsresult
GetInitArgs(JSContext *cx, PRUint32 argc, jsval *argv,
            nsIPrincipal** aPrincipal, nsIURI** aDocumentURI,
            nsIURI** aBaseURI)
{
  
  PRBool haveUniversalXPConnect;
  nsresult rv = nsContentUtils::GetSecurityManager()->
    IsCapabilityEnabled("UniversalXPConnect", &haveUniversalXPConnect);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!haveUniversalXPConnect) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }    
  
  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  
  
  
  PRBool wasNull;
  nsCOMPtr<nsIPrincipal> prin = JSvalToInterface(cx, argv[0], xpc, &wasNull);
  if (!prin && !wasNull) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIURI> documentURI;
  nsCOMPtr<nsIURI> baseURI;
  if (argc > 1) {
    
    
    documentURI = JSvalToInterface(cx, argv[1], xpc, &wasNull);
    if (!documentURI && !wasNull) {
      return NS_ERROR_INVALID_ARG;
    }

    if (argc > 2) {
      
      baseURI = JSvalToInterface(cx, argv[2], xpc, &wasNull);
      if (!baseURI && !wasNull) {
        return NS_ERROR_INVALID_ARG;
      }
    }
  }

  NS_IF_ADDREF(*aPrincipal = prin);
  NS_IF_ADDREF(*aDocumentURI = documentURI);
  NS_IF_ADDREF(*aBaseURI = baseURI);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMParser::Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                        PRUint32 argc, jsval *argv)
{
  AttemptedInitMarker marker(&mAttemptedInit);
  nsCOMPtr<nsIPrincipal> prin;
  nsCOMPtr<nsIURI> documentURI;
  nsCOMPtr<nsIURI> baseURI;
  if (argc > 0) {
    nsresult rv = GetInitArgs(cx, argc, argv, getter_AddRefs(prin),
                              getter_AddRefs(documentURI),
                              getter_AddRefs(baseURI));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_UNEXPECTED);

    nsresult rv = secMan->GetSubjectPrincipal(getter_AddRefs(prin));
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ENSURE_TRUE(prin, NS_ERROR_UNEXPECTED);
  }

  NS_ASSERTION(prin, "Must have principal by now");
  
  if (!documentURI) {
    
    

    
    
    
    

    
    
    

    nsCOMPtr<nsIDocument> doc;
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aOwner);
    if (aOwner) {
      nsCOMPtr<nsIDOMDocument> domdoc = window->GetExtantDocument();
      doc = do_QueryInterface(domdoc);
    }

    if (!doc) {
      return NS_ERROR_UNEXPECTED;
    }

    baseURI = doc->GetDocBaseURI();
    documentURI = doc->GetDocumentURI();
  }

  nsCOMPtr<nsIScriptGlobalObject> scriptglobal = do_QueryInterface(aOwner);
  return Init(prin, documentURI, baseURI, scriptglobal);
}

NS_IMETHODIMP
nsDOMParser::Init(nsIPrincipal *aPrincipal, nsIURI *aDocumentURI,
                  nsIURI *aBaseURI)
{
  AttemptedInitMarker marker(&mAttemptedInit);

  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  NS_ENSURE_TRUE(cx, NS_ERROR_UNEXPECTED);

  nsIScriptContext* scriptContext = GetScriptContextFromJSContext(cx);

  nsCOMPtr<nsIPrincipal> principal = aPrincipal;

  if (!principal && !aDocumentURI) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_UNEXPECTED);

    nsresult rv = secMan->GetSubjectPrincipal(getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ENSURE_TRUE(principal, NS_ERROR_UNEXPECTED);
  }

  return Init(principal, aDocumentURI, aBaseURI,
              scriptContext ? scriptContext->GetGlobalObject() : nsnull);
}
