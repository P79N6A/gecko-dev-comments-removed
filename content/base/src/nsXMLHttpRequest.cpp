




































#include "nsXMLHttpRequest.h"
#include "nsISimpleEnumerator.h"
#include "nsIXPConnect.h"
#include "nsICharsetConverterManager.h"
#include "nsLayoutCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIDOMSerializer.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "prprf.h"
#include "nsIDOMEventListener.h"
#include "nsIJSContextStack.h"
#include "nsJSEnvironment.h"
#include "nsIScriptSecurityManager.h"
#include "nsWeakPtr.h"
#include "nsICharsetAlias.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMElement.h"
#include "nsIDOMFileInternal.h"
#include "nsIDOMWindow.h"
#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsIVariant.h"
#include "nsVariant.h"
#include "nsIParser.h"
#include "nsLoadListenerProxy.h"
#include "nsStringStream.h"
#include "nsIStreamConverterService.h"
#include "nsICachingChannel.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsDOMJSUtils.h"
#include "nsCOMArray.h"
#include "nsDOMClassInfo.h"
#include "nsIScriptableUConv.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsLayoutStatics.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsDOMError.h"
#include "nsIHTMLDocument.h"
#include "nsIDOM3Document.h"
#include "nsIMultiPartChannel.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIStorageStream.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsCommaSeparatedTokenizer.h"

#define LOAD_STR "load"
#define ERROR_STR "error"
#define ABORT_STR "abort"
#define LOADSTART_STR "loadstart"
#define PROGRESS_STR "progress"
#define UPLOADPROGRESS_STR "uploadprogress"
#define READYSTATE_STR "readystatechange"




#define XML_HTTP_REQUEST_UNINITIALIZED  (1 << 0)  // 0
#define XML_HTTP_REQUEST_OPENED         (1 << 1)  // 1 aka LOADING
#define XML_HTTP_REQUEST_LOADED         (1 << 2)  // 2
#define XML_HTTP_REQUEST_INTERACTIVE    (1 << 3)  // 3
#define XML_HTTP_REQUEST_COMPLETED      (1 << 4)  // 4
#define XML_HTTP_REQUEST_SENT           (1 << 5)  // Internal, LOADING in IE and external view
#define XML_HTTP_REQUEST_STOPPED        (1 << 6)  // Internal, INTERACTIVE in IE and external view


#define XML_HTTP_REQUEST_ABORTED        (1 << 7)  // Internal
#define XML_HTTP_REQUEST_ASYNC          (1 << 8)  // Internal
#define XML_HTTP_REQUEST_PARSEBODY      (1 << 9)  // Internal
#define XML_HTTP_REQUEST_XSITEENABLED   (1 << 10) // Internal, Is any cross-site request allowed?
                                                  
                                                  
#define XML_HTTP_REQUEST_SYNCLOOPING    (1 << 11) // Internal
#define XML_HTTP_REQUEST_MULTIPART      (1 << 12) // Internal
#define XML_HTTP_REQUEST_GOT_FINAL_STOP (1 << 13) // Internal
#define XML_HTTP_REQUEST_BACKGROUND     (1 << 14) // Internal


#define XML_HTTP_REQUEST_MPART_HEADERS  (1 << 15) // Internal
#define XML_HTTP_REQUEST_USE_XSITE_AC   (1 << 16) // Internal
#define XML_HTTP_REQUEST_NEED_AC_PREFLIGHT (1 << 17) // Internal
#define XML_HTTP_REQUEST_AC_WITH_CREDENTIALS (1 << 18) // Internal

#define XML_HTTP_REQUEST_LOADSTATES         \
  (XML_HTTP_REQUEST_UNINITIALIZED |         \
   XML_HTTP_REQUEST_OPENED |                \
   XML_HTTP_REQUEST_LOADED |                \
   XML_HTTP_REQUEST_INTERACTIVE |           \
   XML_HTTP_REQUEST_COMPLETED |             \
   XML_HTTP_REQUEST_SENT |                  \
   XML_HTTP_REQUEST_STOPPED)

#define ACCESS_CONTROL_CACHE_SIZE 100

#define NS_BADCERTHANDLER_CONTRACTID \
  "@mozilla.org/content/xmlhttprequest-bad-cert-handler;1"

#define NS_PROGRESS_EVENT_INTERVAL 50

class nsResumeTimeoutsEvent : public nsRunnable
{
public:
  nsResumeTimeoutsEvent(nsPIDOMWindow* aWindow) : mWindow(aWindow) {}

  NS_IMETHOD Run()
  {
    mWindow->ResumeTimeouts(PR_FALSE);
    return NS_OK;
  }

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
};




static void AddLoadFlags(nsIRequest *request, nsLoadFlags newFlags)
{
  nsLoadFlags flags;
  request->GetLoadFlags(&flags);
  flags |= newFlags;
  request->SetLoadFlags(flags);
}

static nsresult IsCapabilityEnabled(const char *capability, PRBool *enabled)
{
  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  if (!secMan)
    return NS_ERROR_FAILURE;

  return secMan->IsCapabilityEnabled(capability, enabled);
}




class nsMultipartProxyListener : public nsIStreamListener
{
public:
  nsMultipartProxyListener(nsIStreamListener *dest);
  virtual ~nsMultipartProxyListener();

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;
};


nsMultipartProxyListener::nsMultipartProxyListener(nsIStreamListener *dest)
  : mDestListener(dest)
{
}

nsMultipartProxyListener::~nsMultipartProxyListener()
{
}

NS_IMPL_ISUPPORTS2(nsMultipartProxyListener, nsIStreamListener,
                   nsIRequestObserver)



NS_IMETHODIMP
nsMultipartProxyListener::OnStartRequest(nsIRequest *aRequest,
                                         nsISupports *ctxt)
{
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCAutoString contentType;
  nsresult rv = channel->GetContentType(contentType);

  if (!contentType.EqualsLiteral("multipart/x-mixed-replace")) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  

  nsCOMPtr<nsIXMLHttpRequest> xhr = do_QueryInterface(mDestListener);

  nsCOMPtr<nsIStreamConverterService> convServ =
    do_GetService("@mozilla.org/streamConverters;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIStreamListener> toListener(mDestListener);
    nsCOMPtr<nsIStreamListener> fromListener;

    rv = convServ->AsyncConvertData("multipart/x-mixed-replace",
                                    "*/*",
                                    toListener,
                                    nsnull,
                                    getter_AddRefs(fromListener));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && fromListener, NS_ERROR_UNEXPECTED);

    mDestListener = fromListener;
  }

  if (xhr) {
    static_cast<nsXMLHttpRequest*>(xhr.get())->mState |=
      XML_HTTP_REQUEST_MPART_HEADERS;
   }

  return mDestListener->OnStartRequest(aRequest, ctxt);
}

NS_IMETHODIMP
nsMultipartProxyListener::OnStopRequest(nsIRequest *aRequest,
                                        nsISupports *ctxt,
                                        nsresult status)
{
  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}



NS_IMETHODIMP
nsMultipartProxyListener::OnDataAvailable(nsIRequest *aRequest,
                                          nsISupports *ctxt,
                                          nsIInputStream *inStr,
                                          PRUint32 sourceOffset,
                                          PRUint32 count)
{
  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset,
                                        count);
}



class nsACProxyListener : public nsIStreamListener,
                          public nsIInterfaceRequestor,
                          public nsIChannelEventSink
{
public:
  nsACProxyListener(nsIChannel* aOuterChannel,
                    nsIStreamListener* aOuterListener,
                    nsISupports* aOuterContext,
                    nsIPrincipal* aReferrerPrincipal,
                    const nsACString& aRequestMethod,
                    PRBool aWithCredentials)
   : mOuterChannel(aOuterChannel), mOuterListener(aOuterListener),
     mOuterContext(aOuterContext), mReferrerPrincipal(aReferrerPrincipal),
     mRequestMethod(aRequestMethod), mWithCredentials(aWithCredentials)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

private:
  void AddResultToCache(nsIRequest* aRequest);

  nsCOMPtr<nsIChannel> mOuterChannel;
  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsISupports> mOuterContext;
  nsCOMPtr<nsIPrincipal> mReferrerPrincipal;
  nsCString mRequestMethod;
  PRBool mWithCredentials;
};

NS_IMPL_ISUPPORTS4(nsACProxyListener, nsIStreamListener, nsIRequestObserver,
                   nsIInterfaceRequestor, nsIChannelEventSink)

void
nsACProxyListener::AddResultToCache(nsIRequest *aRequest)
{
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  NS_ASSERTION(http, "Request was not http");

  
  nsCAutoString headerVal;
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Max-Age"),
                          headerVal);
  if (headerVal.IsEmpty()) {
    return;
  }

  
  
  
  PRUint32 age = 0;
  nsCSubstring::const_char_iterator iter, end;
  headerVal.BeginReading(iter);
  headerVal.EndReading(end);
  while (iter != end) {
    if (*iter < '0' || *iter > '9') {
      return;
    }
    age = age * 10 + (*iter - '0');
    
    age = PR_MIN(age, 86400);
    ++iter;
  }

  if (!age || !nsXMLHttpRequest::EnsureACCache()) {
    return;
  }


  
  
  

  nsCOMPtr<nsIURI> uri;
  http->GetURI(getter_AddRefs(uri));

  
  PRTime expirationTime = PR_Now() + (PRUint64)age * PR_USEC_PER_SEC;

  nsAccessControlLRUCache::CacheEntry* entry =
    nsXMLHttpRequest::sAccessControlCache->
    GetEntry(uri, mReferrerPrincipal, mWithCredentials, PR_TRUE);
  if (!entry) {
    return;
  }

  
  
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Methods"),
                          headerVal);

  nsCCommaSeparatedTokenizer methods(headerVal);
  while(methods.hasMoreTokens()) {
    const nsDependentCSubstring& method = methods.nextToken();
    if (method.IsEmpty()) {
      continue;
    }
    PRUint32 i;
    for (i = 0; i < entry->mMethods.Length(); ++i) {
      if (entry->mMethods[i].token.Equals(method)) {
        entry->mMethods[i].expirationTime = expirationTime;
        break;
      }
    }
    if (i == entry->mMethods.Length()) {
      nsAccessControlLRUCache::TokenTime* newMethod =
        entry->mMethods.AppendElement();
      if (!newMethod) {
        return;
      }

      newMethod->token = method;
      newMethod->expirationTime = expirationTime;
    }
  }

  
  
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Headers"),
                          headerVal);

  nsCCommaSeparatedTokenizer headers(headerVal);
  while(headers.hasMoreTokens()) {
    const nsDependentCSubstring& header = headers.nextToken();
    if (header.IsEmpty()) {
      continue;
    }
    PRUint32 i;
    for (i = 0; i < entry->mHeaders.Length(); ++i) {
      if (entry->mHeaders[i].token.Equals(header)) {
        entry->mHeaders[i].expirationTime = expirationTime;
        break;
      }
    }
    if (i == entry->mHeaders.Length()) {
      nsAccessControlLRUCache::TokenTime* newHeader =
        entry->mHeaders.AppendElement();
      if (!newHeader) {
        return;
      }

      newHeader->token = header;
      newHeader->expirationTime = expirationTime;
    }
  }
}

