





#ifndef imgLoader_h__
#define imgLoader_h__

#include "mozilla/Attributes.h"

#include "imgILoader.h"
#include "imgICache.h"
#include "nsWeakReference.h"
#include "nsIContentSniffer.h"
#include "nsRefPtrHashtable.h"
#include "nsExpirationTracker.h"
#include "nsAutoPtr.h"
#include "imgRequest.h"
#include "nsIObserverService.h"
#include "nsIChannelPolicy.h"
#include "nsIProgressEventSink.h"
#include "nsIChannel.h"

class imgLoader;
class imgRequest;
class imgRequestProxy;
class imgIRequest;
class imgINotificationObserver;
class nsILoadGroup;
class imgCacheExpirationTracker;
class imgMemoryReporter;

class imgCacheEntry
{
public:
  imgCacheEntry(imgLoader* loader, imgRequest *request, bool aForcePrincipalCheck);
  ~imgCacheEntry();

  nsrefcnt AddRef()
  {
    NS_PRECONDITION(int32_t(mRefCnt) >= 0, "illegal refcnt");
    NS_ABORT_IF_FALSE(_mOwningThread.GetThread() == PR_GetCurrentThread(), "imgCacheEntry addref isn't thread-safe!");
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "imgCacheEntry", sizeof(*this));
    return mRefCnt;
  }
 
  nsrefcnt Release()
  {
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    NS_ABORT_IF_FALSE(_mOwningThread.GetThread() == PR_GetCurrentThread(), "imgCacheEntry release isn't thread-safe!");
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "imgCacheEntry");
    if (mRefCnt == 0) {
      mRefCnt = 1; 
      delete this;
      return 0;
    }
    return mRefCnt;                              
  }

  uint32_t GetDataSize() const
  {
    return mDataSize;
  }
  void SetDataSize(uint32_t aDataSize)
  {
    int32_t oldsize = mDataSize;
    mDataSize = aDataSize;
    UpdateCache(mDataSize - oldsize);
  }

  int32_t GetTouchedTime() const
  {
    return mTouchedTime;
  }
  void SetTouchedTime(int32_t time)
  {
    mTouchedTime = time;
    Touch( false);
  }

  int32_t GetExpiryTime() const
  {
    return mExpiryTime;
  }
  void SetExpiryTime(int32_t aExpiryTime)
  {
    mExpiryTime = aExpiryTime;
    Touch();
  }

  bool GetMustValidate() const
  {
    return mMustValidate;
  }
  void SetMustValidate(bool aValidate)
  {
    mMustValidate = aValidate;
    Touch();
  }

  already_AddRefed<imgRequest> GetRequest() const
  {
    imgRequest *req = mRequest;
    NS_ADDREF(req);
    return req;
  }

  bool Evicted() const
  {
    return mEvicted;
  }

  nsExpirationState *GetExpirationState()
  {
    return &mExpirationState;
  }

  bool HasNoProxies() const
  {
    return mHasNoProxies;
  }

  bool ForcePrincipalCheck() const
  {
    return mForcePrincipalCheck;
  }

  imgLoader* Loader() const
  {
    return mLoader;
  }

private: 
  friend class imgLoader;
  friend class imgCacheQueue;
  void Touch(bool updateTime = true);
  void UpdateCache(int32_t diff = 0);
  void SetEvicted(bool evict)
  {
    mEvicted = evict;
  }
  void SetHasNoProxies(bool hasNoProxies);

  
  imgCacheEntry(const imgCacheEntry &);

private: 
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  imgLoader* mLoader;
  nsRefPtr<imgRequest> mRequest;
  uint32_t mDataSize;
  int32_t mTouchedTime;
  int32_t mExpiryTime;
  nsExpirationState mExpirationState;
  bool mMustValidate : 1;
  bool mEvicted : 1;
  bool mHasNoProxies : 1;
  bool mForcePrincipalCheck : 1;
};

#include <vector>

#define NS_IMGLOADER_CID \
{ /* 9f6a0d2e-1dd1-11b2-a5b8-951f13c846f7 */         \
     0x9f6a0d2e,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0xa5, 0xb8, 0x95, 0x1f, 0x13, 0xc8, 0x46, 0xf7} \
}

