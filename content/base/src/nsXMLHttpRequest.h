




































#ifndef nsXMLHttpRequest_h__
#define nsXMLHttpRequest_h__

#include "nsIXMLHttpRequest.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsIHttpChannel.h"
#include "nsIDocument.h"
#include "nsIStreamListener.h"
#include "nsWeakReference.h"
#include "jsapi.h"
#include "nsIScriptContext.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIProgressEventSink.h"
#include "nsCOMArray.h"
#include "nsJSUtils.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIJSNativeInitializer.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMLSProgressEvent.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "prclist.h"
#include "prtime.h"

class nsILoadGroup;

class nsAccessControlLRUCache
{
  struct CacheEntry : public PRCList
  {
    CacheEntry(const nsACString& aKey, PRTime aValue)
    : key(aKey), value(aValue)
    {
      MOZ_COUNT_CTOR(nsAccessControlLRUCache::CacheEntry);
    }
    
    ~CacheEntry()
    {
      MOZ_COUNT_DTOR(nsAccessControlLRUCache::CacheEntry);
    }
    
    nsCString key;
    PRTime value;
  };

public:
  nsAccessControlLRUCache()
  {
    MOZ_COUNT_CTOR(nsAccessControlLRUCache);
    PR_INIT_CLIST(&mList);
  }

  ~nsAccessControlLRUCache()
  {
    Clear();
    MOZ_COUNT_DTOR(nsAccessControlLRUCache);
  }

  PRBool Initialize()
  {
    return mTable.Init();
  }

  void GetEntry(nsIURI* aURI, nsIPrincipal* aPrincipal,
                PRTime* _retval);

  void PutEntry(nsIURI* aURI, nsIPrincipal* aPrincipal,
                PRTime aValue);

  void Clear();

private:
  PRBool GetEntryInternal(const nsACString& aKey, CacheEntry** _retval);

  PR_STATIC_CALLBACK(PLDHashOperator)
    RemoveExpiredEntries(const nsACString& aKey, nsAutoPtr<CacheEntry>& aValue,
                         void* aUserData);

  static PRBool GetCacheKey(nsIURI* aURI, nsIPrincipal* aPrincipal,
                            nsACString& _retval);

  nsClassHashtable<nsCStringHashKey, CacheEntry> mTable;
  PRCList mList;
};

class nsXMLHttpRequest : public nsIXMLHttpRequest,
                         public nsIJSXMLHttpRequest,
                         public nsIDOMLoadListener,
                         public nsIDOMEventTarget,
                         public nsIStreamListener,
                         public nsIChannelEventSink,
                         public nsIProgressEventSink,
                         public nsIInterfaceRequestor,
                         public nsSupportsWeakReference,
                         public nsIJSNativeInitializer
{
public:
  nsXMLHttpRequest();
  virtual ~nsXMLHttpRequest();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIXMLHTTPREQUEST

  
  NS_DECL_NSIJSXMLHTTPREQUEST

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOMEVENTLISTENER

  
  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent);
  NS_IMETHOD Unload(nsIDOMEvent* aEvent);
  NS_IMETHOD Abort(nsIDOMEvent* aEvent);
  NS_IMETHOD Error(nsIDOMEvent* aEvent);

  
  NS_DECL_NSISTREAMLISTENER

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_DECL_NSIPROGRESSEVENTSINK

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                       PRUint32 argc, jsval* argv);

  
  nsresult Init();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXMLHttpRequest, nsIXMLHttpRequest)

  static PRBool EnsureACCache()
  {
    if (sAccessControlCache)
      return PR_TRUE;

    nsAutoPtr<nsAccessControlLRUCache> newCache(new nsAccessControlLRUCache());
    NS_ENSURE_TRUE(newCache, PR_FALSE);

    if (newCache->Initialize()) {
      sAccessControlCache = newCache.forget();
      return PR_TRUE;
    }

    return PR_FALSE;
  }

  static void ShutdownACCache()
  {
    if (sAccessControlCache) {
      delete sAccessControlCache;
      sAccessControlCache = nsnull;
    }
  }

  static nsAccessControlLRUCache* sAccessControlCache;