NS_IMETHODIMP
nsACProxyListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);

  if (NS_SUCCEEDED(rv)) {
    rv = status;
  }

  if (NS_SUCCEEDED(rv)) {
    
    AddResultToCache(aRequest);

    rv = mOuterChannel->AsyncOpen(mOuterListener, mOuterContext);
  }

  if (NS_FAILED(rv)) {
    mOuterChannel->Cancel(rv);
    mOuterListener->OnStartRequest(mOuterChannel, mOuterContext);
    mOuterListener->OnStopRequest(mOuterChannel, mOuterContext, rv);
    
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsACProxyListener::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext,
                                 nsresult aStatus)
{
  return NS_OK;
}



NS_IMETHODIMP
nsACProxyListener::OnDataAvailable(nsIRequest *aRequest,
                                   nsISupports *ctxt,
                                   nsIInputStream *inStr,
                                   PRUint32 sourceOffset,
                                   PRUint32 count)
{
  return NS_OK;
}

NS_IMETHODIMP
nsACProxyListener::OnChannelRedirect(nsIChannel *aOldChannel,
                                     nsIChannel *aNewChannel,
                                     PRUint32 aFlags)
{
  
  return NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags) ?
         NS_OK : NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsACProxyListener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}



nsXHREventTarget::~nsXHREventTarget()
{
  nsISupports *supports = static_cast<nsIXMLHttpRequestEventTarget*>(this);
  nsContentUtils::ReleaseWrapper(supports, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXHREventTarget)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXHREventTarget,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadStartListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnProgressListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_ROOT_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXHREventTarget,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadStartListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnProgressListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXHREventTarget)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestEventTarget)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsXHREventTarget, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsXHREventTarget, nsDOMEventTargetHelper)

NS_IMETHODIMP
nsXHREventTarget::GetOnload(nsIDOMEventListener** aOnLoad)
{
  return GetInnerEventListener(mOnLoadListener, aOnLoad);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnload(nsIDOMEventListener* aOnLoad)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(LOAD_STR),
                                mOnLoadListener, aOnLoad);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnerror(nsIDOMEventListener** aOnerror)
{
  return GetInnerEventListener(mOnErrorListener, aOnerror);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnerror(nsIDOMEventListener* aOnerror)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_STR),
                                mOnErrorListener, aOnerror);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnabort(nsIDOMEventListener** aOnabort)
{
  return GetInnerEventListener(mOnAbortListener, aOnabort);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnabort(nsIDOMEventListener* aOnabort)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ABORT_STR),
                                mOnAbortListener, aOnabort);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnloadstart(nsIDOMEventListener** aOnloadstart)
{
  return GetInnerEventListener(mOnLoadStartListener, aOnloadstart);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnloadstart(nsIDOMEventListener* aOnloadstart)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(LOADSTART_STR),
                                mOnLoadStartListener, aOnloadstart);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnprogress(nsIDOMEventListener** aOnprogress)
{
  return GetInnerEventListener(mOnProgressListener, aOnprogress);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnprogress(nsIDOMEventListener* aOnprogress)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(PROGRESS_STR),
                                mOnProgressListener, aOnprogress);
}



nsXMLHttpRequestUpload::~nsXMLHttpRequestUpload()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

NS_INTERFACE_MAP_BEGIN(nsXMLHttpRequestUpload)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestUpload)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLHttpRequestUpload)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)

void
nsAccessControlLRUCache::CacheEntry::PurgeExpired(PRTime now)
{
  PRUint32 i;
  for (i = 0; i < mMethods.Length(); ++i) {
    if (now >= mMethods[i].expirationTime) {
      mMethods.RemoveElementAt(i--);
    }
  }
  for (i = 0; i < mHeaders.Length(); ++i) {
    if (now >= mHeaders[i].expirationTime) {
      mHeaders.RemoveElementAt(i--);
    }
  }
}

PRBool
nsAccessControlLRUCache::CacheEntry::CheckRequest(const nsCString& aMethod,
                                                  const nsTArray<nsCString>& aHeaders)
{
  PurgeExpired(PR_Now());

  if (!aMethod.EqualsLiteral("GET") && !aMethod.EqualsLiteral("POST")) {
    PRUint32 i;
    for (i = 0; i < mMethods.Length(); ++i) {
      if (aMethod.Equals(mMethods[i].token))
        break;
    }
    if (i == mMethods.Length()) {
      return PR_FALSE;
    }
  }

  for (PRUint32 i = 0; i < aHeaders.Length(); ++i) {
    PRUint32 j;
    for (j = 0; j < mHeaders.Length(); ++j) {
      if (aHeaders[i].Equals(mHeaders[j].token,
                             nsCaseInsensitiveCStringComparator())) {
        break;
      }
    }
    if (j == mHeaders.Length()) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

nsAccessControlLRUCache::CacheEntry*
nsAccessControlLRUCache::GetEntry(nsIURI* aURI,
                                  nsIPrincipal* aPrincipal,
                                  PRBool aWithCredentials,
                                  PRBool aCreate)
{
  nsCString key;
  if (!GetCacheKey(aURI, aPrincipal, aWithCredentials, key)) {
    NS_WARNING("Invalid cache key!");
    return nsnull;
  }

  CacheEntry* entry;

  if (mTable.Get(key, &entry)) {
    

    
    PR_REMOVE_LINK(entry);
    PR_INSERT_LINK(entry, &mList);

    return entry;
  }

  if (!aCreate) {
    return nsnull;
  }

  
  
  entry = new CacheEntry(key);
  if (!entry) {
    NS_WARNING("Failed to allocate new cache entry!");
    return nsnull;
  }

  if (!mTable.Put(key, entry)) {
    
    delete entry;

    NS_WARNING("Failed to add entry to the access control cache!");
    return nsnull;
  }

  PR_INSERT_LINK(entry, &mList);

  NS_ASSERTION(mTable.Count() <= ACCESS_CONTROL_CACHE_SIZE + 1,
               "Something is borked, too many entries in the cache!");

  
  if (mTable.Count() > ACCESS_CONTROL_CACHE_SIZE) {
    
    PRTime now = PR_Now();
    mTable.Enumerate(RemoveExpiredEntries, &now);

    
    
    if (mTable.Count() > ACCESS_CONTROL_CACHE_SIZE) {
      CacheEntry* lruEntry = static_cast<CacheEntry*>(PR_LIST_TAIL(&mList));
      PR_REMOVE_LINK(lruEntry);

      
      mTable.Remove(lruEntry->mKey);

      NS_ASSERTION(mTable.Count() == ACCESS_CONTROL_CACHE_SIZE,
                   "Somehow tried to remove an entry that was never added!");
    }
  }
  
  return entry;
}

void
nsAccessControlLRUCache::RemoveEntries(nsIURI* aURI, nsIPrincipal* aPrincipal)
{
  CacheEntry* entry;
  nsCString key;
  if (GetCacheKey(aURI, aPrincipal, PR_TRUE, key) &&
      mTable.Get(key, &entry)) {
    PR_REMOVE_LINK(entry);
    mTable.Remove(key);
  }

  if (GetCacheKey(aURI, aPrincipal, PR_FALSE, key) &&
      mTable.Get(key, &entry)) {
    PR_REMOVE_LINK(entry);
    mTable.Remove(key);
  }
}

void
nsAccessControlLRUCache::Clear()
{
  PR_INIT_CLIST(&mList);
  mTable.Clear();
}

 PLDHashOperator
nsAccessControlLRUCache::RemoveExpiredEntries(const nsACString& aKey,
                                              nsAutoPtr<CacheEntry>& aValue,
                                              void* aUserData)
{
  PRTime* now = static_cast<PRTime*>(aUserData);
  
  aValue->PurgeExpired(*now);
  
  if (aValue->mHeaders.IsEmpty() &&
      aValue->mHeaders.IsEmpty()) {
    
    PR_REMOVE_LINK(aValue);
    return PL_DHASH_REMOVE;
  }
  
  return PL_DHASH_NEXT;
}

 PRBool
nsAccessControlLRUCache::GetCacheKey(nsIURI* aURI,
                                     nsIPrincipal* aPrincipal,
                                     PRBool aWithCredentials,
                                     nsACString& _retval)
{
  NS_ASSERTION(aURI, "Null uri!");
  NS_ASSERTION(aPrincipal, "Null principal!");
  
  NS_NAMED_LITERAL_CSTRING(space, " ");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  nsCAutoString scheme, host, port;
  if (uri) {
    uri->GetScheme(scheme);
    uri->GetHost(host);
    port.AppendInt(NS_GetRealPort(uri));
  }

  nsCAutoString cred;
  if (aWithCredentials) {
    _retval.AssignLiteral("cred");
  }
  else {
    _retval.AssignLiteral("nocred");
  }

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  _retval.Assign(cred + space + scheme + space + host + space + port + space +
                 spec);

  return PR_TRUE;
}







nsAccessControlLRUCache* nsXMLHttpRequest::sAccessControlCache = nsnull;

nsXMLHttpRequest::nsXMLHttpRequest()
  : mRequestObserver(nsnull), mState(XML_HTTP_REQUEST_UNINITIALIZED),
    mUploadTransferred(0), mUploadTotal(0), mUploadComplete(PR_TRUE),
    mUploadProgress(0), mUploadProgressMax(0),
    mErrorLoad(PR_FALSE), mTimerIsActive(PR_FALSE),
    mProgressEventWasDelayed(PR_FALSE),
    mLoadLengthComputable(PR_FALSE), mLoadTotal(0),
    mFirstStartRequestSeen(PR_FALSE)
{
  nsLayoutStatics::AddRef();
}

nsXMLHttpRequest::~nsXMLHttpRequest()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }

  if (mState & (XML_HTTP_REQUEST_STOPPED |
                XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_INTERACTIVE)) {
    Abort();
  }

  NS_ABORT_IF_FALSE(!(mState & XML_HTTP_REQUEST_SYNCLOOPING), "we rather crash than hang");
  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  nsLayoutStatics::Release();
}




