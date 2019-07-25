






































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
#include "nsIPrivateBrowsingService.h"




#include "nsIHttpChannelInternal.h"  
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/Preferences.h"

#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::imagelib;

#if defined(DEBUG_pavlov) || defined(DEBUG_timeless)
#include "nsISimpleEnumerator.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsComponentManagerUtils.h"


static void PrintImageDecoders()
{
  nsCOMPtr<nsIComponentRegistrar> compMgr;
  if (NS_FAILED(NS_GetComponentRegistrar(getter_AddRefs(compMgr))) || !compMgr)
    return;
  nsCOMPtr<nsISimpleEnumerator> enumer;
  if (NS_FAILED(compMgr->EnumerateContractIDs(getter_AddRefs(enumer))) || !enumer)
    return;
  
  nsCString str;
  nsCOMPtr<nsISupports> s;
  bool more = false;
  while (NS_SUCCEEDED(enumer->HasMoreElements(&more)) && more) {
    enumer->GetNext(getter_AddRefs(s));
    if (s) {
      nsCOMPtr<nsISupportsCString> ss(do_QueryInterface(s));

      nsCAutoString xcs;
      ss->GetData(xcs);

      NS_NAMED_LITERAL_CSTRING(decoderContract, "@mozilla.org/image/decoder;3?type=");

      if (StringBeginsWith(xcs, decoderContract)) {
        printf("Have decoder for mime type: %s\n", xcs.get()+decoderContract.Length());
      }
    }
  }
}
#endif


