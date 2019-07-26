




#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"

#include "ImageLogging.h"
#include "imgLoader.h"
#include "imgRequestProxy.h"

#include "RasterImage.h"






#undef LoadImage

#include "nsCOMPtr.h"

#include "nsContentUtils.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsIHttpChannel.h"
#include "nsICachingChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIServiceManager.h"
#include "nsIFileURL.h"
#include "nsThreadUtils.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsIDocument.h"
#include "nsPIDOMWindow.h"

#include "netCore.h"

#include "nsURILoader.h"

#include "nsIComponentRegistrar.h"

#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"

#include "nsIMemoryReporter.h"




#include "nsIHttpChannelInternal.h"  
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsILoadContext.h"

#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::image;

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(ImagesMallocSizeOf, "images")

class imgMemoryReporter MOZ_FINAL :
  public nsIMemoryMultiReporter
{
public:
  imgMemoryReporter()
  {
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetName(nsACString &name)
  {
    name.Assign("images");
    return NS_OK;
  }

  NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback *callback,
                            nsISupports *closure)
  {
    AllSizes chrome;
    AllSizes content;

    for (uint32_t i = 0; i < mKnownLoaders.Length(); i++) {
      mKnownLoaders[i]->mChromeCache.EnumerateRead(EntryAllSizes, &chrome);
      mKnownLoaders[i]->mCache.EnumerateRead(EntryAllSizes, &content);
    }

#define REPORT(_path, _kind, _amount, _desc)                                  \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = callback->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),      \
                              _kind, nsIMemoryReporter::UNITS_BYTES, _amount, \
                              NS_LITERAL_CSTRING(_desc), closure);            \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

    REPORT("explicit/images/chrome/used/raw",
           nsIMemoryReporter::KIND_HEAP, chrome.mUsedRaw,
           "Memory used by in-use chrome images (compressed data).");

    REPORT("explicit/images/chrome/used/uncompressed-heap",
           nsIMemoryReporter::KIND_HEAP, chrome.mUsedUncompressedHeap,
           "Memory used by in-use chrome images (uncompressed data).");

    REPORT("explicit/images/chrome/used/uncompressed-nonheap",
           nsIMemoryReporter::KIND_NONHEAP, chrome.mUsedUncompressedNonheap,
           "Memory used by in-use chrome images (uncompressed data).");

    REPORT("explicit/images/chrome/unused/raw",
           nsIMemoryReporter::KIND_HEAP, chrome.mUnusedRaw,
           "Memory used by not in-use chrome images (compressed data).");

    REPORT("explicit/images/chrome/unused/uncompressed-heap",
           nsIMemoryReporter::KIND_HEAP, chrome.mUnusedUncompressedHeap,
           "Memory used by not in-use chrome images (uncompressed data).");

    REPORT("explicit/images/chrome/unused/uncompressed-nonheap",
           nsIMemoryReporter::KIND_NONHEAP, chrome.mUnusedUncompressedNonheap,
           "Memory used by not in-use chrome images (uncompressed data).");

    REPORT("explicit/images/content/used/raw",
           nsIMemoryReporter::KIND_HEAP, content.mUsedRaw,
           "Memory used by in-use content images (compressed data).");

    REPORT("explicit/images/content/used/uncompressed-heap",
           nsIMemoryReporter::KIND_HEAP, content.mUsedUncompressedHeap,
           "Memory used by in-use content images (uncompressed data).");

    REPORT("explicit/images/content/used/uncompressed-nonheap",
           nsIMemoryReporter::KIND_NONHEAP, content.mUsedUncompressedNonheap,
           "Memory used by in-use content images (uncompressed data).");

    REPORT("explicit/images/content/unused/raw",
           nsIMemoryReporter::KIND_HEAP, content.mUnusedRaw,
           "Memory used by not in-use content images (compressed data).");

    REPORT("explicit/images/content/unused/uncompressed-heap",
           nsIMemoryReporter::KIND_HEAP, content.mUnusedUncompressedHeap,
           "Memory used by not in-use content images (uncompressed data).");

    REPORT("explicit/images/content/unused/uncompressed-nonheap",
           nsIMemoryReporter::KIND_NONHEAP, content.mUnusedUncompressedNonheap,
           "Memory used by not in-use content images (uncompressed data).");

#undef REPORT

    return NS_OK;
  }

  NS_IMETHOD GetExplicitNonHeap(int64_t *n)
  {
    size_t n2 = 0;
    for (uint32_t i = 0; i < mKnownLoaders.Length(); i++) {
      mKnownLoaders[i]->mChromeCache.EnumerateRead(EntryExplicitNonHeapSize, &n2);
      mKnownLoaders[i]->mCache.EnumerateRead(EntryExplicitNonHeapSize, &n2);
    }
    *n = n2;
    return NS_OK;
  }

  static int64_t GetImagesContentUsedUncompressed()
  {
    size_t n = 0;
    for (uint32_t i = 0; i < imgLoader::sMemReporter->mKnownLoaders.Length(); i++) {
      imgLoader::sMemReporter->mKnownLoaders[i]->mCache.EnumerateRead(EntryUsedUncompressedSize, &n);
    }
    return n;
  }

  void RegisterLoader(imgLoader* aLoader)
  {
    mKnownLoaders.AppendElement(aLoader);
  }

  void UnregisterLoader(imgLoader* aLoader)
  {
    mKnownLoaders.RemoveElement(aLoader);
  }

private:
  nsTArray<imgLoader*> mKnownLoaders;

  struct AllSizes {
    size_t mUsedRaw;
    size_t mUsedUncompressedHeap;
    size_t mUsedUncompressedNonheap;
    size_t mUnusedRaw;
    size_t mUnusedUncompressedHeap;
    size_t mUnusedUncompressedNonheap;

    AllSizes() {
      memset(this, 0, sizeof(*this));
    }
  };

  static PLDHashOperator EntryAllSizes(const nsACString&,
                                       imgCacheEntry *entry,
                                       void *userArg)
  {
    nsRefPtr<imgRequest> req = entry->GetRequest();
    Image *image = static_cast<Image*>(req->mImage.get());
    if (image) {
      AllSizes *sizes = static_cast<AllSizes*>(userArg);
      if (entry->HasNoProxies()) {
        sizes->mUnusedRaw +=
          image->HeapSizeOfSourceWithComputedFallback(ImagesMallocSizeOf);
        sizes->mUnusedUncompressedHeap +=
          image->HeapSizeOfDecodedWithComputedFallback(ImagesMallocSizeOf);
        sizes->mUnusedUncompressedNonheap += image->NonHeapSizeOfDecoded();
      } else {
        sizes->mUsedRaw +=
          image->HeapSizeOfSourceWithComputedFallback(ImagesMallocSizeOf);
        sizes->mUsedUncompressedHeap +=
          image->HeapSizeOfDecodedWithComputedFallback(ImagesMallocSizeOf);
        sizes->mUsedUncompressedNonheap += image->NonHeapSizeOfDecoded();
      }
    }

    return PL_DHASH_NEXT;
  }

  static PLDHashOperator EntryExplicitNonHeapSize(const nsACString&,
                                                  imgCacheEntry *entry,
                                                  void *userArg)
  {
    size_t *n = static_cast<size_t*>(userArg);
    nsRefPtr<imgRequest> req = entry->GetRequest();
    Image *image = static_cast<Image*>(req->mImage.get());
    if (image) {
      *n += image->NonHeapSizeOfDecoded();
    }

    return PL_DHASH_NEXT;
  }

  static PLDHashOperator EntryUsedUncompressedSize(const nsACString&,
                                                   imgCacheEntry *entry,
                                                   void *userArg)
  {
    if (!entry->HasNoProxies()) {
      size_t *n = static_cast<size_t*>(userArg);
      nsRefPtr<imgRequest> req = entry->GetRequest();
      Image *image = static_cast<Image*>(req->mImage.get());
      if (image) {
        *n += image->HeapSizeOfDecodedWithComputedFallback(ImagesMallocSizeOf);
        *n += image->NonHeapSizeOfDecoded();
      }
    }

    return PL_DHASH_NEXT;
  }
};