nsresult
nsXMLHttpRequest::Init()
{
  
  
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");

  if (!stack) {
    return NS_OK;
  }

  JSContext *cx;

  if (NS_FAILED(stack->Peek(&cx)) || !cx) {
    return NS_OK;
  }

  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (secMan) {
    secMan->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  }
  NS_ENSURE_STATE(subjectPrincipal);
  mPrincipal = subjectPrincipal;

  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (context) {
    mScriptContext = context;
    nsCOMPtr<nsPIDOMWindow> window =
      do_QueryInterface(context->GetGlobalObject());
    if (window) {
      mOwner = window->GetCurrentInnerWindow();
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::Init(nsIPrincipal* aPrincipal,
                       nsIScriptContext* aScriptContext,
                       nsPIDOMWindow* aOwnerWindow,
                       nsIURI* aBaseURI)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  
  
  

  mPrincipal = aPrincipal;
  mScriptContext = aScriptContext;
  if (aOwnerWindow) {
    mOwner = aOwnerWindow->GetCurrentInnerWindow();
  }
  else {
    mOwner = nsnull;
  }
  mBaseURI = aBaseURI;

  return NS_OK;
}




NS_IMETHODIMP
nsXMLHttpRequest::Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                             PRUint32 argc, jsval *argv)
{
  mOwner = do_QueryInterface(aOwner);
  if (!mOwner) {
    NS_WARNING("Unexpected nsIJSNativeInitializer owner");
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(scriptPrincipal);
  mPrincipal = scriptPrincipal->GetPrincipal();
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(sgo);
  mScriptContext = sgo->GetContext();
  NS_ENSURE_STATE(mScriptContext);
  return NS_OK; 
}

void
nsXMLHttpRequest::SetRequestObserver(nsIRequestObserver* aObserver)
{
  mRequestObserver = aObserver;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLHttpRequest,
                                                  nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mReadRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mACGetChannel)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mUpload,
                                                       nsIXMLHttpRequestUpload)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXMLHttpRequest,
                                                nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mReadRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mACGetChannel)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mUpload)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIJSXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLHttpRequest)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequest, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequest, nsXHREventTarget)

NS_IMETHODIMP
nsXMLHttpRequest::GetOnreadystatechange(nsIDOMEventListener * *aOnreadystatechange)
{
  return
    nsXHREventTarget::GetInnerEventListener(mOnReadystatechangeListener,
                                            aOnreadystatechange);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnreadystatechange(nsIDOMEventListener * aOnreadystatechange)
{
  return
    nsXHREventTarget::RemoveAddEventListener(NS_LITERAL_STRING(READYSTATE_STR),
                                             mOnReadystatechangeListener,
                                             aOnreadystatechange);
}

NS_IMETHODIMP
nsXMLHttpRequest::GetOnuploadprogress(nsIDOMEventListener * *aOnuploadprogress)
{
  return
    nsXHREventTarget::GetInnerEventListener(mOnUploadProgressListener,
                                            aOnuploadprogress);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnuploadprogress(nsIDOMEventListener * aOnuploadprogress)
{
  return
    nsXHREventTarget::RemoveAddEventListener(NS_LITERAL_STRING(UPLOADPROGRESS_STR),
                                             mOnUploadProgressListener,
                                             aOnuploadprogress);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetChannel(nsIChannel **aChannel)
{
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_IF_ADDREF(*aChannel = mChannel);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseXML(nsIDOMDocument **aResponseXML)
{
  NS_ENSURE_ARG_POINTER(aResponseXML);
  *aResponseXML = nsnull;
  if ((XML_HTTP_REQUEST_COMPLETED & mState) && mResponseXML) {
    *aResponseXML = mResponseXML;
    NS_ADDREF(*aResponseXML);
  }

  return NS_OK;
}





nsresult
nsXMLHttpRequest::DetectCharset(nsACString& aCharset)
{
  aCharset.Truncate();
  nsresult rv;
  nsCAutoString charsetVal;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(mReadRequest));
  if (!channel) {
    channel = mChannel;
    if (!channel) {
      
      
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  rv = channel->GetContentCharset(charsetVal);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(NS_CHARSETALIAS_CONTRACTID,&rv));
    if(NS_SUCCEEDED(rv) && calias) {
      rv = calias->GetPreferred(charsetVal, aCharset);
    }
  }
  return rv;
}

nsresult
nsXMLHttpRequest::ConvertBodyToText(nsAString& aOutBuffer)
{
  
  
  
  

  PRInt32 dataLen = mResponseBody.Length();
  if (!dataLen)
    return NS_OK;

  nsresult rv = NS_OK;

  nsCAutoString dataCharset;
  nsCOMPtr<nsIDocument> document(do_QueryInterface(mResponseXML));
  if (document) {
    dataCharset = document->GetDocumentCharacterSet();
  } else {
    if (NS_FAILED(DetectCharset(dataCharset)) || dataCharset.IsEmpty()) {
      
      dataCharset.AssignLiteral("UTF-8");
    }
  }

  if (dataCharset.EqualsLiteral("ASCII")) {
    CopyASCIItoUTF16(mResponseBody, aOutBuffer);

    return NS_OK;
  }

  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = ccm->GetUnicodeDecoderRaw(dataCharset.get(),
                                 getter_AddRefs(decoder));
  if (NS_FAILED(rv))
    return rv;

  const char * inBuffer = mResponseBody.get();
  PRInt32 outBufferLength;
  rv = decoder->GetMaxLength(inBuffer, dataLen, &outBufferLength);
  if (NS_FAILED(rv))
    return rv;

  PRUnichar * outBuffer =
    static_cast<PRUnichar*>(nsMemory::Alloc((outBufferLength + 1) *
                                               sizeof(PRUnichar)));
  if (!outBuffer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 totalChars = 0,
          outBufferIndex = 0,
          outLen = outBufferLength;

  do {
    PRInt32 inBufferLength = dataLen;
    rv = decoder->Convert(inBuffer,
                          &inBufferLength,
                          &outBuffer[outBufferIndex],
                          &outLen);
    totalChars += outLen;
    if (NS_FAILED(rv)) {
      
      
      outBuffer[outBufferIndex + outLen++] = (PRUnichar)0xFFFD;
      outBufferIndex += outLen;
      outLen = outBufferLength - (++totalChars);

      decoder->Reset();

      if((inBufferLength + 1) > dataLen) {
        inBufferLength = dataLen;
      } else {
        inBufferLength++;
      }

      inBuffer = &inBuffer[inBufferLength];
      dataLen -= inBufferLength;
    }
  } while ( NS_FAILED(rv) && (dataLen > 0) );

  aOutBuffer.Assign(outBuffer, totalChars);
  nsMemory::Free(outBuffer);

  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponseText(nsAString& aResponseText)
{
  nsresult rv = NS_OK;

  aResponseText.Truncate();

  if (mState & (XML_HTTP_REQUEST_COMPLETED |
                XML_HTTP_REQUEST_INTERACTIVE)) {
    rv = ConvertBodyToText(aResponseText);
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatus(PRUint32 *aStatus)
{
  *aStatus = 0;

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return NS_OK;
      }
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsresult rv = httpChannel->GetResponseStatus(aStatus);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      
      
      
      PRInt32 readyState;
      GetReadyState(&readyState);
      if (readyState >= 3) {
        *aStatus = 0;
        return NS_OK;
      }
    }

    return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatusText(nsACString& aStatusText)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  aStatusText.Truncate();

  nsresult rv = NS_OK;

  if (httpChannel) {
    rv = httpChannel->GetResponseStatusText(aStatusText);
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::Abort()
{
  if (mReadRequest) {
    mReadRequest->Cancel(NS_BINDING_ABORTED);
  }
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
  }
  if (mACGetChannel) {
    mACGetChannel->Cancel(NS_BINDING_ABORTED);
  }
  mResponseXML = nsnull;
  PRUint32 responseLength = mResponseBody.Length();
  mResponseBody.Truncate();
  mState |= XML_HTTP_REQUEST_ABORTED;

  if (!(mState & (XML_HTTP_REQUEST_UNINITIALIZED |
                  XML_HTTP_REQUEST_OPENED |
                  XML_HTTP_REQUEST_COMPLETED))) {
    ChangeState(XML_HTTP_REQUEST_COMPLETED, PR_TRUE);
  }

  if (!(mState & XML_HTTP_REQUEST_SYNCLOOPING)) {
    NS_NAMED_LITERAL_STRING(abortStr, ABORT_STR);
    DispatchProgressEvent(this, abortStr, mLoadLengthComputable, responseLength,
                          mLoadTotal);
    if (mUpload && !mUploadComplete) {
      mUploadComplete = PR_TRUE;
      DispatchProgressEvent(mUpload, abortStr, PR_TRUE, mUploadTransferred,
                            mUploadTotal);
    }
  }

  
  
  
  if (mState & XML_HTTP_REQUEST_ABORTED) {
    ChangeState(XML_HTTP_REQUEST_UNINITIALIZED, PR_FALSE);  
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetAllResponseHeaders(char **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    return NS_OK;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsHeaderVisitor *visitor = nsnull;
    NS_NEWXPCOM(visitor, nsHeaderVisitor);
    if (!visitor)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(visitor);

    nsresult rv = httpChannel->VisitResponseHeaders(visitor);
    if (NS_SUCCEEDED(rv))
      *_retval = ToNewCString(visitor->Headers());

    NS_RELEASE(visitor);
    return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseHeader(const nsACString& header,
                                    nsACString& _retval)
{
  nsresult rv = NS_OK;
  _retval.Truncate();

  
  PRBool chrome = PR_FALSE; 
  IsCapabilityEnabled("UniversalXPConnect", &chrome);
  if (!chrome &&
       (header.LowerCaseEqualsASCII("set-cookie") ||
        header.LowerCaseEqualsASCII("set-cookie2"))) {
    NS_WARNING("blocked access to response header");
    _retval.SetIsVoid(PR_TRUE);
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return NS_OK;
      }
    }

    const char *kCrossOriginSafeHeaders[] = {
      "cache-control", "content-language", "content-type", "expires",
      "last-modified", "pragma"
    };
    PRBool safeHeader = PR_FALSE;
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(kCrossOriginSafeHeaders); ++i) {
      if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
        safeHeader = PR_TRUE;
        break;
      }
    }

    if (!safeHeader) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    rv = httpChannel->GetResponseHeader(header, _retval);
  }

  if (rv == NS_ERROR_NOT_AVAILABLE) {
    
    _retval.SetIsVoid(PR_TRUE);
    rv = NS_OK;
  }

  return rv;
}

nsresult
nsXMLHttpRequest::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  NS_ENSURE_ARG_POINTER(aLoadGroup);
  *aLoadGroup = nsnull;

  if (mState & XML_HTTP_REQUEST_BACKGROUND) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
  if (doc) {
    *aLoadGroup = doc->GetDocumentLoadGroup().get();  
  }

  return NS_OK;
}

nsresult
nsXMLHttpRequest::CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent)
{
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("Events"),
                                               aDOMEvent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(*aDOMEvent));
  if (!privevent) {
    NS_IF_RELEASE(*aDOMEvent);
    return NS_ERROR_FAILURE;
  }

  (*aDOMEvent)->InitEvent(NS_LITERAL_STRING(READYSTATE_STR),
                          PR_FALSE, PR_FALSE);

  
  privevent->SetTrusted(PR_TRUE);

  return NS_OK;
}

void
nsXMLHttpRequest::DispatchProgressEvent(nsPIDOMEventTarget* aTarget,
                                        const nsAString& aType,
                                        PRBool aUseLSEventWrapper,
                                        PRBool aLengthComputable,
                                        PRUint64 aLoaded, PRUint64 aTotal,
                                        PRUint64 aPosition, PRUint64 aTotalSize)
{
  NS_ASSERTION(aTarget, "null target");
  if (aType.IsEmpty() ||
      (!AllowUploadProgress() &&
       (aTarget == mUpload || aType.EqualsLiteral(UPLOADPROGRESS_STR)))) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("ProgressEvent"),
                                               getter_AddRefs(event));
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(event));
  if (!privevent) {
    return;
  }
  privevent->SetTrusted(PR_TRUE);

  nsCOMPtr<nsIDOMProgressEvent> progress = do_QueryInterface(event);
  if (!progress) {
    return;
  }

  progress->InitProgressEvent(aType, PR_FALSE, PR_FALSE, aLengthComputable,
                              aLoaded, (aTotal == LL_MAXUINT) ? 0 : aTotal);

  if (aUseLSEventWrapper) {
    nsCOMPtr<nsIDOMProgressEvent> xhrprogressEvent =
      new nsXMLHttpProgressEvent(progress, aPosition, aTotalSize);
    if (!xhrprogressEvent) {
      return;
    }
    event = xhrprogressEvent;
  }
  aTarget->DispatchDOMEvent(nsnull, event, nsnull, nsnull);
}