class imgMemoryReporter :
  public nsIMemoryReporter
{
public:
  enum ReporterType {
    CHROME_BIT = PR_BIT(0),
    USED_BIT   = PR_BIT(1),
    RAW_BIT    = PR_BIT(2),
    HEAP_BIT   = PR_BIT(3),

    ChromeUsedRaw                     = CHROME_BIT | USED_BIT | RAW_BIT | HEAP_BIT,
    ChromeUsedUncompressedHeap        = CHROME_BIT | USED_BIT | HEAP_BIT,
    ChromeUsedUncompressedNonheap     = CHROME_BIT | USED_BIT,
    ChromeUnusedRaw                   = CHROME_BIT | RAW_BIT | HEAP_BIT,
    ChromeUnusedUncompressedHeap      = CHROME_BIT | HEAP_BIT,
    ChromeUnusedUncompressedNonheap   = CHROME_BIT,
    ContentUsedRaw                    = USED_BIT | RAW_BIT | HEAP_BIT,
    ContentUsedUncompressedHeap       = USED_BIT | HEAP_BIT,
    ContentUsedUncompressedNonheap    = USED_BIT,
    ContentUnusedRaw                  = RAW_BIT | HEAP_BIT,
    ContentUnusedUncompressedHeap     = HEAP_BIT,
    ContentUnusedUncompressedNonheap  = 0
  };

  imgMemoryReporter(ReporterType aType)
    : mType(aType)
  {
    
    
    NS_ASSERTION(!(aType & RAW_BIT) || (aType & HEAP_BIT),
                 "RAW bit should imply HEAP bit.");
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetProcess(nsACString &process)
  {
    process.Truncate();
    return NS_OK;
  }

  NS_IMETHOD GetPath(nsACString &path)
  {
    if (mType == ChromeUsedRaw) {
      path.AssignLiteral("explicit/images/chrome/used/raw");
    } else if (mType == ChromeUsedUncompressedHeap) {
      path.AssignLiteral("explicit/images/chrome/used/uncompressed-heap");
    } else if (mType == ChromeUsedUncompressedNonheap) {
      path.AssignLiteral("explicit/images/chrome/used/uncompressed-nonheap");
    } else if (mType == ChromeUnusedRaw) {
      path.AssignLiteral("explicit/images/chrome/unused/raw");
    } else if (mType == ChromeUnusedUncompressedHeap) {
      path.AssignLiteral("explicit/images/chrome/unused/uncompressed-heap");
    } else if (mType == ChromeUnusedUncompressedNonheap) {
      path.AssignLiteral("explicit/images/chrome/unused/uncompressed-nonheap");
    } else if (mType == ContentUsedRaw) {
      path.AssignLiteral("explicit/images/content/used/raw");
    } else if (mType == ContentUsedUncompressedHeap) {
      path.AssignLiteral("explicit/images/content/used/uncompressed-heap");
    } else if (mType == ContentUsedUncompressedNonheap) {
      path.AssignLiteral("explicit/images/content/used/uncompressed-nonheap");
    } else if (mType == ContentUnusedRaw) {
      path.AssignLiteral("explicit/images/content/unused/raw");
    } else if (mType == ContentUnusedUncompressedHeap) {
      path.AssignLiteral("explicit/images/content/unused/uncompressed-heap");
    } else if (mType == ContentUnusedUncompressedNonheap) {
      path.AssignLiteral("explicit/images/content/unused/uncompressed-nonheap");
    }
    return NS_OK;
  }

  NS_IMETHOD GetKind(PRInt32 *kind)
  {
    if (mType & HEAP_BIT) {
      *kind = KIND_HEAP;
    }
    else {
      *kind = KIND_MAPPED;
    }
    return NS_OK;
  }

  NS_IMETHOD GetUnits(PRInt32 *units)
  {
    *units = UNITS_BYTES;
    return NS_OK;
  }

  struct EnumArg {
    EnumArg(ReporterType aType)
      : rtype(aType), value(0)
    { }

    ReporterType rtype;
    PRInt32 value;
  };

  static PLDHashOperator EnumEntries(const nsACString&,
                                     imgCacheEntry *entry,
                                     void *userArg)
  {
    EnumArg *arg = static_cast<EnumArg*>(userArg);
    ReporterType rtype = arg->rtype;

    if (rtype & USED_BIT) {
      if (entry->HasNoProxies())
        return PL_DHASH_NEXT;
    } else {
      if (!entry->HasNoProxies())
        return PL_DHASH_NEXT;
    }

    nsRefPtr<imgRequest> req = entry->GetRequest();
    Image *image = static_cast<Image*>(req->mImage.get());
    if (!image)
      return PL_DHASH_NEXT;

    if (rtype & RAW_BIT) {
      arg->value += image->GetSourceHeapSize();
    } else if (rtype & HEAP_BIT) {
      arg->value += image->GetDecodedHeapSize();
    } else {
      arg->value += image->GetDecodedNonheapSize();
    }

    return PL_DHASH_NEXT;
  }

  NS_IMETHOD GetAmount(PRInt64 *amount)
  {
    EnumArg arg(mType);
    if (mType & CHROME_BIT) {
      imgLoader::sChromeCache.EnumerateRead(EnumEntries, &arg);
    } else {
      imgLoader::sCache.EnumerateRead(EnumEntries, &arg);
    }

    *amount = arg.value;
    return NS_OK;
  }

  NS_IMETHOD GetDescription(nsACString &desc)
  {
    if (mType == ChromeUsedRaw) {
      desc.AssignLiteral("Memory used by in-use chrome images (compressed data).");
    } else if (mType == ChromeUsedUncompressedHeap) {
      desc.AssignLiteral("Memory used by in-use chrome images (uncompressed data).");
    } else if (mType == ChromeUsedUncompressedNonheap) {
      desc.AssignLiteral("Memory used by in-use chrome images (uncompressed data).");
    } else if (mType == ChromeUnusedRaw) {
      desc.AssignLiteral("Memory used by not in-use chrome images (compressed data).");
    } else if (mType == ChromeUnusedUncompressedHeap) {
      desc.AssignLiteral("Memory used by not in-use chrome images (uncompressed data).");
    } else if (mType == ChromeUnusedUncompressedNonheap) {
      desc.AssignLiteral("Memory used by not in-use chrome images (uncompressed data).");
    } else if (mType == ContentUsedRaw) {
      desc.AssignLiteral("Memory used by in-use content images (compressed data).");
    } else if (mType == ContentUsedUncompressedHeap) {
      desc.AssignLiteral("Memory used by in-use content images (uncompressed data).");
    } else if (mType == ContentUsedUncompressedNonheap) {
      desc.AssignLiteral("Memory used by in-use content images (uncompressed data).");
    } else if (mType == ContentUnusedRaw) {
      desc.AssignLiteral("Memory used by not in-use content images (compressed data).");
    } else if (mType == ContentUnusedUncompressedHeap) {
      desc.AssignLiteral("Memory used by not in-use content images (uncompressed data).");
    } else if (mType == ContentUnusedUncompressedNonheap) {
      desc.AssignLiteral("Memory used by not in-use content images (uncompressed data).");
    }
    return NS_OK;
  }

  ReporterType mType;
};

NS_IMPL_ISUPPORTS1(imgMemoryReporter, nsIMemoryReporter)

NS_IMPL_ISUPPORTS3(nsProgressNotificationProxy,
                     nsIProgressEventSink,
                     nsIChannelEventSink,
                     nsIInterfaceRequestor)

NS_IMETHODIMP
nsProgressNotificationProxy::OnProgress(nsIRequest* request,
                                        nsISupports* ctxt,
                                        PRUint64 progress,
                                        PRUint64 progressMax)
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
                                                    PRUint32 flags,
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

static void NewRequestAndEntry(bool aForcePrincipalCheckForCacheEntry,
                               imgRequest **aRequest, imgCacheEntry **aEntry)
{
  nsRefPtr<imgRequest> request = new imgRequest();
  nsRefPtr<imgCacheEntry> entry = new imgCacheEntry(request, aForcePrincipalCheckForCacheEntry);
  request.forget(aRequest);
  entry.forget(aEntry);
}

static bool ShouldRevalidateEntry(imgCacheEntry *aEntry,
                              nsLoadFlags aFlags,
                              bool aHasExpired)
{
  bool bValidateEntry = false;

  if (aFlags & nsIRequest::LOAD_BYPASS_CACHE)
    return PR_FALSE;

  if (aFlags & nsIRequest::VALIDATE_ALWAYS) {
    bValidateEntry = PR_TRUE;
  }
  else if (aEntry->GetMustValidate()) {
    bValidateEntry = PR_TRUE;
  }
  
  
  
  
  else if (aHasExpired) {
    
    
    
    
    
    if (aFlags & (nsIRequest::VALIDATE_NEVER | 
                  nsIRequest::VALIDATE_ONCE_PER_SESSION)) 
    {
      bValidateEntry = PR_FALSE;
    }
    
    
    
    
    else if (!(aFlags & nsIRequest::LOAD_FROM_CACHE)) {
      bValidateEntry = PR_TRUE;
    }
  }

  return bValidateEntry;
}




static bool
ValidateCORSAndPrincipal(imgRequest* request, bool forcePrincipalCheck,
                         PRInt32 corsmode, nsIPrincipal* loadingPrincipal)
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
                     nsnull,      
                     nsnull,      
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
                                     PR_FALSE);

    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal = do_QueryInterface(newHttpChannel);
    NS_ENSURE_TRUE(httpChannelInternal, NS_ERROR_UNEXPECTED);
    httpChannelInternal->SetDocumentURI(aInitialDocumentURI);
    newHttpChannel->SetReferrer(aReferringURI);
  }

  
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(*aResult);
  if (p) {
    PRUint32 priority = nsISupportsPriority::PRIORITY_LOW;

    if (aLoadFlags & nsIRequest::LOAD_BACKGROUND)
      ++priority; 

    p->AdjustPriority(priority);
  }

  bool setOwner = nsContentUtils::SetUpChannelOwner(aLoadingPrincipal,
                                                      *aResult, aURI, PR_FALSE);
  *aForcePrincipalCheckForCacheEntry = setOwner;

  return NS_OK;
}