class imgCacheQueue
{
public: 
  imgCacheQueue();
  void Remove(imgCacheEntry *);
  void Push(imgCacheEntry *);
  void MarkDirty();
  bool IsDirty();
  already_AddRefed<imgCacheEntry> Pop();
  void Refresh();
  uint32_t GetSize() const;
  void UpdateSize(int32_t diff);
  uint32_t GetNumElements() const;
  typedef std::vector<nsRefPtr<imgCacheEntry> > queueContainer;  
  typedef queueContainer::iterator iterator;
  typedef queueContainer::const_iterator const_iterator;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

private:
  queueContainer mQueue;
  bool mDirty;
  uint32_t mSize;
};

class imgMemoryReporter;

class imgLoader : public imgILoader,
                  public nsIContentSniffer,
                  public imgICache,
                  public nsSupportsWeakReference,
                  public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGILOADER
  NS_DECL_NSICONTENTSNIFFER
  NS_DECL_IMGICACHE
  NS_DECL_NSIOBSERVER

  imgLoader();
  virtual ~imgLoader();

  nsresult Init();

  static imgLoader* Create()
  {
      
      
      
      
      
      imgILoader *loader;
      CallCreateInstance("@mozilla.org/image/loader;1", &loader);
      
      
      return static_cast<imgLoader*>(loader);
  }

  static already_AddRefed<imgLoader> GetInstance();

  nsresult LoadImage(nsIURI *aURI,
                     nsIURI *aInitialDocumentURI,
                     nsIURI *aReferrerURI,
                     nsIPrincipal* aLoadingPrincipal,
                     nsILoadGroup *aLoadGroup,
                     imgINotificationObserver *aObserver,
                     nsISupports *aCX,
                     nsLoadFlags aLoadFlags,
                     nsISupports *aCacheKey,
                     nsIChannelPolicy *aPolicy,
                     imgRequestProxy **_retval);
  nsresult LoadImageWithChannel(nsIChannel *channel,
                                imgINotificationObserver *aObserver,
                                nsISupports *aCX,
                                nsIStreamListener **listener,
                                imgRequestProxy **_retval);

  static nsresult GetMimeTypeFromContent(const char* aContents, uint32_t aLength, nsACString& aContentType);
  
  static NS_EXPORT_(bool) SupportImageWithMimeType(const char* aMimeType);

  static void GlobalInit(); 
  static void Shutdown(); 

  nsresult ClearChromeImageCache();
  nsresult ClearImageCache();
  void MinimizeCaches();

  nsresult InitCache();

  bool RemoveFromCache(nsIURI *aKey);
  bool RemoveFromCache(imgCacheEntry *entry);

  bool PutIntoCache(nsIURI *key, imgCacheEntry *entry);

  
  
  
  inline static bool CompareCacheEntries(const nsRefPtr<imgCacheEntry> &one,
                                         const nsRefPtr<imgCacheEntry> &two)
  {
    if (!one)
      return false;
    if (!two)
      return true;

    const double sizeweight = 1.0 - sCacheTimeWeight;

    
    
    
    double oneweight = double(one->GetDataSize()) * sizeweight -
                       double(one->GetTouchedTime()) * sCacheTimeWeight;
    double twoweight = double(two->GetDataSize()) * sizeweight -
                       double(two->GetTouchedTime()) * sCacheTimeWeight;

    return oneweight < twoweight;
  }

  void VerifyCacheSizes();

  
  
  
  
  
  
  
  
  
  
  
  bool SetHasNoProxies(nsIURI *key, imgCacheEntry *entry);
  bool SetHasProxies(nsIURI *key);