already_AddRefed<nsIHttpChannel>
nsXMLHttpRequest::GetCurrentHttpChannel()
{
  nsIHttpChannel *httpChannel = nsnull;

  if (mReadRequest) {
    CallQueryInterface(mReadRequest, &httpChannel);
  }

  if (!httpChannel && mChannel) {
    CallQueryInterface(mChannel, &httpChannel);
  }

  return httpChannel;
}

inline PRBool
IsSystemPrincipal(nsIPrincipal* aPrincipal)
{
  PRBool isSystem = PR_FALSE;
  nsContentUtils::GetSecurityManager()->
    IsSystemPrincipal(aPrincipal, &isSystem);
  return isSystem;
}

static PRBool
CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel)
{
  NS_ASSERTION(!IsSystemPrincipal(aPrincipal), "Shouldn't get here!");

  nsCOMPtr<nsIURI> channelURI, originalURI;
  nsresult rv = aChannel->GetURI(getter_AddRefs(channelURI));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  rv = aChannel->GetOriginalURI(getter_AddRefs(originalURI));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = aPrincipal->CheckMayLoad(channelURI, PR_FALSE);
  if (NS_SUCCEEDED(rv) && originalURI != channelURI) {
    rv = aPrincipal->CheckMayLoad(originalURI, PR_FALSE);
  }
  return NS_SUCCEEDED(rv);
}