static PRUint32 SecondsFromPRTime(PRTime prTime)
{
  return PRUint32(PRInt64(prTime) / PRInt64(PR_USEC_PER_SEC));
}

imgCacheEntry::imgCacheEntry(imgRequest *request, bool forcePrincipalCheck)
 : mRequest(request),
   mDataSize(0),
   mTouchedTime(SecondsFromPRTime(PR_Now())),
   mExpiryTime(0),
   mMustValidate(PR_FALSE),
   
   
   mEvicted(PR_TRUE),
   mHasNoProxies(PR_TRUE),
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

void imgCacheEntry::UpdateCache(PRInt32 diff )
{
  
  
  if (!Evicted() && HasNoProxies()) {
    nsCOMPtr<nsIURI> uri;
    mRequest->GetURI(getter_AddRefs(uri));
    imgLoader::CacheEntriesChanged(uri, diff);
  }
}

void imgCacheEntry::SetHasNoProxies(bool hasNoProxies)
{
#if defined(PR_LOGGING)
  nsCOMPtr<nsIURI> uri;
  mRequest->GetURI(getter_AddRefs(uri));
  nsCAutoString spec;
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
 : mDirty(PR_FALSE),
   mSize(0)
{}

void imgCacheQueue::UpdateSize(PRInt32 diff)
{
  mSize += diff;
}

PRUint32 imgCacheQueue::GetSize() const
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
    return nsnull;
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
  mDirty = PR_FALSE;
}

void imgCacheQueue::MarkDirty()
{
  mDirty = PR_TRUE;
}

bool imgCacheQueue::IsDirty()
{
  return mDirty;
}

PRUint32 imgCacheQueue::GetNumElements() const
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
    if (!proxyRequest) return NS_ERROR_OUT_OF_MEMORY;
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

class imgCacheObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
private:
  imgLoader mLoader;
};

NS_IMPL_ISUPPORTS1(imgCacheObserver, nsIObserver)

NS_IMETHODIMP
imgCacheObserver::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aSomeData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    DiscardTracker::DiscardAll();
    mLoader.MinimizeCaches();
  } else if (strcmp(aTopic, "chrome-flush-skin-caches") == 0 ||
             strcmp(aTopic, "chrome-flush-caches") == 0) {
    mLoader.ClearChromeImageCache();
  }
  return NS_OK;
}