NS_MEMORY_REPORTER_IMPLEMENT(
  ImagesContentUsedUncompressed,
  "images-content-used-uncompressed",
  KIND_OTHER,
  UNITS_BYTES,
  imgMemoryReporter::GetImagesContentUsedUncompressed,
  "This is the sum of the 'explicit/images/content/used/uncompressed-heap' "
  "and 'explicit/images/content/used/uncompressed-nonheap' numbers.  However, "
  "it is measured at a different time and so may give slightly different "
  "results.")

NS_IMPL_ISUPPORTS1(imgMemoryReporter, nsIMemoryMultiReporter)

NS_IMPL_ISUPPORTS3(nsProgressNotificationProxy,
                     nsIProgressEventSink,
                     nsIChannelEventSink,
                     nsIInterfaceRequestor)

NS_IMETHODIMP
nsProgressNotificationProxy::OnProgress(nsIRequest* request,
                                        nsISupports* ctxt,
                                        uint64_t progress,
                                        uint64_t progressMax)
{
  nsCOMPtr<nsILoadGroup> loadGroup;
  request->GetLoadGroup(getter_AddRefs(loadGroup));

  nsCOMPtr<nsIProgressEventSink> target;
  NS_QueryNotificationCallbacks(mOriginalCallbacks,
                                loadGroup,
                                NS_GET_IID(nsIProgressEventSink),
                                getter_AddRefs(target));
  if (!target)
    return NS_OK;
  return target->OnProgress(mImageRequest, ctxt, progress, progressMax);
}

NS_IMETHODIMP
nsProgressNotificationProxy::OnStatus(nsIRequest* request,
                                      nsISupports* ctxt,
                                      nsresult status,
                                      const PRUnichar* statusArg)
{
  nsCOMPtr<nsILoadGroup> loadGroup;
  request->GetLoadGroup(getter_AddRefs(loadGroup));

  nsCOMPtr<nsIProgressEventSink> target;
  NS_QueryNotificationCallbacks(mOriginalCallbacks,
                                loadGroup,
                                NS_GET_IID(nsIProgressEventSink),
                                getter_AddRefs(target));
  if (!target)
    return NS_OK;
  return target->OnStatus(mImageRequest, ctxt, status, statusArg);
}

NS_IMETHODIMP
nsProgressNotificationProxy::AsyncOnChannelRedirect(nsIChannel *oldChannel,
                                                    nsIChannel *newChannel,
                                                    uint32_t flags,
                                                    nsIAsyncVerifyRedirectCallback *cb)
{
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  newChannel->GetLoadGroup(getter_AddRefs(loadGroup));
  nsCOMPtr<nsIChannelEventSink> target;
  NS_QueryNotificationCallbacks(mOriginalCallbacks,
                                loadGroup,
                                NS_GET_IID(nsIChannelEventSink),
                                getter_AddRefs(target));
  if (!target) {
      cb->OnRedirectVerifyCallback(NS_OK);
      return NS_OK;
  }

  
  return target->AsyncOnChannelRedirect(oldChannel, newChannel, flags, cb);
}

NS_IMETHODIMP
nsProgressNotificationProxy::GetInterface(const nsIID& iid,
                                          void** result)
{
  if (iid.Equals(NS_GET_IID(nsIProgressEventSink))) {
    *result = static_cast<nsIProgressEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (iid.Equals(NS_GET_IID(nsIChannelEventSink))) {
    *result = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (mOriginalCallbacks)
    return mOriginalCallbacks->GetInterface(iid, result);
  return NS_NOINTERFACE;
}

static void NewRequestAndEntry(bool aForcePrincipalCheckForCacheEntry, imgLoader* aLoader,
                               imgRequest **aRequest, imgCacheEntry **aEntry)
{
  nsRefPtr<imgRequest> request = new imgRequest(aLoader);
  nsRefPtr<imgCacheEntry> entry = new imgCacheEntry(aLoader, request, aForcePrincipalCheckForCacheEntry);
  request.forget(aRequest);
  entry.forget(aEntry);
}

static bool ShouldRevalidateEntry(imgCacheEntry *aEntry,
                              nsLoadFlags aFlags,
                              bool aHasExpired)
{
  bool bValidateEntry = false;

  if (aFlags & nsIRequest::LOAD_BYPASS_CACHE)
    return false;

  if (aFlags & nsIRequest::VALIDATE_ALWAYS) {
    bValidateEntry = true;
  }
  else if (aEntry->GetMustValidate()) {
    bValidateEntry = true;
  }
  
  
  
  
  else if (aHasExpired) {
    
    
    
    
    
    if (aFlags & (nsIRequest::VALIDATE_NEVER | 
                  nsIRequest::VALIDATE_ONCE_PER_SESSION)) 
    {
      bValidateEntry = false;
    }
    
    
    
    
    else if (!(aFlags & nsIRequest::LOAD_FROM_CACHE)) {
      bValidateEntry = true;
    }
  }

  return bValidateEntry;
}




static bool
ValidateCORSAndPrincipal(imgRequest* request, bool forcePrincipalCheck,
                         int32_t corsmode, nsIPrincipal* loadingPrincipal)
{
  
  
  if (request->GetCORSMode() != corsmode) {
    return false;
  } else if (request->GetCORSMode() != imgIRequest::CORS_NONE ||
             forcePrincipalCheck) {
    nsCOMPtr<nsIPrincipal> otherprincipal = request->GetLoadingPrincipal();

    
    
    if (otherprincipal && !loadingPrincipal) {
      return false;
    }

    if (otherprincipal && loadingPrincipal) {
      bool equals = false;
      otherprincipal->Equals(loadingPrincipal, &equals);
      return equals;
    }
  }

  return true;
}

static nsresult NewImageChannel(nsIChannel **aResult,
                                
                                
                                
                                
                                
                                
                                
                                
                                bool *aForcePrincipalCheckForCacheEntry,
                                nsIURI *aURI,
                                nsIURI *aInitialDocumentURI,
                                nsIURI *aReferringURI,
                                nsILoadGroup *aLoadGroup,
                                const nsCString& aAcceptHeader,
                                nsLoadFlags aLoadFlags,
                                nsIChannelPolicy *aPolicy,
                                nsIPrincipal *aLoadingPrincipal)
{
  nsresult rv;
  nsCOMPtr<nsIChannel> newChannel;
  nsCOMPtr<nsIHttpChannel> newHttpChannel;
 
  nsCOMPtr<nsIInterfaceRequestor> callbacks;

  if (aLoadGroup) {
    
    
    
    
    
    
    
    aLoadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
  }

  
  
  
  
  
  
  rv = NS_NewChannel(aResult,
                     aURI,        
                     nullptr,      
                     nullptr,      
                     callbacks,   
                     aLoadFlags,
                     aPolicy);
  if (NS_FAILED(rv))
    return rv;

  *aForcePrincipalCheckForCacheEntry = false;

  
  newHttpChannel = do_QueryInterface(*aResult);
  if (newHttpChannel) {
    newHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                     aAcceptHeader,
                                     false);

    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal = do_QueryInterface(newHttpChannel);
    NS_ENSURE_TRUE(httpChannelInternal, NS_ERROR_UNEXPECTED);
    httpChannelInternal->SetDocumentURI(aInitialDocumentURI);
    newHttpChannel->SetReferrer(aReferringURI);
  }

  
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(*aResult);
  if (p) {
    uint32_t priority = nsISupportsPriority::PRIORITY_LOW;

    if (aLoadFlags & nsIRequest::LOAD_BACKGROUND)
      ++priority; 

    p->AdjustPriority(priority);
  }

  bool setOwner = nsContentUtils::SetUpChannelOwner(aLoadingPrincipal,
                                                      *aResult, aURI, false);
  *aForcePrincipalCheckForCacheEntry = setOwner;

  return NS_OK;
}

static uint32_t SecondsFromPRTime(PRTime prTime)
{
  return uint32_t(int64_t(prTime) / int64_t(PR_USEC_PER_SEC));
}

imgCacheEntry::imgCacheEntry(imgLoader* loader, imgRequest *request, bool forcePrincipalCheck)
 : mLoader(loader),
   mRequest(request),
   mDataSize(0),
   mTouchedTime(SecondsFromPRTime(PR_Now())),
   mExpiryTime(0),
   mMustValidate(false),
   
   
   mEvicted(true),
   mHasNoProxies(true),
   mForcePrincipalCheck(forcePrincipalCheck)
{}

imgCacheEntry::~imgCacheEntry()
{
  LOG_FUNC(gImgLog, "imgCacheEntry::~imgCacheEntry()");
}

void imgCacheEntry::Touch(bool updateTime )
{
  LOG_SCOPE(gImgLog, "imgCacheEntry::Touch");

  if (updateTime)
    mTouchedTime = SecondsFromPRTime(PR_Now());

  UpdateCache();
}

void imgCacheEntry::UpdateCache(int32_t diff )
{
  
  
  if (!Evicted() && HasNoProxies()) {
    nsCOMPtr<nsIURI> uri;
    mRequest->GetURI(getter_AddRefs(uri));
    mLoader->CacheEntriesChanged(uri, diff);
  }
}

void imgCacheEntry::SetHasNoProxies(bool hasNoProxies)
{
#if defined(PR_LOGGING)
  nsCOMPtr<nsIURI> uri;
  mRequest->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  if (uri)
    uri->GetSpec(spec);
  if (hasNoProxies)
    LOG_FUNC_WITH_PARAM(gImgLog, "imgCacheEntry::SetHasNoProxies true", "uri", spec.get());
  else
    LOG_FUNC_WITH_PARAM(gImgLog, "imgCacheEntry::SetHasNoProxies false", "uri", spec.get());
#endif

  mHasNoProxies = hasNoProxies;
}

imgCacheQueue::imgCacheQueue()
 : mDirty(false),
   mSize(0)
{}

void imgCacheQueue::UpdateSize(int32_t diff)
{
  mSize += diff;
}

uint32_t imgCacheQueue::GetSize() const
{
  return mSize;
}

#include <algorithm>
using namespace std;

void imgCacheQueue::Remove(imgCacheEntry *entry)
{
  queueContainer::iterator it = find(mQueue.begin(), mQueue.end(), entry);
  if (it != mQueue.end()) {
    mSize -= (*it)->GetDataSize();
    mQueue.erase(it);
    MarkDirty();
  }
}

void imgCacheQueue::Push(imgCacheEntry *entry)
{
  mSize += entry->GetDataSize();

  nsRefPtr<imgCacheEntry> refptr(entry);
  mQueue.push_back(refptr);
  MarkDirty();
}

already_AddRefed<imgCacheEntry> imgCacheQueue::Pop()
{
  if (mQueue.empty())
    return nullptr;
  if (IsDirty())
    Refresh();

  nsRefPtr<imgCacheEntry> entry = mQueue[0];
  std::pop_heap(mQueue.begin(), mQueue.end(), imgLoader::CompareCacheEntries);
  mQueue.pop_back();

  mSize -= entry->GetDataSize();
  imgCacheEntry *ret = entry;
  NS_ADDREF(ret);
  return ret;
}

void imgCacheQueue::Refresh()
{
  std::make_heap(mQueue.begin(), mQueue.end(), imgLoader::CompareCacheEntries);
  mDirty = false;
}

void imgCacheQueue::MarkDirty()
{
  mDirty = true;
}

bool imgCacheQueue::IsDirty()
{
  return mDirty;
}

uint32_t imgCacheQueue::GetNumElements() const
{
  return mQueue.size();
}

imgCacheQueue::iterator imgCacheQueue::begin()
{
  return mQueue.begin();
}
imgCacheQueue::const_iterator imgCacheQueue::begin() const
{
  return mQueue.begin();
}

imgCacheQueue::iterator imgCacheQueue::end()
{
  return mQueue.end();
}
imgCacheQueue::const_iterator imgCacheQueue::end() const
{
  return mQueue.end();
}

nsresult imgLoader::CreateNewProxyForRequest(imgRequest *aRequest, nsILoadGroup *aLoadGroup,
                                             imgIDecoderObserver *aObserver,
                                             nsLoadFlags aLoadFlags, imgIRequest *aProxyRequest,
                                             imgIRequest **_retval)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgLoader::CreateNewProxyForRequest", "imgRequest", aRequest);

  




  imgRequestProxy *proxyRequest;
  if (aProxyRequest) {
    proxyRequest = static_cast<imgRequestProxy *>(aProxyRequest);
  } else {
    proxyRequest = new imgRequestProxy();
  }
  NS_ADDREF(proxyRequest);

  


  proxyRequest->SetLoadFlags(aLoadFlags);

  nsCOMPtr<nsIURI> uri;
  aRequest->GetURI(getter_AddRefs(uri));

  
  nsresult rv = proxyRequest->Init(aRequest, aLoadGroup, aRequest->mImage, uri, aObserver);
  if (NS_FAILED(rv)) {
    NS_RELEASE(proxyRequest);
    return rv;
  }

  
  *_retval = static_cast<imgIRequest*>(proxyRequest);

  return NS_OK;
}