nsresult
nsXMLHttpRequest::CheckChannelForCrossSiteRequest(nsIChannel* aChannel)
{
  nsresult rv;

  
  
  if ((mState & XML_HTTP_REQUEST_XSITEENABLED) ||
      CheckMayLoad(mPrincipal, aChannel)) {
    return NS_OK;
  }

  
  mState |= XML_HTTP_REQUEST_USE_XSITE_AC;

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(httpChannel, NS_ERROR_DOM_BAD_URI);
    
  nsCAutoString method;
  httpChannel->GetRequestMethod(method);
  if (!mACUnsafeHeaders.IsEmpty() ||
      HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
      (mUpload && mUpload->HasListeners())) {
    mState |= XML_HTTP_REQUEST_NEED_AC_PREFLIGHT;
  }
  else if (method.LowerCaseEqualsLiteral("post")) {
    nsCAutoString contentTypeHeader;
    httpChannel->GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                  contentTypeHeader);

    nsCAutoString contentType, charset;
    rv = NS_ParseContentType(contentTypeHeader, contentType, charset);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!contentType.LowerCaseEqualsLiteral("text/plain")) {
      mState |= XML_HTTP_REQUEST_NEED_AC_PREFLIGHT;
    }
  }
  else if (!method.LowerCaseEqualsLiteral("get") &&
           !method.LowerCaseEqualsLiteral("head")) {
    mState |= XML_HTTP_REQUEST_NEED_AC_PREFLIGHT;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OpenRequest(const nsACString& method,
                              const nsACString& url,
                              PRBool async,
                              const nsAString& user,
                              const nsAString& password)
{
  NS_ENSURE_ARG(!method.IsEmpty());
  NS_ENSURE_ARG(!url.IsEmpty());

  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  
  
  if (method.LowerCaseEqualsLiteral("trace") ||
      method.LowerCaseEqualsLiteral("track")) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  PRBool authp = PR_FALSE;

  if (mState & (XML_HTTP_REQUEST_OPENED |
                XML_HTTP_REQUEST_LOADED |
                XML_HTTP_REQUEST_INTERACTIVE |
                XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_STOPPED)) {
    
    Abort();

    
    
    
    
    
  }

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    
    
    
    

    mState &= ~XML_HTTP_REQUEST_ABORTED;
  }

  if (async) {
    mState |= XML_HTTP_REQUEST_ASYNC;
  } else {
    mState &= ~XML_HTTP_REQUEST_ASYNC;
  }

  mState &= ~XML_HTTP_REQUEST_MPART_HEADERS;

  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
  
  nsCOMPtr<nsIURI> baseURI;
  if (mBaseURI) {
    baseURI = mBaseURI;
  }
  else if (doc) {
    baseURI = doc->GetBaseURI();
  }

  rv = NS_NewURI(getter_AddRefs(uri), url, nsnull, baseURI);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                                 uri,
                                 mPrincipal,
                                 doc,
                                 EmptyCString(), 
                                 nsnull,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  if (!user.IsEmpty()) {
    nsCAutoString userpass;
    CopyUTF16toUTF8(user, userpass);
    if (!password.IsEmpty()) {
      userpass.Append(':');
      AppendUTF16toUTF8(password, userpass);
    }
    uri->SetUserPass(userpass);
    authp = PR_TRUE;
  }

  
  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(getter_AddRefs(loadGroup));

  
  
  
  
  nsLoadFlags loadFlags;
  if (HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)) ||
      HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
      (mUpload && mUpload->HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)))) {
    loadFlags = nsIRequest::LOAD_NORMAL;
  } else {
    loadFlags = nsIRequest::LOAD_BACKGROUND;
  }
  rv = NS_NewChannel(getter_AddRefs(mChannel), uri, nsnull, loadGroup, nsnull,
                     loadFlags);
  if (NS_FAILED(rv)) return rv;

  
  if (IsSystemPrincipal(mPrincipal)) {
    
    mState |= XML_HTTP_REQUEST_XSITEENABLED;
  }

  mState &= ~(XML_HTTP_REQUEST_USE_XSITE_AC |
              XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
    rv = httpChannel->SetRequestMethod(method);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ChangeState(XML_HTTP_REQUEST_OPENED);

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::Open(const nsACString& method, const nsACString& url)
{
  nsresult rv = NS_OK;
  PRBool async = PR_TRUE;
  nsAutoString user, password;

  nsAXPCNativeCallContext *cc = nsnull;
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  if (xpc) {
    rv = xpc->GetCurrentNativeCallContext(&cc);
  }

  if (NS_SUCCEEDED(rv) && cc) {
    PRUint32 argc;
    rv = cc->GetArgc(&argc);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    jsval* argv;
    rv = cc->GetArgvPtr(&argv);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    JSContext* cx;
    rv = cc->GetJSContext(&cx);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    
    if (nsContentUtils::IsCallerTrustedForRead()) {
      mState |= XML_HTTP_REQUEST_XSITEENABLED;
    } else {
      mState &= ~XML_HTTP_REQUEST_XSITEENABLED;
    }

    if (argc > 2) {
      JSAutoRequest ar(cx);
      JSBool asyncBool;
      ::JS_ValueToBoolean(cx, argv[2], &asyncBool);
      async = (PRBool)asyncBool;

      if (argc > 3 && !JSVAL_IS_NULL(argv[3]) && !JSVAL_IS_VOID(argv[3])) {
        JSString* userStr = ::JS_ValueToString(cx, argv[3]);

        if (userStr) {
          user.Assign(reinterpret_cast<PRUnichar *>
                                      (::JS_GetStringChars(userStr)),
                      ::JS_GetStringLength(userStr));
        }

        if (argc > 4 && !JSVAL_IS_NULL(argv[4]) && !JSVAL_IS_VOID(argv[4])) {
          JSString* passwdStr = JS_ValueToString(cx, argv[4]);

          if (passwdStr) {
            password.Assign(reinterpret_cast<PRUnichar *>
                                            (::JS_GetStringChars(passwdStr)),
                            ::JS_GetStringLength(passwdStr));
          }
        }
      }
    }
  }

  return OpenRequest(method, url, async, user, password);
}




NS_METHOD
nsXMLHttpRequest::StreamReaderFunc(nsIInputStream* in,
                                   void* closure,
                                   const char* fromRawSegment,
                                   PRUint32 toOffset,
                                   PRUint32 count,
                                   PRUint32 *writeCount)
{
  nsXMLHttpRequest* xmlHttpRequest = static_cast<nsXMLHttpRequest*>(closure);
  if (!xmlHttpRequest || !writeCount) {
    NS_WARNING("XMLHttpRequest cannot read from stream: no closure or writeCount");
    return NS_ERROR_FAILURE;
  }

  
  xmlHttpRequest->mResponseBody.Append(fromRawSegment,count);

  nsresult rv = NS_OK;

  if (xmlHttpRequest->mState & XML_HTTP_REQUEST_PARSEBODY) {
    

    
    
    
    nsCOMPtr<nsIInputStream> copyStream;
    rv = NS_NewByteInputStream(getter_AddRefs(copyStream), fromRawSegment, count);

    if (NS_SUCCEEDED(rv) && xmlHttpRequest->mXMLParserStreamListener) {
      NS_ASSERTION(copyStream, "NS_NewByteInputStream lied");
      nsresult parsingResult = xmlHttpRequest->mXMLParserStreamListener
                                  ->OnDataAvailable(xmlHttpRequest->mReadRequest,
                                                    xmlHttpRequest->mContext,
                                                    copyStream, toOffset, count);

      
      
      if (NS_FAILED(parsingResult)) {
        xmlHttpRequest->mState &= ~XML_HTTP_REQUEST_PARSEBODY;
      }
    }
  }

  xmlHttpRequest->ChangeState(XML_HTTP_REQUEST_INTERACTIVE);

  if (NS_SUCCEEDED(rv)) {
    *writeCount = count;
  } else {
    *writeCount = 0;
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  NS_ENSURE_ARG_POINTER(inStr);

  NS_ABORT_IF_FALSE(mContext.get() == ctxt,"start context different from OnDataAvailable context");

  PRUint32 totalRead;
  return inStr->ReadSegments(nsXMLHttpRequest::StreamReaderFunc, (void*)this, count, &totalRead);
}

PRBool
IsSameOrBaseChannel(nsIRequest* aPossibleBase, nsIChannel* aChannel)
{
  nsCOMPtr<nsIMultiPartChannel> mpChannel = do_QueryInterface(aPossibleBase);
  if (mpChannel) {
    nsCOMPtr<nsIChannel> baseChannel;
    nsresult rv = mpChannel->GetBaseChannel(getter_AddRefs(baseChannel));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    
    return baseChannel == aChannel;
  }

  return aPossibleBase == aChannel;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv = NS_OK;
  if (!mFirstStartRequestSeen && mRequestObserver) {
    mFirstStartRequestSeen = PR_TRUE;
    mRequestObserver->OnStartRequest(request, ctxt);
  }

  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED)
    return NS_OK;

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    NS_ERROR("Ugh, still getting data on an aborted XMLHttpRequest!");

    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIPrincipal> documentPrincipal = mPrincipal;
  if (IsSystemPrincipal(documentPrincipal)) {
    
    
    
    
    nsresult rv;
    documentPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  channel->SetOwner(documentPrincipal);

  mReadRequest = request;
  mContext = ctxt;
  mState |= XML_HTTP_REQUEST_PARSEBODY;
  mState &= ~XML_HTTP_REQUEST_MPART_HEADERS;
  ChangeState(XML_HTTP_REQUEST_LOADED);

  nsresult status;
  request->GetStatus(&status);
  mErrorLoad = mErrorLoad || NS_FAILED(status);

  if (mUpload && !mUploadComplete && !mErrorLoad &&
      (mState & XML_HTTP_REQUEST_ASYNC)) {
    mUploadComplete = PR_TRUE;
    DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOAD_STR),
                          PR_TRUE, mUploadTotal, mUploadTotal);
  }

  
  mResponseBody.Truncate();

  
  PRBool parseBody = PR_TRUE;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
    nsCAutoString method;
    httpChannel->GetRequestMethod(method);
    parseBody = !method.EqualsLiteral("HEAD");
  }

  if (parseBody && NS_SUCCEEDED(status)) {
    if (!mOverrideMimeType.IsEmpty()) {
      channel->SetContentType(mOverrideMimeType);
    }

    
    
    
    
    nsCAutoString type;
    channel->GetContentType(type);

    if (type.Find("xml") == kNotFound) {
      mState &= ~XML_HTTP_REQUEST_PARSEBODY;
    }
  } else {
    
    mState &= ~XML_HTTP_REQUEST_PARSEBODY;
  }

  if (mState & XML_HTTP_REQUEST_PARSEBODY) {
    nsCOMPtr<nsIURI> baseURI, docURI;
    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(mScriptContext);

    if (doc) {
      docURI = doc->GetDocumentURI();
      baseURI = doc->GetBaseURI();
    }

    
    
    
    
    const nsAString& emptyStr = EmptyString();
    nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(mOwner);
    rv = nsContentUtils::CreateDocument(emptyStr, emptyStr, nsnull, docURI,
                                        baseURI, mPrincipal, global,
                                        getter_AddRefs(mResponseXML));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> responseDoc = do_QueryInterface(mResponseXML);
    responseDoc->SetPrincipal(documentPrincipal);

    if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mResponseXML);
      if (htmlDoc) {
        htmlDoc->DisableCookieAccess();
      }
    }

    
    nsCOMPtr<nsPIDOMEventTarget> target(do_QueryInterface(mResponseXML));
    if (target) {
      nsWeakPtr requestWeak =
        do_GetWeakReference(static_cast<nsIXMLHttpRequest*>(this));
      nsCOMPtr<nsIDOMEventListener> proxy = new nsLoadListenerProxy(requestWeak);
      if (!proxy) return NS_ERROR_OUT_OF_MEMORY;

      
      rv = target->AddEventListenerByIID(static_cast<nsIDOMEventListener*>
                                                    (proxy),
                                         NS_GET_IID(nsIDOMLoadListener));
      NS_ENSURE_SUCCESS(rv, rv);
    }



    nsCOMPtr<nsIStreamListener> listener;
    nsCOMPtr<nsILoadGroup> loadGroup;
    channel->GetLoadGroup(getter_AddRefs(loadGroup));

    rv = responseDoc->StartDocumentLoad(kLoadAsData, channel, loadGroup,
                                        nsnull, getter_AddRefs(listener),
                                        !(mState & XML_HTTP_REQUEST_USE_XSITE_AC));
    NS_ENSURE_SUCCESS(rv, rv);

    mXMLParserStreamListener = listener;
    rv = mXMLParserStreamListener->OnStartRequest(request, ctxt);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (NS_SUCCEEDED(rv) &&
      (mState & XML_HTTP_REQUEST_ASYNC) &&
      HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR))) {
    StartProgressEventTimer();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  
  
  
  
  
  
  nsCOMPtr<nsIMultiPartChannel> mpChannel = do_QueryInterface(request);
  if (mpChannel) {
    PRBool last;
    rv = mpChannel->GetIsLastPart(&last);
    NS_ENSURE_SUCCESS(rv, rv);
    if (last) {
      mState |= XML_HTTP_REQUEST_GOT_FINAL_STOP;
    }
  }
  else {
    mState |= XML_HTTP_REQUEST_GOT_FINAL_STOP;
  }

  if (mRequestObserver && mState & XML_HTTP_REQUEST_GOT_FINAL_STOP) {
    NS_ASSERTION(mFirstStartRequestSeen, "Inconsistent state!");
    mFirstStartRequestSeen = PR_FALSE;
    mRequestObserver->OnStopRequest(request, ctxt, status);
  }

  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED)
    return NS_OK;

  nsCOMPtr<nsIParser> parser;

  
  if (mState & XML_HTTP_REQUEST_PARSEBODY && mXMLParserStreamListener) {
    parser = do_QueryInterface(mXMLParserStreamListener);
    NS_ABORT_IF_FALSE(parser, "stream listener was expected to be a parser");
    rv = mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
  }

  mXMLParserStreamListener = nsnull;
  mReadRequest = nsnull;
  mContext = nsnull;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  channel->SetNotificationCallbacks(nsnull);
  mNotificationCallbacks = nsnull;
  mChannelEventSink = nsnull;
  mProgressEventSink = nsnull;

  if (NS_FAILED(status)) {
    
    
    Error(nsnull);

    
    
    
    
    mChannel = nsnull;
  } else if (!parser || parser->IsParserEnabled()) {
    
    
    
    
    
    
    
    
    
    
    RequestCompleted();
  } else {
    ChangeState(XML_HTTP_REQUEST_STOPPED, PR_FALSE);
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return rv;
}