class imgCacheExpirationTracker : public nsExpirationTracker<imgCacheEntry, 3>
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
    nsCAutoString spec;
    uri->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(gImgLog, "imgCacheExpirationTracker::NotifyExpired", "entry", spec.get());
  }
#endif

  
  
  if (!entry->Evicted())
    imgLoader::RemoveFromCache(entry);

  imgLoader::VerifyCacheSizes();
}

imgCacheObserver *gCacheObserver;
imgCacheExpirationTracker *gCacheTracker;

imgLoader::imgCacheTable imgLoader::sCache;
imgCacheQueue imgLoader::sCacheQueue;

imgLoader::imgCacheTable imgLoader::sChromeCache;
imgCacheQueue imgLoader::sChromeCacheQueue;

PRFloat64 imgLoader::sCacheTimeWeight;
PRUint32 imgLoader::sCacheMaxSize;

NS_IMPL_ISUPPORTS5(imgLoader, imgILoader, nsIContentSniffer, imgICache, nsISupportsWeakReference, nsIObserver)

imgLoader::imgLoader()
{
  
}

imgLoader::~imgLoader()
{
  
}

void imgLoader::VerifyCacheSizes()
{
#ifdef DEBUG
  if (!gCacheTracker)
    return;

  PRUint32 cachesize = sCache.Count() + sChromeCache.Count();
  PRUint32 queuesize = sCacheQueue.GetNumElements() + sChromeCacheQueue.GetNumElements();
  PRUint32 trackersize = 0;
  for (nsExpirationTracker<imgCacheEntry, 3>::Iterator it(gCacheTracker); it.Next(); )
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
    return sChromeCache;
  else
    return sCache;
}

imgCacheQueue & imgLoader::GetCacheQueue(nsIURI *aURI)
{
  bool chrome = false;
  aURI->SchemeIs("chrome", &chrome);
  if (chrome)
    return sChromeCacheQueue;
  else
    return sCacheQueue;
}

nsresult imgLoader::InitCache()
{
  NS_TIME_FUNCTION;

  nsresult rv;
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os)
    return NS_ERROR_FAILURE;
  
  gCacheObserver = new imgCacheObserver();
  if (!gCacheObserver) 
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(gCacheObserver);

  os->AddObserver(gCacheObserver, "memory-pressure", PR_FALSE);
  os->AddObserver(gCacheObserver, "chrome-flush-skin-caches", PR_FALSE);
  os->AddObserver(gCacheObserver, "chrome-flush-caches", PR_FALSE);

  gCacheTracker = new imgCacheExpirationTracker();
  if (!gCacheTracker)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!sCache.Init())
      return NS_ERROR_OUT_OF_MEMORY;
  if (!sChromeCache.Init())
      return NS_ERROR_OUT_OF_MEMORY;

  PRInt32 timeweight;
  rv = Preferences::GetInt("image.cache.timeweight", &timeweight);
  if (NS_SUCCEEDED(rv))
    sCacheTimeWeight = timeweight / 1000.0;
  else
    sCacheTimeWeight = 0.5;

  PRInt32 cachesize;
  rv = Preferences::GetInt("image.cache.size", &cachesize);
  if (NS_SUCCEEDED(rv))
    sCacheMaxSize = cachesize;
  else
    sCacheMaxSize = 5 * 1024 * 1024;

  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUsedRaw));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUsedUncompressedHeap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUsedUncompressedNonheap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUnusedRaw));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUnusedUncompressedHeap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ChromeUnusedUncompressedNonheap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUsedRaw));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUsedUncompressedHeap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUsedUncompressedNonheap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUnusedRaw));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUnusedUncompressedHeap));
  NS_RegisterMemoryReporter(new imgMemoryReporter(imgMemoryReporter::ContentUnusedUncompressedNonheap));
  
  return NS_OK;
}

nsresult imgLoader::Init()
{
  ReadAcceptHeaderPref();

  Preferences::AddWeakObserver(this, "image.http.accept");

  
  nsCOMPtr<nsIObserverService> obService = mozilla::services::GetObserverService();
  if (obService)
    obService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
imgLoader::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    if (!strcmp(NS_ConvertUTF16toUTF8(aData).get(), "image.http.accept")) {
      ReadAcceptHeaderPref();
    }
  }

  
  else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData))
      ClearImageCache();
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
  nsCAutoString spec;
  imgCacheTable &cache = GetCache(uri);

  uri->GetSpec(spec);
  *_retval = nsnull;

  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    if (gCacheTracker && entry->HasNoProxies())
      gCacheTracker->MarkUsed(entry);

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
  ClearChromeImageCache();
  ClearImageCache();
  NS_IF_RELEASE(gCacheObserver);
  delete gCacheTracker;
  gCacheTracker = nsnull;
}