class imgCacheObserver MOZ_FINAL : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(imgCacheObserver, nsIObserver)

NS_IMETHODIMP
imgCacheObserver::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aSomeData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    DiscardTracker::DiscardAll();
  }
  return NS_OK;
}

class imgCacheExpirationTracker MOZ_FINAL
  : public nsExpirationTracker<imgCacheEntry, 3>
{
  enum { TIMEOUT_SECONDS = 10 };
public:
  imgCacheExpirationTracker();

protected:
  void NotifyExpired(imgCacheEntry *entry);
};

imgCacheExpirationTracker::imgCacheExpirationTracker()
 : nsExpirationTracker<imgCacheEntry, 3>(TIMEOUT_SECONDS * 1000)
{}

void imgCacheExpirationTracker::NotifyExpired(imgCacheEntry *entry)
{
  
  
  nsRefPtr<imgCacheEntry> kungFuDeathGrip(entry);

#if defined(PR_LOGGING)
  nsRefPtr<imgRequest> req(entry->GetRequest());
  if (req) {
    nsCOMPtr<nsIURI> uri;
    req->GetURI(getter_AddRefs(uri));
    nsAutoCString spec;
    uri->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(gImgLog, "imgCacheExpirationTracker::NotifyExpired", "entry", spec.get());
  }
#endif

  
  
  if (!entry->Evicted())
    entry->Loader()->RemoveFromCache(entry);

  entry->Loader()->VerifyCacheSizes();
}

imgCacheObserver *gCacheObserver;

double imgLoader::sCacheTimeWeight;
uint32_t imgLoader::sCacheMaxSize;
imgMemoryReporter* imgLoader::sMemReporter;

NS_IMPL_ISUPPORTS5(imgLoader, imgILoader, nsIContentSniffer, imgICache, nsISupportsWeakReference, nsIObserver)

imgLoader::imgLoader()
: mRespectPrivacy(false)
{
  sMemReporter->AddRef();
  sMemReporter->RegisterLoader(this);
}

imgLoader::~imgLoader()
{
  ClearChromeImageCache();
  ClearImageCache();
  sMemReporter->UnregisterLoader(this);
  sMemReporter->Release();
}

void imgLoader::VerifyCacheSizes()
{
#ifdef DEBUG
  if (!mCacheTracker)
    return;

  uint32_t cachesize = mCache.Count() + mChromeCache.Count();
  uint32_t queuesize = mCacheQueue.GetNumElements() + mChromeCacheQueue.GetNumElements();
  uint32_t trackersize = 0;
  for (nsExpirationTracker<imgCacheEntry, 3>::Iterator it(mCacheTracker); it.Next(); )
    trackersize++;
  NS_ABORT_IF_FALSE(queuesize == trackersize, "Queue and tracker sizes out of sync!");
  NS_ABORT_IF_FALSE(queuesize <= cachesize, "Queue has more elements than cache!");
#endif
}

imgLoader::imgCacheTable & imgLoader::GetCache(nsIURI *aURI)
{
  bool chrome = false;
  aURI->SchemeIs("chrome", &chrome);
  if (chrome)
    return mChromeCache;
  else
    return mCache;
}

imgCacheQueue & imgLoader::GetCacheQueue(nsIURI *aURI)
{
  bool chrome = false;
  aURI->SchemeIs("chrome", &chrome);
  if (chrome)
    return mChromeCacheQueue;
  else
    return mCacheQueue;
}