nsresult
nsXMLHttpRequest::RequestCompleted()
{
  nsresult rv = NS_OK;

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  
  
  
  if (mState & (XML_HTTP_REQUEST_UNINITIALIZED |
                XML_HTTP_REQUEST_COMPLETED)) {
    return NS_OK;
  }

  
  
  
  
  if (mResponseXML) {
    nsCOMPtr<nsIDOMElement> root;
    mResponseXML->GetDocumentElement(getter_AddRefs(root));
    if (!root) {
      mResponseXML = nsnull;
    }
  }

  ChangeState(XML_HTTP_REQUEST_COMPLETED, PR_TRUE);

  PRUint32 responseLength = mResponseBody.Length();
  NS_NAMED_LITERAL_STRING(errorStr, ERROR_STR);
  NS_NAMED_LITERAL_STRING(loadStr, LOAD_STR);
  DispatchProgressEvent(this,
                        mErrorLoad ? errorStr : loadStr,
                        !mErrorLoad,
                        responseLength,
                        mErrorLoad ? 0 : responseLength);
  if (mErrorLoad && mUpload && !mUploadComplete) {
    DispatchProgressEvent(mUpload, errorStr, PR_TRUE,
                          mUploadTransferred, mUploadTotal);
  }

  if (!(mState & XML_HTTP_REQUEST_GOT_FINAL_STOP)) {
    
    ChangeState(XML_HTTP_REQUEST_OPENED);
  }

  nsJSContext::MaybeCC(PR_FALSE);
  return rv;
}

NS_IMETHODIMP
nsXMLHttpRequest::SendAsBinary(const nsAString &aBody)
{
  char *data = static_cast<char*>(NS_Alloc(aBody.Length() + 1));
  if (!data)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAString::const_iterator iter, end;
  aBody.BeginReading(iter);
  aBody.EndReading(end);
  char *p = data;
  while (iter != end) {
    if (*iter & 0xFF00) {
      NS_Free(data);
      return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
    }
    *p++ = static_cast<char>(*iter++);
  }
  *p = '\0';

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream), data,
                                      aBody.Length(), NS_ASSIGNMENT_ADOPT);
  if (NS_FAILED(rv))
    NS_Free(data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> variant = new nsVariant();
  if (!variant) return NS_ERROR_OUT_OF_MEMORY;

  rv = variant->SetAsISupports(stream);
  NS_ENSURE_SUCCESS(rv, rv);

  return Send(variant);
}