nsresult imgLoader::ClearChromeImageCache()
{
  return EvictEntries(sChromeCache);
}

nsresult imgLoader::ClearImageCache()
{
  return EvictEntries(sCache);
}

void imgLoader::MinimizeCaches()
{
  EvictEntries(sCacheQueue);
  EvictEntries(sChromeCacheQueue);
}

bool imgLoader::PutIntoCache(nsIURI *key, imgCacheEntry *entry)
{
  imgCacheTable &cache = GetCache(key);

  nsCAutoString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::PutIntoCache", "uri", spec.get());

  
  
  
  nsRefPtr<imgCacheEntry> tmpCacheEntry;
  if (cache.Get(spec, getter_AddRefs(tmpCacheEntry)) && tmpCacheEntry) {
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Element already in the cache", nsnull));
    nsRefPtr<imgRequest> tmpRequest = getter_AddRefs(tmpCacheEntry->GetRequest());

    
    
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Replacing cached element", nsnull));

    RemoveFromCache(key);
  } else {
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::PutIntoCache -- Element NOT already in the cache", nsnull));
  }

  if (!cache.Put(spec, entry))
    return PR_FALSE;

  
  if (entry->Evicted())
    entry->SetEvicted(PR_FALSE);

  
  
  if (entry->HasNoProxies()) {
    nsresult addrv = NS_OK;

    if (gCacheTracker)
      addrv = gCacheTracker->AddObject(entry);

    if (NS_SUCCEEDED(addrv)) {
      imgCacheQueue &queue = GetCacheQueue(key);
      queue.Push(entry);
    }
  }

  nsRefPtr<imgRequest> request(getter_AddRefs(entry->GetRequest()));
  request->SetIsInCache(PR_TRUE);

  return PR_TRUE;
}

bool imgLoader::SetHasNoProxies(nsIURI *key, imgCacheEntry *entry)
{
#if defined(PR_LOGGING)
  nsCAutoString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::SetHasNoProxies", "uri", spec.get());
#endif

  if (entry->Evicted())
    return PR_FALSE;

  imgCacheQueue &queue = GetCacheQueue(key);

  nsresult addrv = NS_OK;

  if (gCacheTracker)
    addrv = gCacheTracker->AddObject(entry);

  if (NS_SUCCEEDED(addrv)) {
    queue.Push(entry);
    entry->SetHasNoProxies(PR_TRUE);
  }

  imgCacheTable &cache = GetCache(key);
  CheckCacheLimits(cache, queue);

  return PR_TRUE;
}

bool imgLoader::SetHasProxies(nsIURI *key)
{
  VerifyCacheSizes();

  imgCacheTable &cache = GetCache(key);

  nsCAutoString spec;
  key->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::SetHasProxies", "uri", spec.get());

  nsRefPtr<imgCacheEntry> entry;
  if (cache.Get(spec, getter_AddRefs(entry)) && entry && entry->HasNoProxies()) {
    imgCacheQueue &queue = GetCacheQueue(key);
    queue.Remove(entry);

    if (gCacheTracker)
      gCacheTracker->RemoveObject(entry);

    entry->SetHasNoProxies(PR_FALSE);

    return PR_TRUE;
  }

  return PR_FALSE;
}