void imgLoader::GlobalInit()
{
  gCacheObserver = new imgCacheObserver();
  NS_ADDREF(gCacheObserver);

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->AddObserver(gCacheObserver, "memory-pressure", false);

  int32_t timeweight;
  nsresult rv = Preferences::GetInt("image.cache.timeweight", &timeweight);
  if (NS_SUCCEEDED(rv))
    sCacheTimeWeight = timeweight / 1000.0;
  else
    sCacheTimeWeight = 0.5;

  int32_t cachesize;
  rv = Preferences::GetInt("image.cache.size", &cachesize);
  if (NS_SUCCEEDED(rv))
    sCacheMaxSize = cachesize;
  else
    sCacheMaxSize = 5 * 1024 * 1024;

  sMemReporter = new imgMemoryReporter();
  NS_RegisterMemoryMultiReporter(sMemReporter);
  NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(ImagesContentUsedUncompressed));
}

nsresult imgLoader::InitCache()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os)
    return NS_ERROR_FAILURE;

  os->AddObserver(this, "memory-pressure", false);
  os->AddObserver(this, "chrome-flush-skin-caches", false);
  os->AddObserver(this, "chrome-flush-caches", false);
  os->AddObserver(this, "last-pb-context-exited", false);
  os->AddObserver(this, "profile-before-change", false);
  os->AddObserver(this, "xpcom-shutdown", false);

  mCacheTracker = new imgCacheExpirationTracker();

  mCache.Init();
  mChromeCache.Init();

    return NS_OK;
}

nsresult imgLoader::Init()
{
  InitCache();

  ReadAcceptHeaderPref();

  Preferences::AddWeakObserver(this, "image.http.accept");

    return NS_OK;
}

NS_IMETHODIMP
imgLoader::RespectPrivacyNotifications()
{
  mRespectPrivacy = true;
  return NS_OK;
}

NS_IMETHODIMP
imgLoader::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    if (!strcmp(NS_ConvertUTF16toUTF8(aData).get(), "image.http.accept")) {
      ReadAcceptHeaderPref();
    }

  } else if (strcmp(aTopic, "memory-pressure") == 0) {
    MinimizeCaches();
  } else if (strcmp(aTopic, "chrome-flush-skin-caches") == 0 ||
             strcmp(aTopic, "chrome-flush-caches") == 0) {
    MinimizeCaches();
    ClearChromeImageCache();
  } else if (strcmp(aTopic, "last-pb-context-exited") == 0) {
    if (mRespectPrivacy) {
      ClearImageCache();
      ClearChromeImageCache();
    }
  } else if (strcmp(aTopic, "profile-before-change") == 0 ||
             strcmp(aTopic, "xpcom-shutdown") == 0) {
    mCacheTracker = nullptr;
  }

  
  else {
    NS_ABORT_IF_FALSE(0, "Invalid topic received");
  }

  return NS_OK;
}

void imgLoader::ReadAcceptHeaderPref()
{
  nsAdoptingCString accept = Preferences::GetCString("image.http.accept");
  if (accept)
    mAcceptHeader = accept;
  else
    mAcceptHeader = "image/png,image/*;q=0.8,*/*;q=0.5";
}


NS_IMETHODIMP imgLoader::ClearCache(bool chrome)
{
  if (chrome)
    return ClearChromeImageCache();
  else
    return ClearImageCache();
}


NS_IMETHODIMP imgLoader::RemoveEntry(nsIURI *uri)
{
  if (RemoveFromCache(uri))
    return NS_OK;

  return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP imgLoader::FindEntryProperties(nsIURI *uri, nsIProperties **_retval)
{
  nsRefPtr<imgCacheEntry> entry;
  nsAutoCString spec;
  imgCacheTable &cache = GetCache(uri);

  uri->GetSpec(spec);
  *_retval = nullptr;

  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    if (mCacheTracker && entry->HasNoProxies())
      mCacheTracker->MarkUsed(entry);

    nsRefPtr<imgRequest> request = getter_AddRefs(entry->GetRequest());
    if (request) {
      *_retval = request->Properties();
      NS_ADDREF(*_retval);
    }
  }

  return NS_OK;
}

void imgLoader::Shutdown()
{
  NS_RELEASE(gCacheObserver);
}

nsresult imgLoader::ClearChromeImageCache()
{
  return EvictEntries(mChromeCache);
}

nsresult imgLoader::ClearImageCache()
{
  return EvictEntries(mCache);
}

void imgLoader::MinimizeCaches()
{
  EvictEntries(mCacheQueue);
  EvictEntries(mChromeCacheQueue);
}

bool imgLoader::PutIntoCache(nsIURI *key, imgCacheEntry *entry)
{
  imgCacheTable &cache = GetCache(key);

  nsAutoCString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::PutIntoCache", "uri", spec.get());

  
  
  
  nsRefPtr<imgCacheEntry> tmpCacheEntry;
  if (cache.Get(spec, getter_AddRefs(tmpCacheEntry)) && tmpCacheEntry) {
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Element already in the cache", nullptr));
    nsRefPtr<imgRequest> tmpRequest = getter_AddRefs(tmpCacheEntry->GetRequest());

    
    
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Replacing cached element", nullptr));

    RemoveFromCache(key);
  } else {
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Element NOT already in the cache", nullptr));
  }

  cache.Put(spec, entry);

  
  if (entry->Evicted())
    entry->SetEvicted(false);

  
  
  if (entry->HasNoProxies()) {
    nsresult addrv = NS_OK;

    if (mCacheTracker)
      addrv = mCacheTracker->AddObject(entry);

    if (NS_SUCCEEDED(addrv)) {
      imgCacheQueue &queue = GetCacheQueue(key);
      queue.Push(entry);
    }
  }

  nsRefPtr<imgRequest> request(getter_AddRefs(entry->GetRequest()));
  request->SetIsInCache(true);

  return true;
}

bool imgLoader::SetHasNoProxies(nsIURI *key, imgCacheEntry *entry)
{
#if defined(PR_LOGGING)
  nsAutoCString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::SetHasNoProxies", "uri", spec.get());
#endif

  if (entry->Evicted())
    return false;

  imgCacheQueue &queue = GetCacheQueue(key);

  nsresult addrv = NS_OK;

  if (mCacheTracker)
    addrv = mCacheTracker->AddObject(entry);

  if (NS_SUCCEEDED(addrv)) {
    queue.Push(entry);
    entry->SetHasNoProxies(true);
  }

  imgCacheTable &cache = GetCache(key);
  CheckCacheLimits(cache, queue);

  return true;
}

bool imgLoader::SetHasProxies(nsIURI *key)
{
  VerifyCacheSizes();

  imgCacheTable &cache = GetCache(key);

  nsAutoCString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::SetHasProxies", "uri", spec.get());

  nsRefPtr<imgCacheEntry> entry;
  if (cache.Get(spec, getter_AddRefs(entry)) && entry && entry->HasNoProxies()) {
    imgCacheQueue &queue = GetCacheQueue(key);
    queue.Remove(entry);

    if (mCacheTracker)
      mCacheTracker->RemoveObject(entry);

    entry->SetHasNoProxies(false);

    return true;
  }

  return false;
}

void imgLoader::CacheEntriesChanged(nsIURI *uri, int32_t sizediff )
{
  imgCacheQueue &queue = GetCacheQueue(uri);
  queue.MarkDirty();
  queue.UpdateSize(sizediff);
}

void imgLoader::CheckCacheLimits(imgCacheTable &cache, imgCacheQueue &queue)
{
  if (queue.GetNumElements() == 0)
    NS_ASSERTION(queue.GetSize() == 0, 
                 "imgLoader::CheckCacheLimits -- incorrect cache size");

  
  while (queue.GetSize() >= sCacheMaxSize) {
    
    nsRefPtr<imgCacheEntry> entry(queue.Pop());

    NS_ASSERTION(entry, "imgLoader::CheckCacheLimits -- NULL entry pointer");

#if defined(PR_LOGGING)
    nsRefPtr<imgRequest> req(entry->GetRequest());
    if (req) {
      nsCOMPtr<nsIURI> uri;
      req->GetURI(getter_AddRefs(uri));
      nsAutoCString spec;
      uri->GetSpec(spec);
      LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::CheckCacheLimits", "entry", spec.get());
    }
#endif

    if (entry)
      RemoveFromCache(entry);
  }
}

