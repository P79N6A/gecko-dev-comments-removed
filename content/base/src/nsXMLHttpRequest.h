




































#ifndef nsXMLHttpRequest_h__
#define nsXMLHttpRequest_h__

#include "nsIXMLHttpRequest.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsIDOMLoadListener.h"
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
#include "nsIJSNativeInitializer.h"
#include "nsIDOMLSProgressEvent.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "prclist.h"
#include "prtime.h"
#include "nsIDOMNSEvent.h"
#include "nsITimer.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMProgressEvent.h"
#include "nsDOMEventTargetHelper.h"

class nsILoadGroup;

class nsAccessControlLRUCache
{
public:
  struct TokenTime
  {
    nsCString token;
    PRTime expirationTime;
  };

  struct CacheEntry : public PRCList
  {
    CacheEntry(nsCString& aKey)
      : mKey(aKey)
    {
      MOZ_COUNT_CTOR(nsAccessControlLRUCache::CacheEntry);
    }
    
    ~CacheEntry()
    {
      MOZ_COUNT_DTOR(nsAccessControlLRUCache::CacheEntry);
    }

    void PurgeExpired(PRTime now);
    PRBool CheckRequest(const nsCString& aMethod,
                        const nsTArray<nsCString>& aCustomHeaders);

    nsCString mKey;
    nsTArray<TokenTime> mMethods;
    nsTArray<TokenTime> mHeaders;
  };

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

  CacheEntry* GetEntry(nsIURI* aURI, nsIPrincipal* aPrincipal,
                       PRBool aWithCredentials, PRBool aCreate);
  void RemoveEntries(nsIURI* aURI, nsIPrincipal* aPrincipal);

  void Clear();

private:
  static PLDHashOperator
    RemoveExpiredEntries(const nsACString& aKey, nsAutoPtr<CacheEntry>& aValue,
                         void* aUserData);

  static PRBool GetCacheKey(nsIURI* aURI, nsIPrincipal* aPrincipal,
                            PRBool aWithCredentials, nsACString& _retval);

  nsClassHashtable<nsCStringHashKey, CacheEntry> mTable;
  PRCList mList;
};

class nsXHREventTarget : public nsDOMEventTargetHelper,
                         public nsIXMLHttpRequestEventTarget,
                         public nsWrapperCache
{
public:
  virtual ~nsXHREventTarget();
  NS_DECL_ISUPPORTS_INHERITED

  class NS_CYCLE_COLLECTION_INNERCLASS
    : public NS_CYCLE_COLLECTION_CLASSNAME(nsDOMEventTargetHelper)
  {
    NS_IMETHOD RootAndUnlinkJSObjects(void *p);
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY(nsXHREventTarget,
                                                  nsDOMEventTargetHelper)
    NS_IMETHOD_(void) Trace(void *p, TraceCallback cb, void *closure);
  };
  NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE

  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)
  NS_FORWARD_NSIDOMNSEVENTTARGET(nsDOMEventTargetHelper::)

  void GetParentObject(nsIScriptGlobalObject **aParentObject)
  {
    if (mOwner) {
      CallQueryInterface(mOwner, aParentObject);
    }
    else {
      *aParentObject = nsnull;
    }
  }

  static nsXHREventTarget* FromSupports(nsISupports* aSupports)
  {
    nsPIDOMEventTarget* target =
      static_cast<nsPIDOMEventTarget*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsPIDOMEventTarget> target_qi =
        do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(target_qi == target, "Uh, fix QI!");
    }
#endif

    return static_cast<nsXHREventTarget*>(target);
  }

protected:
  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadStartListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnProgressListener;
};

class nsXMLHttpRequestUpload : public nsXHREventTarget,
                               public nsIXMLHttpRequestUpload
{
public:
  nsXMLHttpRequestUpload(nsPIDOMWindow* aOwner,
                         nsIScriptContext* aScriptContext)
  {
    mOwner = aOwner;
    mScriptContext = aScriptContext;
  }
  virtual ~nsXMLHttpRequestUpload();
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMNSEVENTTARGET(nsXHREventTarget::)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD

  PRBool HasListeners()
  {
    return mListenerManager && mListenerManager->HasListeners();
  }
};

class nsXMLHttpRequest : public nsXHREventTarget,
                         public nsIXMLHttpRequest,
                         public nsIJSXMLHttpRequest,
                         public nsIDOMLoadListener,
                         public nsIStreamListener,
                         public nsIChannelEventSink,
                         public nsIProgressEventSink,
                         public nsIInterfaceRequestor,
                         public nsSupportsWeakReference,
                         public nsIJSNativeInitializer,
                         public nsITimerCallback
{
public:
  nsXMLHttpRequest();
  virtual ~nsXMLHttpRequest();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIXMLHTTPREQUEST

  
  NS_IMETHOD GetOnuploadprogress(nsIDOMEventListener** aOnuploadprogress);
  NS_IMETHOD SetOnuploadprogress(nsIDOMEventListener* aOnuploadprogress);

  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)

  
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

  
  NS_DECL_NSITIMERCALLBACK

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                       PRUint32 argc, jsval* argv);

  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMNSEVENTTARGET(nsXHREventTarget::)

  
  
  static nsresult CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent);
  
  
  
  
  void DispatchProgressEvent(nsPIDOMEventTarget* aTarget,
                             const nsAString& aType,
                             
                             
                             PRBool aUseLSEventWrapper,
                             PRBool aLengthComputable,
                             
                             PRUint64 aLoaded, PRUint64 aTotal,
                             
                             PRUint64 aPosition, PRUint64 aTotalSize);
  void DispatchProgressEvent(nsPIDOMEventTarget* aTarget,
                             const nsAString& aType,
                             PRBool aLengthComputable,
                             PRUint64 aLoaded, PRUint64 aTotal)
  {
    DispatchProgressEvent(aTarget, aType, PR_FALSE,
                          aLengthComputable, aLoaded, aTotal,
                          aLoaded, aLengthComputable ? aTotal : LL_MAXUINT);
  }

  
  nsresult Init();

  void SetRequestObserver(nsIRequestObserver* aObserver);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXMLHttpRequest,
                                           nsXHREventTarget)

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
    delete sAccessControlCache;
    sAccessControlCache = nsnull;
  }

  PRBool AllowUploadProgress();

  static nsAccessControlLRUCache* sAccessControlCache;