void imgLoader::CacheEntriesChanged(nsIURI *uri, PRInt32 sizediff )
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
      nsCAutoString spec;
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
                                                PRInt32 aCORSMode)
{
  
  
  

  nsresult rv;

  
  
  if (request->mValidator) {
    rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                  aLoadFlags, aExistingRequest, 
                                  reinterpret_cast<imgIRequest **>(aProxyRequest));
    if (NS_FAILED(rv)) {
      return PR_FALSE;
    }

    if (*aProxyRequest) {
      imgRequestProxy* proxy = static_cast<imgRequestProxy*>(*aProxyRequest);

      
      
      
      
      proxy->SetNotificationsDeferred(PR_TRUE);

      
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
      return PR_FALSE;
    }

    nsCOMPtr<imgIRequest> req;
    rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                  aLoadFlags, aExistingRequest, getter_AddRefs(req));
    if (NS_FAILED(rv)) {
      return PR_FALSE;
    }

    
    nsRefPtr<nsProgressNotificationProxy> progressproxy =
        new nsProgressNotificationProxy(newChannel, req);
    if (!progressproxy)
      return PR_FALSE;

    nsRefPtr<imgCacheValidator> hvc =
      new imgCacheValidator(progressproxy, request, aCX, forcePrincipalCheck);

    nsCOMPtr<nsIStreamListener> listener = hvc.get();

    if (aCORSMode != imgIRequest::CORS_NONE) {
      bool withCredentials = aCORSMode == imgIRequest::CORS_USE_CREDENTIALS;
      nsCOMPtr<nsIStreamListener> corsproxy =
        new nsCORSListenerProxy(hvc, aLoadingPrincipal, newChannel, withCredentials, &rv);
      if (NS_FAILED(rv)) {
        return PR_FALSE;
      }

      listener = corsproxy;
    }

    newChannel->SetNotificationCallbacks(hvc);

    request->mValidator = hvc;

    imgRequestProxy* proxy = static_cast<imgRequestProxy*>
                               (static_cast<imgIRequest*>(req.get()));

    
    
    
    
    proxy->SetNotificationsDeferred(PR_TRUE);

    
    hvc->AddProxy(proxy);

    rv = newChannel->AsyncOpen(listener, nsnull);
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
                                PRInt32 aCORSMode)
{
  LOG_SCOPE(gImgLog, "imgLoader::ValidateEntry");

  bool hasExpired;
  PRUint32 expirationTime = aEntry->GetExpiryTime();
  if (expirationTime <= SecondsFromPRTime(PR_Now())) {
    hasExpired = PR_TRUE;
  } else {
    hasExpired = PR_FALSE;
  }

  nsresult rv;

  
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aURI));
  if (fileUrl) {
    PRUint32 lastModTime = aEntry->GetTouchedTime();

    nsCOMPtr<nsIFile> theFile;
    rv = fileUrl->GetFile(getter_AddRefs(theFile));
    if (NS_SUCCEEDED(rv)) {
      PRInt64 fileLastMod;
      rv = theFile->GetLastModifiedTime(&fileLastMod);
      if (NS_SUCCEEDED(rv)) {
        
        fileLastMod *= 1000;
        hasExpired = SecondsFromPRTime((PRTime)fileLastMod) > lastModTime;
      }
    }
  }

  nsRefPtr<imgRequest> request(aEntry->GetRequest());

  if (!request)
    return PR_FALSE;

  if (!ValidateCORSAndPrincipal(request, aEntry->ForcePrincipalCheck(),
                                aCORSMode, aLoadingPrincipal))
    return PR_FALSE;

  bool validateRequest = false;

  
  
  
  
  
  
  void *key = (void *)aCX;
  if (request->mLoadId != key) {
    
    
    if (aLoadFlags & nsIRequest::LOAD_BYPASS_CACHE)
      return PR_FALSE;

    
    validateRequest = ShouldRevalidateEntry(aEntry, aLoadFlags, hasExpired);

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("imgLoader::ValidateEntry validating cache entry. " 
            "validateRequest = %d", validateRequest));
  }