NS_IMETHODIMP
nsXMLHttpRequest::Send(nsIVariant *aBody)
{
  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }

  
  if (!mChannel || !(XML_HTTP_REQUEST_OPENED & mState)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  

  
  
  nsCAutoString method;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));

  if (httpChannel) {
    httpChannel->GetRequestMethod(method); 

    if (!IsSystemPrincipal(mPrincipal)) {
      nsCOMPtr<nsIURI> codebase;
      mPrincipal->GetURI(getter_AddRefs(codebase));

      httpChannel->SetReferrer(codebase);
    }
  }

  mUploadTransferred = 0;
  mUploadTotal = 0;
  
  mUploadComplete = PR_TRUE;
  mErrorLoad = PR_FALSE;
  mLoadLengthComputable = PR_FALSE;
  mLoadTotal = 0;
  mUploadProgress = 0;
  mUploadProgressMax = 0;
  if (aBody && httpChannel && !method.EqualsLiteral("GET")) {
    nsXPIDLString serial;
    nsCOMPtr<nsIInputStream> postDataStream;
    nsCAutoString charset(NS_LITERAL_CSTRING("UTF-8"));
    nsCAutoString defaultContentType(NS_LITERAL_CSTRING("text/plain"));

    PRUint16 dataType;
    rv = aBody->GetDataType(&dataType);
    if (NS_FAILED(rv))
      return rv;

    switch (dataType) {
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
      {
        nsCOMPtr<nsISupports> supports;
        nsID *iid;
        rv = aBody->GetAsInterface(&iid, getter_AddRefs(supports));
        if (NS_FAILED(rv))
          return rv;
        if (iid)
          nsMemory::Free(iid);

        
        nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(supports));
        if (doc) {
          defaultContentType.AssignLiteral("application/xml");

          nsCOMPtr<nsIDOMSerializer> serializer(do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv));
          if (NS_FAILED(rv)) return rv;

          nsCOMPtr<nsIDOM3Document> dom3doc(do_QueryInterface(doc));
          if (dom3doc) {
            nsAutoString inputEncoding;
            dom3doc->GetInputEncoding(inputEncoding);
            if (!DOMStringIsNull(inputEncoding)) {
              CopyUTF16toUTF8(inputEncoding, charset);
            }
          }

          
          
          nsCOMPtr<nsIStorageStream> storStream;
          rv = NS_NewStorageStream(4096, PR_UINT32_MAX, getter_AddRefs(storStream));
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<nsIOutputStream> output;
          rv = storStream->GetOutputStream(0, getter_AddRefs(output));
          NS_ENSURE_SUCCESS(rv, rv);

          
          rv = serializer->SerializeToStream(doc, output, charset);
          NS_ENSURE_SUCCESS(rv, rv);

          output->Close();
          rv = storStream->NewInputStream(0, getter_AddRefs(postDataStream));
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          
          nsCOMPtr<nsISupportsString> wstr(do_QueryInterface(supports));
          if (wstr) {
            wstr->GetData(serial);
          } else {
            
            nsCOMPtr<nsIInputStream> stream(do_QueryInterface(supports));
            if (stream) {
              postDataStream = stream;
              charset.Truncate();
            }
            else {
              
              nsCOMPtr<nsIDOMFileInternal> file(do_QueryInterface(supports));

              if (file) {
                nsCOMPtr<nsIFile> internalFile;
                rv = file->GetInternalFile(getter_AddRefs(internalFile));
                NS_ENSURE_SUCCESS(rv, rv);

                nsCOMPtr<nsIInputStream> stream;
                rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), internalFile); 
                NS_ENSURE_SUCCESS(rv, rv);

                
                if (stream) {
                  postDataStream = stream;
                  charset.Truncate();
                  defaultContentType.Truncate();

                  nsCOMPtr<nsIMIMEService> mimeService =
                      do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
                  NS_ENSURE_SUCCESS(rv, rv);

                  nsCAutoString mediaType;
                  rv = mimeService->GetTypeFromFile(internalFile, mediaType);
                  if (NS_SUCCEEDED(rv)) {
                    defaultContentType = mediaType;
                  }
                }
              }
            }
          }
        }
      }
      break;
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
      
      break;
    default:
      
      rv = aBody->GetAsWString(getter_Copies(serial));
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }

    if (serial) {
      
      nsCOMPtr<nsIScriptableUnicodeConverter> converter =
        do_CreateInstance("@mozilla.org/intl/scriptableunicodeconverter", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = converter->SetCharset("UTF-8");
      NS_ENSURE_SUCCESS(rv, rv);

      rv = converter->ConvertToInputStream(serial,
                                           getter_AddRefs(postDataStream));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (postDataStream) {
      nsCOMPtr<nsIUploadChannel2> uploadChannel(do_QueryInterface(httpChannel));
      NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

      
      
      nsCAutoString contentType;
      if (NS_FAILED(httpChannel->
                      GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentType)) ||
          contentType.IsEmpty()) {
        contentType = defaultContentType;
      }

      
      if (!charset.IsEmpty()) {
        nsCAutoString specifiedCharset;
        PRBool haveCharset;
        PRInt32 charsetStart, charsetEnd;
        rv = NS_ExtractCharsetFromContentType(contentType, specifiedCharset,
                                              &haveCharset, &charsetStart,
                                              &charsetEnd);
        if (NS_SUCCEEDED(rv)) {
          
          
          
          
          
          
          
          if (!specifiedCharset.Equals(charset,
                                       nsCaseInsensitiveCStringComparator())) {
            nsCAutoString newCharset("; charset=");
            newCharset.Append(charset);
            contentType.Replace(charsetStart, charsetEnd - charsetStart,
                                newCharset);
          }
        }
      }

      
      
      if (!NS_InputStreamIsBuffered(postDataStream)) {
        nsCOMPtr<nsIInputStream> bufferedStream;
        rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                       postDataStream, 
                                       4096);
        NS_ENSURE_SUCCESS(rv, rv);

        postDataStream = bufferedStream;
      }

      mUploadComplete = PR_FALSE;
      PRUint32 uploadTotal = 0;
      postDataStream->Available(&uploadTotal);
      mUploadTotal = uploadTotal;

      
      
      rv = uploadChannel->ExplicitSetUploadStream(postDataStream, contentType, -1, method, PR_FALSE);
    }
  }

  
  mResponseBody.Truncate();

  
  mResponseXML = nsnull;

  rv = CheckChannelForCrossSiteRequest(mChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool withCredentials = !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);

  
  if (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT) {
    
    
    nsCOMPtr<nsIURI> uri;
    rv = mChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAccessControlLRUCache::CacheEntry* entry =
      sAccessControlCache ?
      sAccessControlCache->GetEntry(uri, mPrincipal, withCredentials, PR_FALSE) :
      nsnull;

    if (!entry || !entry->CheckRequest(method, mACUnsafeHeaders)) {
      
      
      nsCOMPtr<nsILoadGroup> loadGroup;
      GetLoadGroup(getter_AddRefs(loadGroup));

      nsLoadFlags loadFlags;
      rv = mChannel->GetLoadFlags(&loadFlags);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_NewChannel(getter_AddRefs(mACGetChannel), uri, nsnull,
                         loadGroup, nsnull, loadFlags);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIHttpChannel> acHttp = do_QueryInterface(mACGetChannel);
      NS_ASSERTION(acHttp, "Failed to QI to nsIHttpChannel!");

      rv = acHttp->SetRequestMethod(NS_LITERAL_CSTRING("OPTIONS"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  mChannel->GetNotificationCallbacks(getter_AddRefs(mNotificationCallbacks));
  mChannel->SetNotificationCallbacks(this);

  
  nsCOMPtr<nsIStreamListener> listener = this;
  if (mState & XML_HTTP_REQUEST_MULTIPART) {
    listener = new nsMultipartProxyListener(listener);
    if (!listener) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!(mState & XML_HTTP_REQUEST_XSITEENABLED)) {
    
    
    listener = new nsCrossSiteListenerProxy(listener, mPrincipal, mChannel,
                                            withCredentials, &rv);
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  if ((mState & XML_HTTP_REQUEST_MULTIPART) || method.EqualsLiteral("POST")) {
    AddLoadFlags(mChannel,
        nsIRequest::LOAD_BYPASS_CACHE | nsIRequest::INHIBIT_CACHING);
  }
  
  
  
  else if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    AddLoadFlags(mChannel,
        nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
    if (mACGetChannel) {
      AddLoadFlags(mACGetChannel,
          nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
    }
  }

  
  
  
  mChannel->SetContentType(NS_LITERAL_CSTRING("application/xml"));

  
  
  if (mACGetChannel) {
    nsCOMPtr<nsIStreamListener> acProxyListener =
      new nsACProxyListener(mChannel, listener, nsnull, mPrincipal, method,
                            withCredentials);
    NS_ENSURE_TRUE(acProxyListener, NS_ERROR_OUT_OF_MEMORY);

    acProxyListener =
      new nsCrossSiteListenerProxy(acProxyListener, mPrincipal, mACGetChannel,
                                   withCredentials, method, mACUnsafeHeaders,
                                   &rv);
    NS_ENSURE_TRUE(acProxyListener, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mACGetChannel->AsyncOpen(acProxyListener, nsnull);
  }
  else {
    
    rv = mChannel->AsyncOpen(listener, nsnull);
  }

  if (NS_FAILED(rv)) {
    
    mChannel = nsnull;
    mACGetChannel = nsnull;
    return rv;
  }

  
  
  
  
  ChangeState(XML_HTTP_REQUEST_SENT);

  
  if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    mState |= XML_HTTP_REQUEST_SYNCLOOPING;

    nsCOMPtr<nsIDocument> suspendedDoc;
    nsCOMPtr<nsIRunnable> resumeTimeoutRunnable;
    if (mOwner) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      if (NS_SUCCEEDED(mOwner->GetTop(getter_AddRefs(topWindow)))) {
        nsCOMPtr<nsPIDOMWindow> suspendedWindow(do_QueryInterface(topWindow));
        if (suspendedWindow &&
            (suspendedWindow = suspendedWindow->GetCurrentInnerWindow())) {
          suspendedDoc = do_QueryInterface(suspendedWindow->GetExtantDocument());
          if (suspendedDoc) {
            suspendedDoc->SuppressEventHandling();
          }
          suspendedWindow->SuspendTimeouts(1, PR_FALSE);
          resumeTimeoutRunnable = new nsResumeTimeoutsEvent(suspendedWindow);
        }
      }
    }

    nsIThread *thread = NS_GetCurrentThread();
    while (mState & XML_HTTP_REQUEST_SYNCLOOPING) {
      if (!NS_ProcessNextEvent(thread)) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
    }

    if (suspendedDoc) {
      suspendedDoc->UnsuppressEventHandlingAndFireEvents(PR_TRUE);
    }

    if (resumeTimeoutRunnable) {
      NS_DispatchToCurrentThread(resumeTimeoutRunnable);
    }
  } else {
    if (!mUploadComplete &&
        HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
        (mUpload && mUpload->HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)))) {
      StartProgressEventTimer();
    }
    DispatchProgressEvent(this, NS_LITERAL_STRING(LOADSTART_STR), PR_FALSE,
                          0, 0);
    if (mUpload && !mUploadComplete) {
      DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOADSTART_STR), PR_TRUE,
                            0, mUploadTotal);
    }
  }

  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetRequestHeader(const nsACString& header,
                                   const nsACString& value)
{
  nsresult rv;

  
  if (!IsValidHTTPToken(header)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  if (mACGetChannel) {
    PRBool pending;
    rv = mACGetChannel->IsPending(&pending);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (pending) {
      return NS_ERROR_IN_PROGRESS;
    }
  }

  if (!mChannel)             
    return NS_ERROR_FAILURE; 

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (!httpChannel) {
    return NS_OK;
  }

  
  

  PRBool privileged;
  rv = IsCapabilityEnabled("UniversalBrowserWrite", &privileged);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (!privileged) {
    
    const char *kInvalidHeaders[] = {
      "accept-charset", "accept-encoding", "connection", "content-length",
      "content-transfer-encoding", "date", "expect", "host", "keep-alive",
      "referer", "te", "trailer", "transfer-encoding", "upgrade", "via"
    };
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(kInvalidHeaders); ++i) {
      if (header.LowerCaseEqualsASCII(kInvalidHeaders[i])) {
        NS_WARNING("refusing to set request header");
        return NS_OK;
      }
    }
    if (StringBeginsWith(header, NS_LITERAL_CSTRING("proxy-"),
                         nsCaseInsensitiveCStringComparator()) ||
        StringBeginsWith(header, NS_LITERAL_CSTRING("sec-"),
                         nsCaseInsensitiveCStringComparator())) {
      NS_WARNING("refusing to set request header");
      return NS_OK;
    }

    
    PRBool safeHeader = !!(mState & XML_HTTP_REQUEST_XSITEENABLED);
    if (!safeHeader) {
      const char *kCrossOriginSafeHeaders[] = {
        "accept", "accept-language", "content-type"
      };
      for (i = 0; i < NS_ARRAY_LENGTH(kCrossOriginSafeHeaders); ++i) {
        if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
          safeHeader = PR_TRUE;
          break;
        }
      }
    }

    if (!safeHeader) {
      mACUnsafeHeaders.AppendElement(header);
    }
  }

  
  return httpChannel->SetRequestHeader(header, value, PR_FALSE);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetReadyState(PRInt32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED) {
    *aState = 0; 
  } else  if (mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
    *aState = 1; 
  } else if (mState & XML_HTTP_REQUEST_LOADED) {
    *aState = 2; 
  } else if (mState & (XML_HTTP_REQUEST_INTERACTIVE | XML_HTTP_REQUEST_STOPPED)) {
    *aState = 3; 
  } else if (mState & XML_HTTP_REQUEST_COMPLETED) {
    *aState = 4; 
  } else {
    NS_ERROR("Should not happen");
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OverrideMimeType(const nsACString& aMimeType)
{
  
  mOverrideMimeType.Assign(aMimeType);
  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::GetMultipart(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_MULTIPART);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetMultipart(PRBool aMultipart)
{
  if (!(mState & XML_HTTP_REQUEST_UNINITIALIZED)) {
    
    return NS_ERROR_IN_PROGRESS;
  }

  if (aMultipart) {
    mState |= XML_HTTP_REQUEST_MULTIPART;
  } else {
    mState &= ~XML_HTTP_REQUEST_MULTIPART;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetMozBackgroundRequest(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_BACKGROUND);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetMozBackgroundRequest(PRBool aMozBackgroundRequest)
{
  PRBool privileged;

  nsresult rv = IsCapabilityEnabled("UniversalXPConnect", &privileged);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!privileged)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (!(mState & XML_HTTP_REQUEST_UNINITIALIZED)) {
    
    return NS_ERROR_IN_PROGRESS;
  }

  if (aMozBackgroundRequest) {
    mState |= XML_HTTP_REQUEST_BACKGROUND;
  } else {
    mState &= ~XML_HTTP_REQUEST_BACKGROUND;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetWithCredentials(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetWithCredentials(PRBool aWithCredentials)
{
  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }
  
  if (aWithCredentials) {
    mState |= XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  }
  else {
    mState &= ~XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  }
  return NS_OK;
}



nsresult
nsXMLHttpRequest::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



nsresult
nsXMLHttpRequest::Load(nsIDOMEvent* aEvent)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mState & XML_HTTP_REQUEST_STOPPED) {
    RequestCompleted();
  }
  return NS_OK;
}

nsresult
nsXMLHttpRequest::Unload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsXMLHttpRequest::BeforeUnload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsXMLHttpRequest::Abort(nsIDOMEvent* aEvent)
{
  Abort();

  return NS_OK;
}

nsresult
nsXMLHttpRequest::Error(nsIDOMEvent* aEvent)
{
  mResponseXML = nsnull;
  ChangeState(XML_HTTP_REQUEST_COMPLETED);

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  DispatchProgressEvent(this, NS_LITERAL_STRING(ERROR_STR), PR_FALSE,
                        mResponseBody.Length(), 0);
  if (mUpload && !mUploadComplete) {
    mUploadComplete = PR_TRUE;
    DispatchProgressEvent(mUpload, NS_LITERAL_STRING(ERROR_STR), PR_TRUE,
                          mUploadTransferred, mUploadTotal);
  }

  nsJSContext::MaybeCC(PR_FALSE);
  return NS_OK;
}

nsresult
nsXMLHttpRequest::ChangeState(PRUint32 aState, PRBool aBroadcast)
{
  
  
  if (aState & XML_HTTP_REQUEST_LOADSTATES) {
    mState &= ~XML_HTTP_REQUEST_LOADSTATES;
  }
  mState |= aState;
  nsresult rv = NS_OK;

  if (mProgressNotifier &&
      !(aState & (XML_HTTP_REQUEST_LOADED | XML_HTTP_REQUEST_INTERACTIVE))) {
    mTimerIsActive = PR_FALSE;
    mProgressNotifier->Cancel();
  }

  if ((mState & XML_HTTP_REQUEST_ASYNC) &&
      (aState & XML_HTTP_REQUEST_LOADSTATES) && 
      aBroadcast) {
    nsCOMPtr<nsIDOMEvent> event;
    rv = CreateReadystatechangeEvent(getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    DispatchDOMEvent(nsnull, event, nsnull, nsnull);
  }

  return rv;
}




NS_IMETHODIMP
nsXMLHttpRequest::OnChannelRedirect(nsIChannel *aOldChannel,
                                    nsIChannel *aNewChannel,
                                    PRUint32    aFlags)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsresult rv;

  if (!NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags)) {
    rv = CheckChannelForCrossSiteRequest(aNewChannel);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    if ((mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT)) {
       return NS_ERROR_DOM_BAD_URI;
    }
  }

  if (mChannelEventSink) {
    rv =
      mChannelEventSink->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
    if (NS_FAILED(rv)) {
      mErrorLoad = PR_TRUE;
      return rv;
    }
  }

  mChannel = aNewChannel;

  return NS_OK;
}





NS_IMETHODIMP
nsXMLHttpRequest::OnProgress(nsIRequest *aRequest, nsISupports *aContext, PRUint64 aProgress, PRUint64 aProgressMax)
{
  
  
  
  if (XML_HTTP_REQUEST_MPART_HEADERS & mState) {
    return NS_OK;
  }

  
  
  PRBool upload = !!((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState);
  PRUint64 loaded = aProgress;
  PRUint64 total = aProgressMax;
  
  
  PRBool lengthComputable = (aProgressMax != LL_MAXUINT);
  if (upload) {
   if (lengthComputable) {
      PRUint64 headerSize = aProgressMax - mUploadTotal;
      loaded -= headerSize;
      total -= headerSize;
    }
    mUploadTransferred = loaded;
    mUploadProgress = aProgress;
    mUploadProgressMax = aProgressMax;
  } else {
    mLoadLengthComputable = lengthComputable;
    mLoadTotal = mLoadLengthComputable ? total : 0;
  }

  if (mTimerIsActive) {
    
    mProgressEventWasDelayed = PR_TRUE;
    return NS_OK;
  }

  if (!mErrorLoad && (mState & XML_HTTP_REQUEST_ASYNC)) {
    StartProgressEventTimer();
    NS_NAMED_LITERAL_STRING(progress, PROGRESS_STR);
    NS_NAMED_LITERAL_STRING(uploadprogress, UPLOADPROGRESS_STR);
    DispatchProgressEvent(this, upload ? uploadprogress : progress, PR_TRUE,
                          lengthComputable, loaded, lengthComputable ? total : 0,
                          aProgress, aProgressMax);

    if (upload && mUpload && !mUploadComplete) {
      NS_WARN_IF_FALSE(mUploadTotal == total, "Wrong upload total?");
      DispatchProgressEvent(mUpload, progress,  PR_TRUE, lengthComputable, loaded,
                            lengthComputable ? total : 0, aProgress, aProgressMax);
    }
  }

  if (mProgressEventSink) {
    mProgressEventSink->OnProgress(aRequest, aContext, aProgress,
                                   aProgressMax);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::OnStatus(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatus, const PRUnichar *aStatusArg)
{
  if (mProgressEventSink) {
    mProgressEventSink->OnStatus(aRequest, aContext, aStatus, aStatusArg);
  }

  return NS_OK;
}

PRBool
nsXMLHttpRequest::AllowUploadProgress()
{
  return !(mState & XML_HTTP_REQUEST_USE_XSITE_AC) ||
    (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);
}




NS_IMETHODIMP
nsXMLHttpRequest::GetInterface(const nsIID & aIID, void **aResult)
{
  nsresult rv;

  
  
  
  
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    mChannelEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    mProgressEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIProgressEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  
  
  if (mNotificationCallbacks) {
    rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }

  if (mState & XML_HTTP_REQUEST_BACKGROUND) {
    nsCOMPtr<nsIInterfaceRequestor> badCertHandler(do_CreateInstance(NS_BADCERTHANDLER_CONTRACTID, &rv));

    
    
    if (NS_SUCCEEDED(rv)) {
      rv = badCertHandler->GetInterface(aIID, aResult);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
           aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    nsCOMPtr<nsIDOMWindow> window;
    if (mOwner) {
      window = mOwner->GetOuterWindow();
    }

    return wwatch->GetPrompt(window, aIID,
                             reinterpret_cast<void**>(aResult));

  }

  return QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsXMLHttpRequest::GetUpload(nsIXMLHttpRequestUpload** aUpload)
{
  *aUpload = nsnull;

  nsresult rv;
  nsIScriptContext* scriptContext =
    GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mUpload) {
    mUpload = new nsXMLHttpRequestUpload(mOwner, scriptContext);
    NS_ENSURE_TRUE(mUpload, NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ADDREF(*aUpload = mUpload);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::Notify(nsITimer* aTimer)
{
  mTimerIsActive = PR_FALSE;
  if (NS_SUCCEEDED(CheckInnerWindowCorrectness()) && !mErrorLoad &&
      (mState & XML_HTTP_REQUEST_ASYNC)) {
    if (mProgressEventWasDelayed) {
      mProgressEventWasDelayed = PR_FALSE;
      if (!(XML_HTTP_REQUEST_MPART_HEADERS & mState)) {
        StartProgressEventTimer();
        
        
        if ((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState) {
          DispatchProgressEvent(this, NS_LITERAL_STRING(UPLOADPROGRESS_STR),
                                PR_TRUE, PR_TRUE, mUploadTransferred,
                                mUploadTotal, mUploadProgress,
                                mUploadProgressMax);
          if (mUpload && !mUploadComplete) {
            DispatchProgressEvent(mUpload, NS_LITERAL_STRING(PROGRESS_STR),
                                  PR_TRUE, PR_TRUE, mUploadTransferred,
                                  mUploadTotal, mUploadProgress,
                                  mUploadProgressMax);
          }
        } else {
          DispatchProgressEvent(this, NS_LITERAL_STRING(PROGRESS_STR),
                                mLoadLengthComputable, mResponseBody.Length(),
                                mLoadTotal);
        }
      }
    }
  } else if (mProgressNotifier) {
    mProgressNotifier->Cancel();
  }
  return NS_OK;
}

void
nsXMLHttpRequest::StartProgressEventTimer()
{
  if (!mProgressNotifier) {
    mProgressNotifier = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  if (mProgressNotifier) {
    mProgressEventWasDelayed = PR_FALSE;
    mTimerIsActive = PR_TRUE;
    mProgressNotifier->Cancel();
    mProgressNotifier->InitWithCallback(this, NS_PROGRESS_EVENT_INTERVAL,
                                        nsITimer::TYPE_ONE_SHOT);
  }
}

NS_IMPL_ISUPPORTS1(nsXMLHttpRequest::nsHeaderVisitor, nsIHttpHeaderVisitor)

NS_IMETHODIMP nsXMLHttpRequest::
nsHeaderVisitor::VisitHeader(const nsACString &header, const nsACString &value)
{
    
    PRBool chrome = PR_FALSE; 
    IsCapabilityEnabled("UniversalXPConnect", &chrome);
    if (!chrome &&
         (header.LowerCaseEqualsASCII("set-cookie") ||
          header.LowerCaseEqualsASCII("set-cookie2"))) {
        NS_WARNING("blocked access to response header");
    } else {
        mHeaders.Append(header);
        mHeaders.Append(": ");
        mHeaders.Append(value);
        mHeaders.Append('\n');
    }
    return NS_OK;
}


nsXMLHttpProgressEvent::nsXMLHttpProgressEvent(nsIDOMProgressEvent* aInner,
                                               PRUint64 aCurrentProgress,
                                               PRUint64 aMaxProgress)
{
  mInner = static_cast<nsDOMProgressEvent*>(aInner);
  mCurProgress = aCurrentProgress;
  mMaxProgress = aMaxProgress;
}

nsXMLHttpProgressEvent::~nsXMLHttpProgressEvent()
{}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpProgressEvent)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXMLHttpProgressEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEvent, nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLSProgressEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLHttpProgressEvent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXMLHttpProgressEvent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXMLHttpProgressEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXMLHttpProgressEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInner);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXMLHttpProgressEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mInner,
                                                       nsIDOMProgressEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP nsXMLHttpProgressEvent::GetInput(nsIDOMLSInput * *aInput)
{
  *aInput = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXMLHttpProgressEvent::GetPosition(PRUint32 *aPosition)
{
  
  LL_L2UI(*aPosition, mCurProgress);
  return NS_OK;
}

NS_IMETHODIMP nsXMLHttpProgressEvent::GetTotalSize(PRUint32 *aTotalSize)
{
  
  LL_L2UI(*aTotalSize, mMaxProgress);
  return NS_OK;
}