bool imgLoader::ValidateRequestWithNewChannel(imgRequest *request,
                                                nsIURI *aURI,
                                                nsIURI *aInitialDocumentURI,
                                                nsIURI *aReferrerURI,
                                                nsILoadGroup *aLoadGroup,
                                                imgIDecoderObserver *aObserver,
                                                nsISupports *aCX,
                                                nsLoadFlags aLoadFlags,
                                                imgIRequest *aExistingRequest,
                                                imgIRequest **aProxyRequest,
                                                nsIChannelPolicy *aPolicy,
                                                nsIPrincipal* aLoadingPrincipal,
                                                int32_t aCORSMode)
{
  
  
  

  nsresult rv;

  
  
  if (request->mValidator) {
    rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                  aLoadFlags, aExistingRequest, 
                                  reinterpret_cast<imgIRequest **>(aProxyRequest));
    if (NS_FAILED(rv)) {
      return false;
    }

    if (*aProxyRequest) {
      imgRequestProxy* proxy = static_cast<imgRequestProxy*>(*aProxyRequest);

      
      
      
      
      proxy->SetNotificationsDeferred(true);

      
      request->mValidator->AddProxy(proxy);
    }

    return NS_SUCCEEDED(rv);

  } else {
    
    
    
    nsCOMPtr<nsIChannel> newChannel;
    bool forcePrincipalCheck;
    rv = NewImageChannel(getter_AddRefs(newChannel),
                         &forcePrincipalCheck,
                         aURI,
                         aInitialDocumentURI,
                         aReferrerURI,
                         aLoadGroup,
                         mAcceptHeader,
                         aLoadFlags,
                         aPolicy,
                         aLoadingPrincipal);
    if (NS_FAILED(rv)) {
      return false;
    }

    nsCOMPtr<imgIRequest> req;
    rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                  aLoadFlags, aExistingRequest, getter_AddRefs(req));
    if (NS_FAILED(rv)) {
      return false;
    }

    
    nsRefPtr<nsProgressNotificationProxy> progressproxy =
        new nsProgressNotificationProxy(newChannel, req);
    if (!progressproxy)
      return false;

    nsRefPtr<imgCacheValidator> hvc =
      new imgCacheValidator(progressproxy, this, request, aCX, forcePrincipalCheck);

    nsCOMPtr<nsIStreamListener> listener = hvc.get();

    
    
    
    newChannel->SetNotificationCallbacks(hvc);

    if (aCORSMode != imgIRequest::CORS_NONE) {
      bool withCredentials = aCORSMode == imgIRequest::CORS_USE_CREDENTIALS;
      nsRefPtr<nsCORSListenerProxy> corsproxy =
        new nsCORSListenerProxy(hvc, aLoadingPrincipal, withCredentials);
      rv = corsproxy->Init(newChannel);
      if (NS_FAILED(rv)) {
        return false;
      }

      listener = corsproxy;
    }

    request->mValidator = hvc;

    imgRequestProxy* proxy = static_cast<imgRequestProxy*>
                               (static_cast<imgIRequest*>(req.get()));

    
    
    
    
    proxy->SetNotificationsDeferred(true);

    
    hvc->AddProxy(proxy);

    rv = newChannel->AsyncOpen(listener, nullptr);
    if (NS_SUCCEEDED(rv))
      NS_ADDREF(*aProxyRequest = req.get());

    return NS_SUCCEEDED(rv);
  }
}

bool imgLoader::ValidateEntry(imgCacheEntry *aEntry,
                                nsIURI *aURI,
                                nsIURI *aInitialDocumentURI,
                                nsIURI *aReferrerURI,
                                nsILoadGroup *aLoadGroup,
                                imgIDecoderObserver *aObserver,
                                nsISupports *aCX,
                                nsLoadFlags aLoadFlags,
                                bool aCanMakeNewChannel,
                                imgIRequest *aExistingRequest,
                                imgIRequest **aProxyRequest,
                                nsIChannelPolicy *aPolicy,
                                nsIPrincipal* aLoadingPrincipal,
                                int32_t aCORSMode)
{
  LOG_SCOPE(gImgLog, "imgLoader::ValidateEntry");

  bool hasExpired;
  uint32_t expirationTime = aEntry->GetExpiryTime();
  if (expirationTime <= SecondsFromPRTime(PR_Now())) {
    hasExpired = true;
  } else {
    hasExpired = false;
  }

  nsresult rv;

  
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aURI));
  if (fileUrl) {
    uint32_t lastModTime = aEntry->GetTouchedTime();

    nsCOMPtr<nsIFile> theFile;
    rv = fileUrl->GetFile(getter_AddRefs(theFile));
    if (NS_SUCCEEDED(rv)) {
      PRTime fileLastMod;
      rv = theFile->GetLastModifiedTime(&fileLastMod);
      if (NS_SUCCEEDED(rv)) {
        
        fileLastMod *= 1000;
        hasExpired = SecondsFromPRTime((PRTime)fileLastMod) > lastModTime;
      }
    }
  }

  nsRefPtr<imgRequest> request(aEntry->GetRequest());

  if (!request)
    return false;

  if (!ValidateCORSAndPrincipal(request, aEntry->ForcePrincipalCheck(),
                                aCORSMode, aLoadingPrincipal))
    return false;

  bool validateRequest = false;

  
  
  
  
  
  
  void *key = (void *)aCX;
  if (request->mLoadId != key) {
    
    
    if (aLoadFlags & nsIRequest::LOAD_BYPASS_CACHE)
      return false;

    
    validateRequest = ShouldRevalidateEntry(aEntry, aLoadFlags, hasExpired);

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("imgLoader::ValidateEntry validating cache entry. " 
            "validateRequest = %d", validateRequest));
  }
#if defined(PR_LOGGING)
  else if (!key) {
    nsAutoCString spec;
    aURI->GetSpec(spec);

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("imgLoader::ValidateEntry BYPASSING cache validation for %s " 
            "because of NULL LoadID", spec.get()));
  }
#endif

  
  
  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer;
  nsCOMPtr<nsIApplicationCache> requestAppCache;
  nsCOMPtr<nsIApplicationCache> groupAppCache;
  if ((appCacheContainer = do_GetInterface(request->mRequest)))
    appCacheContainer->GetApplicationCache(getter_AddRefs(requestAppCache));
  if ((appCacheContainer = do_QueryInterface(aLoadGroup)))
    appCacheContainer->GetApplicationCache(getter_AddRefs(groupAppCache));

  if (requestAppCache != groupAppCache) {
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("imgLoader::ValidateEntry - Unable to use cached imgRequest "
            "[request=%p] because of mismatched application caches\n",
            address_of(request)));
    return false;
  }

  if (validateRequest && aCanMakeNewChannel) {
    LOG_SCOPE(gImgLog, "imgLoader::ValidateRequest |cache hit| must validate");

    return ValidateRequestWithNewChannel(request, aURI, aInitialDocumentURI,
                                         aReferrerURI, aLoadGroup, aObserver,
                                         aCX, aLoadFlags, aExistingRequest,
                                         aProxyRequest, aPolicy,
                                         aLoadingPrincipal, aCORSMode);
  }

  return !validateRequest;
}


bool imgLoader::RemoveFromCache(nsIURI *aKey)
{
  if (!aKey) return false;

  imgCacheTable &cache = GetCache(aKey);
  imgCacheQueue &queue = GetCacheQueue(aKey);

  nsAutoCString spec;
  aKey->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::RemoveFromCache", "uri", spec.get());

  nsRefPtr<imgCacheEntry> entry;
  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    cache.Remove(spec);

    NS_ABORT_IF_FALSE(!entry->Evicted(), "Evicting an already-evicted cache entry!");

    
    if (entry->HasNoProxies()) {
      if (mCacheTracker)
        mCacheTracker->RemoveObject(entry);
      queue.Remove(entry);
    }

    entry->SetEvicted(true);

    nsRefPtr<imgRequest> request(getter_AddRefs(entry->GetRequest()));
    request->SetIsInCache(false);

    return true;
  }
  else
    return false;
}