#if defined(PR_LOGGING)
  else if (!key) {
    nsCAutoString spec;
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
    return PR_FALSE;
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
  if (!aKey) return PR_FALSE;

  imgCacheTable &cache = GetCache(aKey);
  imgCacheQueue &queue = GetCacheQueue(aKey);

  nsCAutoString spec;
  aKey->GetSpec(spec);

  LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::RemoveFromCache", "uri", spec.get());

  nsRefPtr<imgCacheEntry> entry;
  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    cache.Remove(spec);

    NS_ABORT_IF_FALSE(!entry->Evicted(), "Evicting an already-evicted cache entry!");

    
    if (entry->HasNoProxies()) {
      if (gCacheTracker)
        gCacheTracker->RemoveObject(entry);
      queue.Remove(entry);
    }

    entry->SetEvicted(PR_TRUE);

    nsRefPtr<imgRequest> request(getter_AddRefs(entry->GetRequest()));
    request->SetIsInCache(PR_FALSE);

    return PR_TRUE;
  }
  else
    return PR_FALSE;
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
      nsCAutoString spec;
      key->GetSpec(spec);

      LOG_STATIC_FUNC_WITH_PARAM(gImgLog, "imgLoader::RemoveFromCache", "entry's uri", spec.get());

      cache.Remove(spec);

      if (entry->HasNoProxies()) {
        LOG_STATIC_FUNC(gImgLog, "imgLoader::RemoveFromCache removing from tracker");
        if (gCacheTracker)
          gCacheTracker->RemoveObject(entry);
        queue.Remove(entry);
      }

      entry->SetEvicted(PR_TRUE);
      request->SetIsInCache(PR_FALSE);

      return PR_TRUE;
    }
  }

  return PR_FALSE;
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

  for (PRUint32 i = 0; i < entries.Length(); ++i)
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

  for (PRUint32 i = 0; i < entries.Length(); ++i)
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

  nsCAutoString spec;
  aURI->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgLoader::LoadImage", "aURI", spec.get());

  *_retval = nsnull;

  nsRefPtr<imgRequest> request;

  nsresult rv;
  nsLoadFlags requestFlags = nsIRequest::LOAD_NORMAL;

  
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

  PRInt32 corsmode = imgIRequest::CORS_NONE;
  if (aLoadFlags & imgILoader::LOAD_CORS_ANONYMOUS) {
    corsmode = imgIRequest::CORS_ANONYMOUS;
  } else if (aLoadFlags & imgILoader::LOAD_CORS_USE_CREDENTIALS) {
    corsmode = imgIRequest::CORS_USE_CREDENTIALS;
  }

  nsRefPtr<imgCacheEntry> entry;

  
  
  
  
  imgCacheTable &cache = GetCache(aURI);

  if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
    if (ValidateEntry(entry, aURI, aInitialDocumentURI, aReferrerURI,
                      aLoadGroup, aObserver, aCX, requestFlags, PR_TRUE,
                      aRequest, _retval, aPolicy, aLoadingPrincipal, corsmode)) {
      request = getter_AddRefs(entry->GetRequest());

      
      if (entry->HasNoProxies()) {
        LOG_FUNC_WITH_PARAM(gImgLog, "imgLoader::LoadImage() adding proxyless entry", "uri", spec.get());
        NS_ABORT_IF_FALSE(!request->HasCacheEntry(), "Proxyless entry's request has cache entry!");
        request->SetCacheEntry(entry);

        if (gCacheTracker)
          gCacheTracker->MarkUsed(entry);
      } 

      entry->Touch();

#ifdef DEBUG_joe
      printf("CACHEGET: %d %s %d\n", time(NULL), spec.get(), entry->GetDataSize());
#endif
    }
    else {
      
      
      entry = nsnull;
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

    NewRequestAndEntry(forcePrincipalCheck, getter_AddRefs(request),
                       getter_AddRefs(entry));

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
      bool withCredentials = corsmode == imgIRequest::CORS_USE_CREDENTIALS;

      nsCOMPtr<nsIStreamListener> corsproxy =
        new nsCORSListenerProxy(pl, aLoadingPrincipal, newChannel,
                                withCredentials, &rv);
      if (NS_FAILED(rv)) {
        return NS_ERROR_FAILURE;
      }

      listener = corsproxy;
    }

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- Calling channel->AsyncOpen()\n", this));

    nsresult openRes = newChannel->AsyncOpen(listener, nsnull);

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
    nsCAutoString spec;

    uri->GetSpec(spec);

    if (cache.Get(spec, getter_AddRefs(entry)) && entry) {
      
      
      
      
      
      
      
      if (ValidateEntry(entry, uri, nsnull, nsnull, nsnull, aObserver, aCX,
                        requestFlags, PR_FALSE, nsnull, nsnull, nsnull,
                        nsnull, imgIRequest::CORS_NONE)) {
        request = getter_AddRefs(entry->GetRequest());
      } else {
        nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(channel));
        bool bUseCacheCopy;

        if (cacheChan)
          cacheChan->IsFromCache(&bUseCacheCopy);
        else
          bUseCacheCopy = PR_FALSE;

        if (!bUseCacheCopy)
          entry = nsnull;
        else {
          request = getter_AddRefs(entry->GetRequest());
        }
      }

      if (request && entry) {
        
        if (entry->HasNoProxies()) {
          LOG_FUNC_WITH_PARAM(gImgLog, "imgLoader::LoadImageWithChannel() adding proxyless entry", "uri", spec.get());
          NS_ABORT_IF_FALSE(!request->HasCacheEntry(), "Proxyless entry's request has cache entry!");
          request->SetCacheEntry(entry);

          if (gCacheTracker)
            gCacheTracker->MarkUsed(entry);
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

    *listener = nsnull; 

    rv = CreateNewProxyForRequest(request, loadGroup, aObserver,
                                  requestFlags, nsnull, _retval);
    static_cast<imgRequestProxy*>(*_retval)->NotifyListener();
  } else {
    
    
    
    NewRequestAndEntry(PR_TRUE, getter_AddRefs(request), getter_AddRefs(entry));

    
    nsCOMPtr<nsIURI> originalURI;
    channel->GetOriginalURI(getter_AddRefs(originalURI));

    
    request->Init(originalURI, uri, channel, channel, entry,
                  aCX, nsnull, imgIRequest::CORS_NONE);

    ProxyListener *pl = new ProxyListener(static_cast<nsIStreamListener *>(request.get()));
    NS_ADDREF(pl);

    *listener = static_cast<nsIStreamListener*>(pl);
    NS_ADDREF(*listener);

    NS_RELEASE(pl);

    
    PutIntoCache(originalURI, entry);

    rv = CreateNewProxyForRequest(request, loadGroup, aObserver,
                                  requestFlags, nsnull, _retval);

    
    
    
    
    
    
  }

  return rv;
}

