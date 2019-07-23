




































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
#include "nsLoadListenerProxy.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsNetCID.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsDOMError.h"


nsresult
nsDOMParser::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


nsresult
nsDOMParser::Load(nsIDOMEvent* aEvent)
{
  mLoopingForSyncLoad = PR_FALSE;

  return NS_OK;
}

nsresult
nsDOMParser::BeforeUnload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsDOMParser::Unload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsDOMParser::Abort(nsIDOMEvent* aEvent)
{
  mLoopingForSyncLoad = PR_FALSE;

  return NS_OK;
}

nsresult
nsDOMParser::Error(nsIDOMEvent* aEvent)
{
  mLoopingForSyncLoad = PR_FALSE;

  return NS_OK;
}

nsDOMParser::nsDOMParser()
  : mLoopingForSyncLoad(PR_FALSE),
    mAttemptedInit(PR_FALSE)
{
}

nsDOMParser::~nsDOMParser()
{
  NS_ABORT_IF_FALSE(!mLoopingForSyncLoad, "we rather crash than hang");
  mLoopingForSyncLoad = PR_FALSE;
}



NS_INTERFACE_MAP_BEGIN(nsDOMParser)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsIDOMParser)
  NS_INTERFACE_MAP_ENTRY(nsIDOMParserJS)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMParser)
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
                                      NS_REINTERPRET_CAST(const char *, buf),
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

  nsresult rv;
  if (!mPrincipal) {
    NS_ENSURE_TRUE(!mAttemptedInit, NS_ERROR_NOT_INITIALIZED);
    AttemptedInitMarker marker(&mAttemptedInit);
    
    nsCOMPtr<nsIPrincipal> prin =
      do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = Init(prin, nsnull, nsnull);
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
                                      mDocumentURI, mBaseURI, mPrincipal,
                                      getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsPIDOMEventTarget> target(do_QueryInterface(domDocument));
  if (target) {
    nsWeakPtr requestWeak(do_GetWeakReference(NS_STATIC_CAST(nsIDOMParser*, this)));
    nsLoadListenerProxy* proxy = new nsLoadListenerProxy(requestWeak);
    if (!proxy) return NS_ERROR_OUT_OF_MEMORY;

    
    rv = target->AddEventListenerByIID(NS_STATIC_CAST(nsIDOMEventListener*, 
                                                      proxy), 
                                       NS_GET_IID(nsIDOMLoadListener));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIChannel> parserChannel;
  NS_NewInputStreamChannel(getter_AddRefs(parserChannel), mDocumentURI, nsnull,
                           nsDependentCString(contentType), nsnull);
  NS_ENSURE_STATE(parserChannel);

  parserChannel->SetOwner(mPrincipal);

  if (charset) {
    parserChannel->SetContentCharset(nsDependentCString(charset));
  }

  
  nsCOMPtr<nsIStreamListener> listener;
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  if (!document) return NS_ERROR_FAILURE;

  mLoopingForSyncLoad = PR_TRUE;

  
  
  
  
  rv = document->StartDocumentLoad(kLoadAsData, parserChannel, 
                                   nsnull, nsnull, 
                                   getter_AddRefs(listener),
                                   PR_FALSE);

  
  document->SetPrincipal(mPrincipal);

  
  document->SetBaseURI(mBaseURI);

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

  
  

  nsIThread *thread = NS_GetCurrentThread();
  while (mLoopingForSyncLoad) {
    if (!NS_ProcessNextEvent(thread))
      break;
  }

  domDocument.swap(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMParser::Init(nsIPrincipal* principal, nsIURI* documentURI,
                  nsIURI* baseURI)
{
  NS_ENSURE_STATE(!mAttemptedInit);
  mAttemptedInit = PR_TRUE;
  
  NS_ENSURE_ARG(principal || documentURI);

  mDocumentURI = documentURI;
  if (!mDocumentURI) {
    principal->GetURI(getter_AddRefs(mDocumentURI));
    if (!mDocumentURI) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  mPrincipal = principal;
  if (!mPrincipal) {
    nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
    NS_ENSURE_TRUE(secMan, NS_ERROR_NOT_AVAILABLE);
    nsresult rv =
      secMan->GetCodebasePrincipal(mDocumentURI, getter_AddRefs(mPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  mBaseURI = baseURI;
  
  
  

  NS_POSTCONDITION(mPrincipal, "Must have principal");
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
nsDOMParser::Initialize(JSContext *cx, JSObject* obj,
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

    secMan->GetSubjectPrincipal(getter_AddRefs(prin));

    
    NS_ENSURE_TRUE(prin, NS_ERROR_UNEXPECTED);
  }

  NS_ASSERTION(prin, "Must have principal by now");
  
  if (!documentURI) {
    
    

    
    
    
    

    
    
    
    
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    if (!doc) {
      return NS_ERROR_UNEXPECTED;
    }

    baseURI = doc->GetBaseURI();
    documentURI = doc->GetDocumentURI();
  }

  return Init(prin, documentURI, baseURI);
}

NS_IMETHODIMP
nsDOMParser::Init()
{
  AttemptedInitMarker marker(&mAttemptedInit);

  nsCOMPtr<nsIXPCNativeCallContext> ncc;

  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(ncc, NS_ERROR_UNEXPECTED);

  JSContext *cx = nsnull;
  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cx, NS_ERROR_UNEXPECTED);

  PRUint32 argc;
  jsval *argv = nsnull;
  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  if (argc != 3) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIPrincipal> prin;
  nsCOMPtr<nsIURI> documentURI;
  nsCOMPtr<nsIURI> baseURI;
  rv = GetInitArgs(cx, argc, argv, getter_AddRefs(prin),
                   getter_AddRefs(documentURI), getter_AddRefs(baseURI));
  NS_ENSURE_SUCCESS(rv, rv);

  return Init(prin, documentURI, baseURI);  
}