bool imgLoader::RemoveFromCache(imgCacheEntry *entry)
{
  LOG_STATIC_FUNC(gImgLog, "imgLoader::RemoveFromCache entry");

  nsRefPtr<imgRequest> request(getter_AddRefs(entry->GetRequest()));
  if (request) {
    nsCOMPtr<nsIURI> key;
    if (NS_SUCCEEDED(request->GetURI(getter_AddRefs(key))) && key) {
      imgCacheTable &cache = GetCache(key);
      imgCacheQueue &queue = GetCacheQueue(key);
      nsAutoCString spec;
      key->GetSpec(spec);

      LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::RemoveFromCache", "entry's uri", spec.get());

      cache.Remove(spec);

      if (entry->HasNoProxies()) {
        LOG_STATIC_FUNC(gImgLog, "imgLoader::RemoveFromCache removing from tracker");
        if (mCacheTracker)
          mCacheTracker->RemoveObject(entry);
        queue.Remove(entry);
      }

      entry->SetEvicted(true);
      request->SetIsInCache(false);

      return true;
    }
  }

  return false;
}

static PLDHashOperator EnumEvictEntries(const nsACString&, 
                                        nsRefPtr<imgCacheEntry> &aData,
                                        void *data)
{
  nsTArray<nsRefPtr<imgCacheEntry> > *entries = 
    reinterpret_cast<nsTArray<nsRefPtr<imgCacheEntry> > *>(data);

  entries->AppendElement(aData);

  return PL_DHASH_NEXT;
}

nsresult imgLoader::EvictEntries(imgCacheTable &aCacheToClear)
{
  LOG_STATIC_FUNC(gImgLog, "imgLoader::EvictEntries table");

  
  
  nsTArray<nsRefPtr<imgCacheEntry> > entries;
  aCacheToClear.Enumerate(EnumEvictEntries, &entries);

  for (uint32_t i = 0; i < entries.Length(); ++i)
    if (!RemoveFromCache(entries[i]))
      return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult imgLoader::EvictEntries(imgCacheQueue &aQueueToClear)
{
  LOG_STATIC_FUNC(gImgLog, "imgLoader::EvictEntries queue");

  
  
  nsTArray<nsRefPtr<imgCacheEntry> > entries(aQueueToClear.GetNumElements());
  for (imgCacheQueue::const_iterator i = aQueueToClear.begin(); i != aQueueToClear.end(); ++i)
    entries.AppendElement(*i);

  for (uint32_t i = 0; i < entries.Length(); ++i)
    if (!RemoveFromCache(entries[i]))
      return NS_ERROR_FAILURE;

  return NS_OK;
}

#define LOAD_FLAGS_CACHE_MASK    (nsIRequest::LOAD_BYPASS_CACHE | \
                                  nsIRequest::LOAD_FROM_CACHE)

#define LOAD_FLAGS_VALIDATE_MASK (nsIRequest::VALIDATE_ALWAYS |   \
                                  nsIRequest::VALIDATE_NEVER |    \
                                  nsIRequest::VALIDATE_ONCE_PER_SESSION)




NS_IMETHODIMP imgLoader::LoadImage(nsIURI *aURI, 
                                   nsIURI *aInitialDocumentURI,
                                   nsIURI *aReferrerURI,
                                   nsIPrincipal* aLoadingPrincipal,
                                   nsILoadGroup *aLoadGroup,
                                   imgIDecoderObserver *aObserver,
                                   nsISupports *aCX,
                                   nsLoadFlags aLoadFlags,
                                   nsISupports *aCacheKey,
                                   imgIRequest *aRequest,
                                   nsIChannelPolicy *aPolicy,
                                   imgIRequest **_retval)
{
  VerifyCacheSizes();

  NS_ASSERTION(aURI, "imgLoader::LoadImage -- NULL URI pointer");

  if (!aURI)
    return NS_ERROR_NULL_POINTER;

  nsAutoCString spec;
  aURI->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgLoader::LoadImage", "aURI", spec.get());

  *_retval = nullptr;

  nsRefPtr<imgRequest> request;

  nsresult rv;
  nsLoadFlags requestFlags = nsIRequest::LOAD_NORMAL;

#ifdef DEBUG
  bool isPrivate = false;

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (channel) {
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(channel, loadContext);
    isPrivate = loadContext && loadContext->UsePrivateBrowsing();
  } else if (aLoadGroup) {
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    aLoadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (callbacks) {
      nsCOMPtr<nsILoadContext> loadContext = do_GetInterface(callbacks);
      isPrivate = loadContext && loadContext->UsePrivateBrowsing();
    }
  }
  MOZ_ASSERT(isPrivate == mRespectPrivacy);
#endif

  
  if (aLoadGroup) {
    aLoadGroup->GetLoadFlags(&requestFlags);
  }
  
  
  
  
  
  
  
  if (aLoadFlags & LOAD_FLAGS_CACHE_MASK) {
    
    requestFlags = (requestFlags & ~LOAD_FLAGS_CACHE_MASK) |
                   (aLoadFlags & LOAD_FLAGS_CACHE_MASK);
  }
  if (aLoadFlags & LOAD_FLAGS_VALIDATE_MASK) {
    
    requestFlags = (requestFlags & ~LOAD_FLAGS_VALIDATE_MASK) |
                   (aLoadFlags & LOAD_FLAGS_VALIDATE_MASK);
  }
  if (aLoadFlags & nsIRequest::LOAD_BACKGROUND) {
    
    requestFlags |= nsIRequest::LOAD_BACKGROUND;
  }

  int32_t corsmode = imgIRequest::CORS_NONE;
  if (aLoadFlags & imgILoader::LOAD_CORS_ANONYMOUS) {
    corsmode = imgIRequest::CORS_ANONYMOUS;
  } else if (aLoadFlags & imgILoader::LOAD_CORS_USE_CREDENTIALS) {
    corsmode = imgIRequest::CORS_USE_CREDENTIALS;
  }

  nsRefPtr<imgCacheEntry> entry;

  
  
  
  
  imgCacheTable &cache = GetCache(aURI);

  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    if (ValidateEntry(entry, aURI, aInitialDocumentURI, aReferrerURI,
                      aLoadGroup, aObserver, aCX, requestFlags, true,
                      aRequest, _retval, aPolicy, aLoadingPrincipal, corsmode)) {
      request = getter_AddRefs(entry->GetRequest());

      
      if (entry->HasNoProxies()) {
        LOG_FUNC_WITH_PARAM(gImgLog, "imgLoader::LoadImage() adding proxyless entry", "uri", spec.get());
        NS_ABORT_IF_FALSE(!request->HasCacheEntry(), "Proxyless entry's request has cache entry!");
        request->SetCacheEntry(entry);

        if (mCacheTracker)
          mCacheTracker->MarkUsed(entry);
      } 

      entry->Touch();

#ifdef DEBUG_joe
      printf("CACHEGET: %d %s %d\n", time(NULL), spec.get(), entry->SizeOfData());
#endif
    }
    else {
      
      
      entry = nullptr;
    }
  }

  
  
  nsCOMPtr<nsIChannel> newChannel;
  
  if (!request) {
    LOG_SCOPE(gImgLog, "imgLoader::LoadImage |cache miss|");

    bool forcePrincipalCheck;
    rv = NewImageChannel(getter_AddRefs(newChannel),
                         &forcePrincipalCheck,
                         aURI,
                         aInitialDocumentURI,
                         aReferrerURI,
                         aLoadGroup,
                         mAcceptHeader,
                         requestFlags,
                         aPolicy,
                         aLoadingPrincipal);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

#ifdef DEBUG
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(newChannel, loadContext);
    MOZ_ASSERT_IF(loadContext, loadContext->UsePrivateBrowsing() == mRespectPrivacy);
#endif

    NewRequestAndEntry(forcePrincipalCheck, this, getter_AddRefs(request), getter_AddRefs(entry));

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- Created new imgRequest [request=%p]\n", this, request.get()));

    
    
    nsCOMPtr<nsILoadGroup> loadGroup =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID);
    newChannel->SetLoadGroup(loadGroup);

    request->Init(aURI, aURI, loadGroup, newChannel, entry, aCX,
                  aLoadingPrincipal, corsmode);

    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aCX);
    if (doc) {
      request->SetInnerWindowID(doc->InnerWindowID());
    }

    
    nsCOMPtr<nsIStreamListener> pl = new ProxyListener(request.get());

    
    
    nsCOMPtr<nsIStreamListener> listener = pl;
    if (corsmode != imgIRequest::CORS_NONE) {
      PR_LOG(gImgLog, PR_LOG_DEBUG,
             ("[this=%p] imgLoader::LoadImage -- Setting up a CORS load",
              this));
      bool withCredentials = corsmode == imgIRequest::CORS_USE_CREDENTIALS;

      nsRefPtr<nsCORSListenerProxy> corsproxy =
        new nsCORSListenerProxy(pl, aLoadingPrincipal, withCredentials);
      rv = corsproxy->Init(newChannel);
      if (NS_FAILED(rv)) {
        PR_LOG(gImgLog, PR_LOG_DEBUG,
               ("[this=%p] imgLoader::LoadImage -- nsCORSListenerProxy "
                "creation failed: 0x%x\n", this, rv));
        request->CancelAndAbort(rv);
        return NS_ERROR_FAILURE;
      }

      listener = corsproxy;
    }

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- Calling channel->AsyncOpen()\n", this));

    nsresult openRes = newChannel->AsyncOpen(listener, nullptr);

    if (NS_FAILED(openRes)) {
      PR_LOG(gImgLog, PR_LOG_DEBUG,
             ("[this=%p] imgLoader::LoadImage -- AsyncOpen() failed: 0x%x\n",
              this, openRes));
      request->CancelAndAbort(openRes);
      return openRes;
    }

    
    PutIntoCache(aURI, entry);
  } else {
    LOG_MSG_WITH_PARAM(gImgLog, 
                       "imgLoader::LoadImage |cache hit|", "request", request);
  }


  
  if (!*_retval) {
    
    
    
    
    
    
    
    
    
    request->SetLoadId(aCX);

    LOG_MSG(gImgLog, "imgLoader::LoadImage", "creating proxy request.");
    rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                  requestFlags, aRequest, _retval);
    if (NS_FAILED(rv)) {
      return rv;
    }

    imgRequestProxy *proxy = static_cast<imgRequestProxy *>(*_retval);

    
    
    if (newChannel) {
      nsCOMPtr<nsIInterfaceRequestor> requestor(
          new nsProgressNotificationProxy(newChannel, proxy));
      if (!requestor)
        return NS_ERROR_OUT_OF_MEMORY;
      newChannel->SetNotificationCallbacks(requestor);
    }

    
    
    
    proxy->AddToLoadGroup();

    
    
    
    
    
    if (!newChannel)
      proxy->NotifyListener();

    return rv;
  }

  NS_ASSERTION(*_retval, "imgLoader::LoadImage -- no return value");

  return NS_OK;
}


