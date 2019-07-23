







































#include "imgILoader.h"
#include "imgICache.h"
#include "nsWeakReference.h"
#include "nsIContentSniffer.h"
#include "nsRefPtrHashtable.h"
#include "nsExpirationTracker.h"
#include "nsAutoPtr.h"
#include "prtypes.h"
#include "imgRequest.h"
#include "nsIObserverService.h"

#ifdef LOADER_THREADSAFE
#include "prlock.h"
#endif

class imgRequest;
class imgRequestProxy;
class imgIRequest;
class imgIDecoderObserver;
class nsILoadGroup;
class nsIPrefBranch;

class imgCacheEntry
{
public:
  imgCacheEntry(imgRequest *request, PRBool mustValidateIfExpired = PR_FALSE);
  ~imgCacheEntry();

  nsrefcnt AddRef()
  {
    NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
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

  PRUint32 GetDataSize() const
  {
    return mDataSize;
  }
  void SetDataSize(PRUint32 aDataSize)
  {
    PRInt32 oldsize = mDataSize;
    mDataSize = aDataSize;
    UpdateCache(mDataSize - oldsize);
  }

  PRInt32 GetTouchedTime() const
  {
    return mTouchedTime;
  }
  void SetTouchedTime(PRInt32 time)
  {
    mTouchedTime = time;
    Touch( PR_FALSE);
  }

  PRInt32 GetExpiryTime() const
  {
    return mExpiryTime;
  }
  void SetExpiryTime(PRInt32 aExpiryTime)
  {
    mExpiryTime = aExpiryTime;
    Touch();
  }

  PRBool GetMustValidateIfExpired() const
  {
    return mMustValidateIfExpired;
  }
  void SetMustValidateIfExpired(PRBool aValidate)
  {
    mMustValidateIfExpired = aValidate;
    Touch();
  }

  already_AddRefed<imgRequest> GetRequest() const
  {
    imgRequest *req = mRequest;
    NS_ADDREF(req);
    return req;
  }

  PRBool Evicted() const
  {
    return mEvicted;
  }

  nsExpirationState *GetExpirationState()
  {
    return &mExpirationState;
  }

  PRBool HasNoProxies() const
  {
    return mHasNoProxies;
  }

private: 
  friend class imgLoader;
  friend class imgCacheQueue;
  void Touch(PRBool updateTime = PR_TRUE);
  void UpdateCache(PRInt32 diff = 0);
  void SetEvicted(PRBool evict)
  {
    mEvicted = evict;
  }
  void SetHasNoProxies(PRBool hasNoProxies);

  
  imgCacheEntry(const imgCacheEntry &);

private: 
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  nsRefPtr<imgRequest> mRequest;
  PRUint32 mDataSize;
  PRInt32 mTouchedTime;
  PRInt32 mExpiryTime;
  nsExpirationState mExpirationState;
  PRPackedBool mMustValidateIfExpired : 1;
  PRPackedBool mEvicted : 1;
  PRPackedBool mHasNoProxies : 1;
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
  PRBool IsDirty();
  already_AddRefed<imgCacheEntry> Pop();
  void Refresh();
  PRUint32 GetSize() const;
  void UpdateSize(PRInt32 diff);
  PRUint32 GetNumElements() const;
  typedef std::vector<nsRefPtr<imgCacheEntry> > queueContainer;  
  typedef queueContainer::iterator iterator;
  typedef queueContainer::const_iterator const_iterator;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

private:
  queueContainer mQueue;
  PRBool mDirty;
  PRUint32 mSize;
};

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

  static nsresult GetMimeTypeFromContent(const char* aContents, PRUint32 aLength, nsACString& aContentType);

  static void Shutdown(); 

  static nsresult ClearChromeImageCache();
  static nsresult ClearImageCache();
  static void MinimizeCaches();

  static nsresult InitCache();

  static PRBool RemoveFromCache(nsIURI *aKey);
  static PRBool RemoveFromCache(imgCacheEntry *entry);

  static PRBool PutIntoCache(nsIURI *key, imgCacheEntry *entry);

  
  
  
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

  static void VerifyCacheSizes();

  
  
  
  
  
  
  
  
  
  
  
  static PRBool SetHasNoProxies(nsIURI *key, imgCacheEntry *entry);
  static PRBool SetHasProxies(nsIURI *key);

private: 


  PRBool ValidateEntry(imgCacheEntry *aEntry, nsIURI *aKey,
                       nsIURI *aInitialDocumentURI, nsIURI *aReferrerURI, 
                       nsILoadGroup *aLoadGroup,
                       imgIDecoderObserver *aObserver, nsISupports *aCX,
                       nsLoadFlags aLoadFlags, PRBool aCanMakeNewChannel,
                       imgIRequest *aExistingRequest,
                       imgIRequest **aProxyRequest);
  PRBool ValidateRequestWithNewChannel(imgRequest *request, nsIURI *aURI,
                                       nsIURI *aInitialDocumentURI,
                                       nsIURI *aReferrerURI,
                                       nsILoadGroup *aLoadGroup,
                                       imgIDecoderObserver *aObserver,
                                       nsISupports *aCX, nsLoadFlags aLoadFlags,
                                       imgIRequest *aExistingRequest,
                                       imgIRequest **aProxyRequest);

  nsresult CreateNewProxyForRequest(imgRequest *aRequest, nsILoadGroup *aLoadGroup,
                                    imgIDecoderObserver *aObserver,
                                    nsLoadFlags aLoadFlags, imgIRequest *aRequestProxy,
                                    imgIRequest **_retval);

  void ReadAcceptHeaderPref(nsIPrefBranch *aBranch);


  typedef nsRefPtrHashtable<nsCStringHashKey, imgCacheEntry> imgCacheTable;

  static nsresult EvictEntries(imgCacheTable &aCacheToClear);
  static nsresult EvictEntries(imgCacheQueue &aQueueToClear);

  static imgCacheTable &GetCache(nsIURI *aURI);
  static imgCacheQueue &GetCacheQueue(nsIURI *aURI);
  static void CacheEntriesChanged(nsIURI *aURI, PRInt32 sizediff = 0);
  static void CheckCacheLimits(imgCacheTable &cache, imgCacheQueue &queue);

private: 
  friend class imgCacheEntry;

  static imgCacheTable sCache;
  static imgCacheQueue sCacheQueue;

  static imgCacheTable sChromeCache;
  static imgCacheQueue sChromeCacheQueue;
  static PRFloat64 sCacheTimeWeight;
  static PRUint32 sCacheMaxSize;

  nsCString mAcceptHeader;
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






#include "nsCOMArray.h"

class imgCacheValidator : public nsIStreamListener
{
public:
  imgCacheValidator(imgRequest *request, void *aContext);
  virtual ~imgCacheValidator();

  void AddProxy(imgRequestProxy *aProxy);

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;

  nsRefPtr<imgRequest> mRequest;
  nsCOMArray<imgIRequest> mProxies;

  void *mContext;

  static imgLoader sImgLoader;
};