protected:

  nsresult DetectCharset(nsACString& aCharset);
  nsresult ConvertBodyToText(nsAString& aOutBuffer);
  static NS_METHOD StreamReaderFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                PRUint32 toOffset,
                PRUint32 count,
                PRUint32 *writeCount);
  
  
  
  
  
  
  nsresult ChangeState(PRUint32 aState, PRBool aBroadcast = PR_TRUE,
                       PRBool aClearEventListeners = PR_FALSE);
  nsresult RequestCompleted();
  nsresult GetLoadGroup(nsILoadGroup **aLoadGroup);
  nsIURI *GetBaseURI();

  
  
  
  nsresult CreateEvent(const nsAString& aType, nsIDOMEvent** domevent);

  
  void CopyEventListeners(nsCOMPtr<nsIDOMEventListener>& aListener,
                          const nsCOMArray<nsIDOMEventListener>& aListenerArray,
                          nsCOMArray<nsIDOMEventListener>& aCopy);

  
  
  
  void NotifyEventListeners(const nsCOMArray<nsIDOMEventListener>& aListeners,
                            nsIDOMEvent* aEvent);
  void ClearEventListeners();
  already_AddRefed<nsIHttpChannel> GetCurrentHttpChannel();

  





  nsresult CheckChannelForCrossSiteRequest();

  nsresult CheckInnerWindowCorrectness()
  {
    if (mOwner) {
      NS_ASSERTION(mOwner->IsInnerWindow(), "Should have inner window here!\n");
      nsPIDOMWindow* outer = mOwner->GetOuterWindow();
      if (!outer || outer->GetCurrentInnerWindow() != mOwner) {
        return NS_ERROR_FAILURE;
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsISupports> mContext;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIChannel> mChannel;
  
  nsCOMPtr<nsIRequest> mReadRequest;
  nsCOMPtr<nsIDOMDocument> mDocument;
  nsCOMPtr<nsIChannel> mACGetChannel;

  nsCOMArray<nsIDOMEventListener> mLoadEventListeners;
  nsCOMArray<nsIDOMEventListener> mErrorEventListeners;
  nsCOMArray<nsIDOMEventListener> mProgressEventListeners;
  nsCOMArray<nsIDOMEventListener> mUploadProgressEventListeners;
  nsCOMArray<nsIDOMEventListener> mReadystatechangeEventListeners;
  
  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow>    mOwner; 

  nsCOMPtr<nsIDOMEventListener> mOnLoadListener;
  nsCOMPtr<nsIDOMEventListener> mOnErrorListener;
  nsCOMPtr<nsIDOMEventListener> mOnProgressListener;
  nsCOMPtr<nsIDOMEventListener> mOnUploadProgressListener;
  nsCOMPtr<nsIDOMEventListener> mOnReadystatechangeListener;

  nsCOMPtr<nsIStreamListener> mXMLParserStreamListener;

  
  class nsHeaderVisitor : public nsIHttpHeaderVisitor {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPHEADERVISITOR
    nsHeaderVisitor() { }
    virtual ~nsHeaderVisitor() {}
    const nsACString &Headers() { return mHeaders; }
  private:
    nsCString mHeaders;
  };

  nsCString mResponseBody;

  nsCString mOverrideMimeType;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  




  nsCOMPtr<nsIChannelEventSink> mChannelEventSink;
  nsCOMPtr<nsIProgressEventSink> mProgressEventSink;

  PRUint32 mState;

  
  
  nsTArray<nsCString> mExtraRequestHeaders;
};




class nsXMLHttpProgressEvent : public nsIDOMLSProgressEvent
{
public:
  nsXMLHttpProgressEvent(nsIDOMEvent * aInner, PRUint64 aCurrentProgress, PRUint64 aMaxProgress);
  virtual ~nsXMLHttpProgressEvent();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMLSPROGRESSEVENT
  NS_FORWARD_NSIDOMEVENT(mInner->)

protected:
  nsCOMPtr<nsIDOMEvent> mInner;
  PRUint64 mCurProgress;
  PRUint64 mMaxProgress;
};

#endif