NS_IMETHODIMP imgLoader::LoadImageWithChannel(nsIChannel *channel, imgIDecoderObserver *aObserver, nsISupports *aCX, nsIStreamListener **listener, imgIRequest **_retval)
{
  NS_ASSERTION(channel, "imgLoader::LoadImageWithChannel -- NULL channel pointer");

#ifdef DEBUG
  nsCOMPtr<nsILoadContext> loadContext;
  NS_QueryNotificationCallbacks(channel, loadContext);
  MOZ_ASSERT_IF(loadContext, loadContext->UsePrivateBrowsing() == mRespectPrivacy);
#endif

  nsRefPtr<imgRequest> request;

  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));

  nsLoadFlags requestFlags = nsIRequest::LOAD_NORMAL;
  channel->GetLoadFlags(&requestFlags);

  nsRefPtr<imgCacheEntry> entry;

  if (requestFlags & nsIRequest::LOAD_BYPASS_CACHE) {
    RemoveFromCache(uri);
  } else {
    
    
    
    
    imgCacheTable &cache = GetCache(uri);
    nsAutoCString spec;

    uri->GetSpec(spec);

    if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
      
      
      
      
      
      
      
      if (ValidateEntry(entry, uri, nullptr, nullptr, nullptr, aObserver, aCX,
                        requestFlags, false, nullptr, nullptr, nullptr,
                        nullptr, imgIRequest::CORS_NONE)) {
        request = getter_AddRefs(entry->GetRequest());
      } else {
        nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(channel));
        bool bUseCacheCopy;

        if (cacheChan)
          cacheChan->IsFromCache(&bUseCacheCopy);
        else
          bUseCacheCopy = false;

        if (!bUseCacheCopy)
          entry = nullptr;
        else {
          request = getter_AddRefs(entry->GetRequest());
        }
      }

      if (request && entry) {
        
        if (entry->HasNoProxies()) {
          LOG_FUNC_WITH_PARAM(gImgLog, "imgLoader::LoadImageWithChannel() adding proxyless entry", "uri", spec.get());
          NS_ABORT_IF_FALSE(!request->HasCacheEntry(), "Proxyless entry's request has cache entry!");
          request->SetCacheEntry(entry);

          if (mCacheTracker)
            mCacheTracker->MarkUsed(entry);
        } 
      }
    }
  }

  nsCOMPtr<nsILoadGroup> loadGroup;
  channel->GetLoadGroup(getter_AddRefs(loadGroup));

  
  requestFlags &= nsIRequest::LOAD_REQUESTMASK;

  nsresult rv = NS_OK;
  if (request) {
    

    channel->Cancel(NS_ERROR_PARSED_DATA_CACHED); 

    *listener = nullptr; 

    rv = CreateNewProxyForRequest(request, loadGroup, aObserver,
                                  requestFlags, nullptr, _retval);
    static_cast<imgRequestProxy*>(*_retval)->NotifyListener();
  } else {
    
    
    
    NewRequestAndEntry(true, this, getter_AddRefs(request), getter_AddRefs(entry));

    
    nsCOMPtr<nsIURI> originalURI;
    channel->GetOriginalURI(getter_AddRefs(originalURI));

    
    request->Init(originalURI, uri, channel, channel, entry,
                  aCX, nullptr, imgIRequest::CORS_NONE);

    ProxyListener *pl = new ProxyListener(static_cast<nsIStreamListener *>(request.get()));
    NS_ADDREF(pl);

    *listener = static_cast<nsIStreamListener*>(pl);
    NS_ADDREF(*listener);

    NS_RELEASE(pl);

    
    PutIntoCache(originalURI, entry);

    rv = CreateNewProxyForRequest(request, loadGroup, aObserver,
                                  requestFlags, nullptr, _retval);

    
    
    
    
    
    
  }

  return rv;
}

bool imgLoader::SupportImageWithMimeType(const char* aMimeType)
{
  nsAutoCString mimeType(aMimeType);
  ToLowerCase(mimeType);
  return Image::GetDecoderType(mimeType.get()) != Image::eDecoderType_unknown;
}

NS_IMETHODIMP imgLoader::GetMIMETypeFromContent(nsIRequest* aRequest,
                                                const uint8_t* aContents,
                                                uint32_t aLength,
                                                nsACString& aContentType)
{
  return GetMimeTypeFromContent((const char*)aContents, aLength, aContentType);
}