protected:
  friend class nsMultipartProxyListener;

  nsresult DetectCharset(nsACString& aCharset);
  nsresult ConvertBodyToText(nsAString& aOutBuffer);
  static NS_METHOD StreamReaderFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                PRUint32 toOffset,
                PRUint32 count,
                PRUint32 *writeCount);
  
  
  nsresult ChangeState(PRUint32 aState, PRBool aBroadcast = PR_TRUE);
  nsresult RequestCompleted();
  nsresult GetLoadGroup(nsILoadGroup **aLoadGroup);
  nsIURI *GetBaseURI();

  nsresult RemoveAddEventListener(const nsAString& aType,
                                  nsRefPtr<nsDOMEventListenerWrapper>& aCurrent,
                                  nsIDOMEventListener* aNew);

  nsresult GetInnerEventListener(nsRefPtr<nsDOMEventListenerWrapper>& aWrapper,
                                 nsIDOMEventListener** aListener);

  already_AddRefed<nsIHttpChannel> GetCurrentHttpChannel();

  





  nsresult CheckChannelForCrossSiteRequest(nsIChannel* aChannel);

  void StartProgressEventTimer();

  nsCOMPtr<nsISupports> mContext;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIChannel> mChannel;
  
  nsCOMPtr<nsIRequest> mReadRequest;
  nsCOMPtr<nsIDOMDocument> mResponseXML;
  nsCOMPtr<nsIChannel> mACGetChannel;
  nsTArray<nsCString> mACUnsafeHeaders;

  nsRefPtr<nsDOMEventListenerWrapper> mOnUploadProgressListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnReadystatechangeListener;

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

  nsIRequestObserver* mRequestObserver;

  nsCOMPtr<nsIURI> mBaseURI;

  PRUint32 mState;

  nsRefPtr<nsXMLHttpRequestUpload> mUpload;
  PRUint64 mUploadTransferred;
  PRUint64 mUploadTotal;
  PRPackedBool mUploadComplete;
  PRUint64 mUploadProgress; 
  PRUint64 mUploadProgressMax; 

  PRPackedBool mErrorLoad;

  PRPackedBool mTimerIsActive;
  PRPackedBool mProgressEventWasDelayed;
  PRPackedBool mLoadLengthComputable;
  PRUint64 mLoadTotal; 
  nsCOMPtr<nsITimer> mProgressNotifier;

  PRPackedBool mFirstStartRequestSeen;
};



class nsXMLHttpProgressEvent : public nsIDOMProgressEvent,
                               public nsIDOMLSProgressEvent,
                               public nsIDOMNSEvent,
                               public nsIPrivateDOMEvent
{
public:
  nsXMLHttpProgressEvent(nsIDOMProgressEvent* aInner,
                         PRUint64 aCurrentProgress,
                         PRUint64 aMaxProgress);
  virtual ~nsXMLHttpProgressEvent();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXMLHttpProgressEvent, nsIDOMNSEvent)
  NS_FORWARD_NSIDOMEVENT(mInner->)
  NS_FORWARD_NSIDOMNSEVENT(mInner->)
  NS_FORWARD_NSIDOMPROGRESSEVENT(mInner->)
  NS_DECL_NSIDOMLSPROGRESSEVENT
  
  NS_IMETHOD DuplicatePrivateData()
  {
    return mInner->DuplicatePrivateData();
  }
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget)
  {
    return mInner->SetTarget(aTarget);
  }
  NS_IMETHOD_(PRBool) IsDispatchStopped()
  {
    return mInner->IsDispatchStopped();
  }
  NS_IMETHOD_(nsEvent*) GetInternalNSEvent()
  {
    return mInner->GetInternalNSEvent();
  }
  NS_IMETHOD SetTrusted(PRBool aTrusted)
  {
    return mInner->SetTrusted(aTrusted);
  }
  virtual void Serialize(IPC::Message* aMsg,
                         PRBool aSerializeInterfaceType)
  {
    mInner->Serialize(aMsg, aSerializeInterfaceType);
  }
  virtual PRBool Deserialize(const IPC::Message* aMsg, void** aIter)
  {
    return mInner->Deserialize(aMsg, aIter);
  }

protected:
  
  
  nsRefPtr<nsDOMProgressEvent> mInner;
  PRUint64 mCurProgress;
  PRUint64 mMaxProgress;
};

#endif
