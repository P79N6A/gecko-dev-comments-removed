




































#include "CanvasImageCache.h"
#include "nsIImageLoadingContent.h"
#include "nsExpirationTracker.h"
#include "imgIRequest.h"
#include "gfxASurface.h"
#include "gfxPoint.h"
#include "nsIDOMElement.h"
#include "nsTHashtable.h"
#include "nsHTMLCanvasElement.h"

namespace mozilla {

struct ImageCacheKey {
  ImageCacheKey(nsIDOMElement* aImage, nsHTMLCanvasElement* aCanvas)
    : mImage(aImage), mCanvas(aCanvas) {}
  nsIDOMElement* mImage;
  nsHTMLCanvasElement* mCanvas;
};

struct ImageCacheEntryData {
  ImageCacheEntryData(const ImageCacheEntryData& aOther)
    : mImage(aOther.mImage)
    , mILC(aOther.mILC)
    , mCanvas(aOther.mCanvas)
    , mRequest(aOther.mRequest)
    , mSurface(aOther.mSurface)
    , mSize(aOther.mSize)
  {}
  ImageCacheEntryData(const ImageCacheKey& aKey)
    : mImage(aKey.mImage)
    , mILC(nsnull)
    , mCanvas(aKey.mCanvas)
  {}

  nsExpirationState* GetExpirationState() { return &mState; }

  
  nsCOMPtr<nsIDOMElement> mImage;
  nsIImageLoadingContent* mILC;
  nsRefPtr<nsHTMLCanvasElement> mCanvas;
  
  nsCOMPtr<imgIRequest> mRequest;
  nsRefPtr<gfxASurface> mSurface;
  gfxIntSize mSize;
  nsExpirationState mState;
};

class ImageCacheEntry : public PLDHashEntryHdr {
public:
  typedef ImageCacheKey KeyType;
  typedef const ImageCacheKey* KeyTypePointer;

  ImageCacheEntry(const KeyType *key) :
      mData(new ImageCacheEntryData(*key)) {}
  ImageCacheEntry(const ImageCacheEntry &toCopy) :
      mData(new ImageCacheEntryData(*toCopy.mData)) {}
  ~ImageCacheEntry() {}

  PRBool KeyEquals(KeyTypePointer key) const
  {
    return mData->mImage == key->mImage && mData->mCanvas == key->mCanvas;
  }

  static KeyTypePointer KeyToPointer(KeyType& key) { return &key; }
  static PLDHashNumber HashKey(KeyTypePointer key)
  {
    return (NS_PTR_TO_INT32(key->mImage) ^ NS_PTR_TO_INT32(key->mCanvas)) >> 2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

  nsAutoPtr<ImageCacheEntryData> mData;
};

class ImageCache : public nsExpirationTracker<ImageCacheEntryData,4> {
public:
  
  enum { GENERATION_MS = 1000 };
  ImageCache()
    : nsExpirationTracker<ImageCacheEntryData,4>(GENERATION_MS)
  {
    mCache.Init();
  }
  ~ImageCache() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(ImageCacheEntryData* aObject)
  {
    RemoveObject(aObject);
    
    mCache.RemoveEntry(ImageCacheKey(aObject->mImage, aObject->mCanvas));
  }

  nsTHashtable<ImageCacheEntry> mCache;
};

static ImageCache* gImageCache = nsnull;

class CanvasImageCacheShutdownObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

void
CanvasImageCache::NotifyDrawImage(nsIDOMElement* aImage,
                                  nsHTMLCanvasElement* aCanvas,
                                  imgIRequest* aRequest,
                                  gfxASurface* aSurface,
                                  const gfxIntSize& aSize)
{
  if (!gImageCache) {
    gImageCache = new ImageCache();
    nsContentUtils::RegisterShutdownObserver(new CanvasImageCacheShutdownObserver());
  }

  ImageCacheEntry* entry = gImageCache->mCache.PutEntry(ImageCacheKey(aImage, aCanvas));
  if (entry) {
    if (entry->mData->mSurface) {
      
      gImageCache->RemoveObject(entry->mData);
    }
    gImageCache->AddObject(entry->mData);

    nsCOMPtr<nsIImageLoadingContent> ilc = do_QueryInterface(aImage);
    if (ilc) {
      ilc->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                      getter_AddRefs(entry->mData->mRequest));
    }
    entry->mData->mILC = ilc;
    entry->mData->mSurface = aSurface;
    entry->mData->mSize = aSize;
  }
}

gfxASurface*
CanvasImageCache::Lookup(nsIDOMElement* aImage,
                         nsHTMLCanvasElement* aCanvas,
                         gfxIntSize* aSize)
{
  if (!gImageCache)
    return nsnull;

  ImageCacheEntry* entry = gImageCache->mCache.GetEntry(ImageCacheKey(aImage, aCanvas));
  if (!entry || !entry->mData->mILC)
    return nsnull;

  nsCOMPtr<imgIRequest> request;
  entry->mData->mILC->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST, getter_AddRefs(request));
  if (request != entry->mData->mRequest)
    return nsnull;

  gImageCache->MarkUsed(entry->mData);

  *aSize = entry->mData->mSize;
  return entry->mData->mSurface;
}

NS_IMPL_ISUPPORTS1(CanvasImageCacheShutdownObserver, nsIObserver)

NS_IMETHODIMP
CanvasImageCacheShutdownObserver::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    delete gImageCache;
    gImageCache = nsnull;

    nsContentUtils::UnregisterShutdownObserver(this);
  }

  return NS_OK;
}

}