NS_IMETHODIMP imgLoader::SupportImageWithMimeType(const char* aMimeType, bool *_retval)
{
  *_retval = PR_FALSE;
  nsCAutoString mimeType(aMimeType);
  ToLowerCase(mimeType);
  *_retval = (Image::GetDecoderType(mimeType.get()) == Image::eDecoderType_unknown)
    ? PR_FALSE : PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP imgLoader::GetMIMETypeFromContent(nsIRequest* aRequest,
                                                const PRUint8* aContents,
                                                PRUint32 aLength,
                                                nsACString& aContentType)
{
  return GetMimeTypeFromContent((const char*)aContents, aLength, aContentType);
}


nsresult imgLoader::GetMimeTypeFromContent(const char* aContents, PRUint32 aLength, nsACString& aContentType)
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
    nsCAutoString contentType;
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
                                          nsnull,
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




NS_IMETHODIMP ProxyListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}





NS_IMPL_ISUPPORTS5(imgCacheValidator, nsIStreamListener, nsIRequestObserver,
                   nsIChannelEventSink, nsIInterfaceRequestor,
                   nsIAsyncVerifyRedirectCallback)

imgLoader imgCacheValidator::sImgLoader;

imgCacheValidator::imgCacheValidator(nsProgressNotificationProxy* progress,
                                     imgRequest *request, void *aContext,
                                     bool forcePrincipalCheckForCacheEntry)
 : mProgressProxy(progress),
   mRequest(request),
   mContext(aContext)
{
  NewRequestAndEntry(forcePrincipalCheckForCacheEntry,
                     getter_AddRefs(mNewRequest), getter_AddRefs(mNewEntry));
}

imgCacheValidator::~imgCacheValidator()
{
  if (mRequest) {
    mRequest->mValidator = nsnull;
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
      PRUint32 count = mProxies.Count();
      for (PRInt32 i = count-1; i>=0; i--) {
        imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);

        
        
        NS_ABORT_IF_FALSE(proxy->NotificationsDeferred(),
                          "Proxies waiting on cache validation should be "
                          "deferring notifications!");
        proxy->SetNotificationsDeferred(PR_FALSE);

        
        
        proxy->SyncNotifyListener();
      }

      
      aRequest->Cancel(NS_BINDING_ABORTED);

      mRequest->SetLoadId(mContext);
      mRequest->mValidator = nsnull;

      mRequest = nsnull;

      mNewRequest = nsnull;
      mNewEntry = nsnull;

      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIURI> uri;
  mRequest->GetURI(getter_AddRefs(uri));

#if defined(PR_LOGGING)
  nsCAutoString spec;
  uri->GetSpec(spec);
  LOG_MSG_WITH_PARAM(gImgLog, "imgCacheValidator::OnStartRequest creating new request", "uri", spec.get());
#endif

  PRInt32 corsmode = mRequest->GetCORSMode();
  nsCOMPtr<nsIPrincipal> loadingPrincipal = mRequest->GetLoadingPrincipal();

  
  mRequest->RemoveFromCache();

  mRequest->mValidator = nsnull;
  mRequest = nsnull;

  
  nsCOMPtr<nsIURI> originalURI;
  channel->GetOriginalURI(getter_AddRefs(originalURI));
  mNewRequest->Init(originalURI, uri, channel, channel, mNewEntry,
                    mContext, loadingPrincipal,
                    corsmode);

  mDestListener = new ProxyListener(mNewRequest);

  
  
  
  sImgLoader.PutIntoCache(originalURI, mNewEntry);

  PRUint32 count = mProxies.Count();
  for (PRInt32 i = count-1; i>=0; i--) {
    imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);
    proxy->ChangeOwner(mNewRequest);

    
    
    NS_ABORT_IF_FALSE(proxy->NotificationsDeferred(),
                      "Proxies waiting on cache validation should be "
                      "deferring notifications!");
    proxy->SetNotificationsDeferred(PR_FALSE);

    
    
    proxy->SyncNotifyListener();
  }

  mNewRequest = nsnull;
  mNewEntry = nsnull;

  return mDestListener->OnStartRequest(aRequest, ctxt);
}


NS_IMETHODIMP imgCacheValidator::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  if (!mDestListener)
    return NS_OK;

  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}





NS_IMETHODIMP imgCacheValidator::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  if (!mDestListener) {
    
    PRUint32 _retval;
    inStr->ReadSegments(NS_DiscardSegment, nsnull, count, &_retval);
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
                                                        nsIChannel *newChannel, PRUint32 flags,
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
      mRedirectCallback = nsnull;
      mRedirectChannel = nsnull;
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
  mRedirectCallback = nsnull;
  mRedirectChannel = nsnull;
  return NS_OK;
}