nsresult imgLoader::GetMimeTypeFromContent(const char* aContents, uint32_t aLength, nsACString& aContentType)
{
  
  if (aLength >= 6 && (!nsCRT::strncmp(aContents, "GIF87a", 6) ||
                       !nsCRT::strncmp(aContents, "GIF89a", 6)))
  {
    aContentType.AssignLiteral("image/gif");
  }

  
  else if (aLength >= 8 && ((unsigned char)aContents[0]==0x89 &&
                   (unsigned char)aContents[1]==0x50 &&
                   (unsigned char)aContents[2]==0x4E &&
                   (unsigned char)aContents[3]==0x47 &&
                   (unsigned char)aContents[4]==0x0D &&
                   (unsigned char)aContents[5]==0x0A &&
                   (unsigned char)aContents[6]==0x1A &&
                   (unsigned char)aContents[7]==0x0A))
  { 
    aContentType.AssignLiteral("image/png");
  }

  
  





  else if (aLength >= 3 &&
     ((unsigned char)aContents[0])==0xFF &&
     ((unsigned char)aContents[1])==0xD8 &&
     ((unsigned char)aContents[2])==0xFF)
  {
    aContentType.AssignLiteral("image/jpeg");
  }

  
  


  else if (aLength >= 5 &&
   ((unsigned char) aContents[0])==0x4a &&
   ((unsigned char) aContents[1])==0x47 &&
   ((unsigned char) aContents[4])==0x00 )
  {
    aContentType.AssignLiteral("image/x-jg");
  }

  else if (aLength >= 2 && !nsCRT::strncmp(aContents, "BM", 2)) {
    aContentType.AssignLiteral("image/bmp");
  }

  
  
  else if (aLength >= 4 && (!memcmp(aContents, "\000\000\001\000", 4) ||
                            !memcmp(aContents, "\000\000\002\000", 4))) {
    aContentType.AssignLiteral("image/x-icon");
  }

  else {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}





#include "nsIRequest.h"
#include "nsIStreamConverterService.h"
#include "nsXPIDLString.h"

NS_IMPL_ISUPPORTS2(ProxyListener, nsIStreamListener, nsIRequestObserver)

ProxyListener::ProxyListener(nsIStreamListener *dest) :
  mDestListener(dest)
{
  
}

ProxyListener::~ProxyListener()
{
  
}





NS_IMETHODIMP ProxyListener::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    nsAutoCString contentType;
    nsresult rv = channel->GetContentType(contentType);

    if (!contentType.IsEmpty()) {
     



      if (NS_LITERAL_CSTRING("multipart/x-mixed-replace").Equals(contentType)) {

        nsCOMPtr<nsIStreamConverterService> convServ(do_GetService("@mozilla.org/streamConverters;1", &rv));
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIStreamListener> toListener(mDestListener);
          nsCOMPtr<nsIStreamListener> fromListener;

          rv = convServ->AsyncConvertData("multipart/x-mixed-replace",
                                          "*/*",
                                          toListener,
                                          nullptr,
                                          getter_AddRefs(fromListener));
          if (NS_SUCCEEDED(rv))
            mDestListener = fromListener;
        }
      }
    }
  }

  return mDestListener->OnStartRequest(aRequest, ctxt);
}


NS_IMETHODIMP ProxyListener::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}




NS_IMETHODIMP
ProxyListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt,
                               nsIInputStream *inStr, uint64_t sourceOffset,
                               uint32_t count)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}





NS_IMPL_ISUPPORTS5(imgCacheValidator, nsIStreamListener, nsIRequestObserver,
                   nsIChannelEventSink, nsIInterfaceRequestor,
                   nsIAsyncVerifyRedirectCallback)

imgCacheValidator::imgCacheValidator(nsProgressNotificationProxy* progress,
                                     imgLoader* loader, imgRequest *request,
                                     void *aContext, bool forcePrincipalCheckForCacheEntry)
 : mProgressProxy(progress),
   mRequest(request),
   mContext(aContext),
   mImgLoader(loader)
{
  NewRequestAndEntry(forcePrincipalCheckForCacheEntry, loader, getter_AddRefs(mNewRequest),
                     getter_AddRefs(mNewEntry));
}

imgCacheValidator::~imgCacheValidator()
{
  if (mRequest) {
    mRequest->mValidator = nullptr;
  }
}

void imgCacheValidator::AddProxy(imgRequestProxy *aProxy)
{
  
  
  aProxy->AddToLoadGroup();

  mProxies.AppendObject(aProxy);
}




NS_IMETHODIMP imgCacheValidator::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  
  
  
  nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(aRequest));
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (cacheChan && channel) {
    bool isFromCache = false;
    cacheChan->IsFromCache(&isFromCache);

    nsCOMPtr<nsIURI> channelURI;
    bool sameURI = false;
    channel->GetURI(getter_AddRefs(channelURI));
    if (channelURI)
      channelURI->Equals(mRequest->mCurrentURI, &sameURI);

    if (isFromCache && sameURI) {
      uint32_t count = mProxies.Count();
      for (int32_t i = count-1; i>=0; i--) {
        imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);

        
        
        NS_ABORT_IF_FALSE(proxy->NotificationsDeferred(),
                          "Proxies waiting on cache validation should be "
                          "deferring notifications!");
        proxy->SetNotificationsDeferred(false);

        
        
        proxy->SyncNotifyListener();
      }

      
      aRequest->Cancel(NS_BINDING_ABORTED);

      mRequest->SetLoadId(mContext);
      mRequest->mValidator = nullptr;

      mRequest = nullptr;

      mNewRequest = nullptr;
      mNewEntry = nullptr;

      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIURI> uri;
  mRequest->GetURI(getter_AddRefs(uri));

#if defined(PR_LOGGING)
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_MSG_WITH_PARAM(gImgLog, "imgCacheValidator::OnStartRequest creating new request", "uri", spec.get());
#endif

  int32_t corsmode = mRequest->GetCORSMode();
  nsCOMPtr<nsIPrincipal> loadingPrincipal = mRequest->GetLoadingPrincipal();

  
  mRequest->RemoveFromCache();

  mRequest->mValidator = nullptr;
  mRequest = nullptr;

  
  nsCOMPtr<nsIURI> originalURI;
  channel->GetOriginalURI(getter_AddRefs(originalURI));
  mNewRequest->Init(originalURI, uri, aRequest, channel, mNewEntry,
                    mContext, loadingPrincipal,
                    corsmode);

  mDestListener = new ProxyListener(mNewRequest);

  
  
  
  mImgLoader->PutIntoCache(originalURI, mNewEntry);

  uint32_t count = mProxies.Count();
  for (int32_t i = count-1; i>=0; i--) {
    imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);
    proxy->ChangeOwner(mNewRequest);

    
    
    NS_ABORT_IF_FALSE(proxy->NotificationsDeferred(),
                      "Proxies waiting on cache validation should be "
                      "deferring notifications!");
    proxy->SetNotificationsDeferred(false);

    
    
    proxy->SyncNotifyListener();
  }

  mNewRequest = nullptr;
  mNewEntry = nullptr;

  return mDestListener->OnStartRequest(aRequest, ctxt);
}


NS_IMETHODIMP imgCacheValidator::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  if (!mDestListener)
    return NS_OK;

  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}





NS_IMETHODIMP
imgCacheValidator::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt,
                                   nsIInputStream *inStr,
                                   uint64_t sourceOffset, uint32_t count)
{
  if (!mDestListener) {
    
    uint32_t _retval;
    inStr->ReadSegments(NS_DiscardSegment, nullptr, count, &_retval);
    return NS_OK;
  }

  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}



NS_IMETHODIMP imgCacheValidator::GetInterface(const nsIID & aIID, void **aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink)))
    return QueryInterface(aIID, aResult);

  return mProgressProxy->GetInterface(aIID, aResult);
}






NS_IMETHODIMP imgCacheValidator::AsyncOnChannelRedirect(nsIChannel *oldChannel,
                                                        nsIChannel *newChannel, uint32_t flags,
                                                        nsIAsyncVerifyRedirectCallback *callback)
{
  
  mNewRequest->SetCacheValidation(mNewEntry, oldChannel);

  
  mRedirectCallback = callback;
  mRedirectChannel = newChannel;

  return mProgressProxy->AsyncOnChannelRedirect(oldChannel, newChannel, flags, this);
}

NS_IMETHODIMP imgCacheValidator::OnRedirectVerifyCallback(nsresult aResult)
{
  
  if (NS_FAILED(aResult)) {
      mRedirectCallback->OnRedirectVerifyCallback(aResult);
      mRedirectCallback = nullptr;
      mRedirectChannel = nullptr;
      return NS_OK;
  }

  
  
  nsCOMPtr<nsIURI> uri;
  mRedirectChannel->GetURI(getter_AddRefs(uri));
  bool doesNotReturnData = false;
  NS_URIChainHasFlags(uri, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                      &doesNotReturnData);

  nsresult result = NS_OK;

  if (doesNotReturnData) {
    result = NS_ERROR_ABORT;
  }

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nullptr;
  mRedirectChannel = nullptr;
  return NS_OK;
}