private: 

  bool ValidateEntry(imgCacheEntry *aEntry, nsIURI *aKey,
                       nsIURI *aInitialDocumentURI, nsIURI *aReferrerURI, 
                       nsILoadGroup *aLoadGroup,
                       imgINotificationObserver *aObserver, nsISupports *aCX,
                       nsLoadFlags aLoadFlags, bool aCanMakeNewChannel,
                       imgRequestProxy **aProxyRequest,
                       nsIChannelPolicy *aPolicy,
                       nsIPrincipal* aLoadingPrincipal,
                       int32_t aCORSMode);

  bool ValidateRequestWithNewChannel(imgRequest *request, nsIURI *aURI,
                                       nsIURI *aInitialDocumentURI,
                                       nsIURI *aReferrerURI,
                                       nsILoadGroup *aLoadGroup,
                                       imgINotificationObserver *aObserver,
                                       nsISupports *aCX, nsLoadFlags aLoadFlags,
                                       imgRequestProxy **aProxyRequest,
                                       nsIChannelPolicy *aPolicy,
                                       nsIPrincipal* aLoadingPrincipal,
                                       int32_t aCORSMode);

  nsresult CreateNewProxyForRequest(imgRequest *aRequest, nsILoadGroup *aLoadGroup,
                                    imgINotificationObserver *aObserver,
                                    nsLoadFlags aLoadFlags, imgRequestProxy **_retval);

  void ReadAcceptHeaderPref();


  typedef nsRefPtrHashtable<nsCStringHashKey, imgCacheEntry> imgCacheTable;

  nsresult EvictEntries(imgCacheTable &aCacheToClear);
  nsresult EvictEntries(imgCacheQueue &aQueueToClear);

  imgCacheTable &GetCache(nsIURI *aURI);
  imgCacheQueue &GetCacheQueue(nsIURI *aURI);
  void CacheEntriesChanged(nsIURI *aURI, int32_t sizediff = 0);
  void CheckCacheLimits(imgCacheTable &cache, imgCacheQueue &queue);

private: 
  friend class imgCacheEntry;
  friend class imgMemoryReporter;

  imgCacheTable mCache;
  imgCacheQueue mCacheQueue;

  imgCacheTable mChromeCache;
  imgCacheQueue mChromeCacheQueue;

  static double sCacheTimeWeight;
  static uint32_t sCacheMaxSize;
  static imgMemoryReporter* sMemReporter;

  nsCString mAcceptHeader;

  nsAutoPtr<imgCacheExpirationTracker> mCacheTracker;
  bool mRespectPrivacy;
};







#include "nsCOMPtr.h"
#include "nsIStreamListener.h"

class ProxyListener : public nsIStreamListener
{
public:
  ProxyListener(nsIStreamListener *dest);
  virtual ~ProxyListener();

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;
};







class nsProgressNotificationProxy MOZ_FINAL
  : public nsIProgressEventSink
  , public nsIChannelEventSink
  , public nsIInterfaceRequestor
{
  public:
    nsProgressNotificationProxy(nsIChannel* channel,
                                imgIRequest* proxy)
        : mImageRequest(proxy) {
      channel->GetNotificationCallbacks(getter_AddRefs(mOriginalCallbacks));
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROGRESSEVENTSINK
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR
  private:
    ~nsProgressNotificationProxy() {}

    nsCOMPtr<nsIInterfaceRequestor> mOriginalCallbacks;
    nsCOMPtr<nsIRequest> mImageRequest;
};





#include "nsCOMArray.h"

class imgCacheValidator : public nsIStreamListener,
                          public nsIChannelEventSink,
                          public nsIInterfaceRequestor,
                          public nsIAsyncVerifyRedirectCallback
{
public:
  imgCacheValidator(nsProgressNotificationProxy* progress, imgLoader* loader,
                    imgRequest *request, void *aContext, bool forcePrincipalCheckForCacheEntry);
  virtual ~imgCacheValidator();

  void AddProxy(imgRequestProxy *aProxy);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

private:
  nsCOMPtr<nsIStreamListener> mDestListener;
  nsRefPtr<nsProgressNotificationProxy> mProgressProxy;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mRedirectChannel;

  nsRefPtr<imgRequest> mRequest;
  nsCOMArray<imgIRequest> mProxies;

  nsRefPtr<imgRequest> mNewRequest;
  nsRefPtr<imgCacheEntry> mNewEntry;

  void *mContext;

  imgLoader* mImgLoader;
};

#endif  
