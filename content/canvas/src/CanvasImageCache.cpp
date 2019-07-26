




#include "CanvasImageCache.h"
#include "nsIImageLoadingContent.h"
#include "nsExpirationTracker.h"
#include "imgIRequest.h"
#include "gfxASurface.h"
#include "gfxPoint.h"
#include "mozilla/dom/Element.h"
#include "nsTHashtable.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

namespace mozilla {

using namespace dom;

struct ImageCacheKey {
  ImageCacheKey(Element* aImage, HTMLCanvasElement* aCanvas)
    : mImage(aImage), mCanvas(aCanvas) {}
  Element* mImage;
  HTMLCanvasElement* mCanvas;
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
    , mILC(nullptr)
    , mCanvas(aKey.mCanvas)
  {}

  nsExpirationState* GetExpirationState() { return &mState; }

  size_t SizeInBytes() { return mSize.width * mSize.height * 4; }

  
  nsRefPtr<Element> mImage;
  nsIImageLoadingContent* mILC;
  nsRefPtr<HTMLCanvasElement> mCanvas;
  
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

  bool KeyEquals(KeyTypePointer key) const
  {
    return mData->mImage == key->mImage && mData->mCanvas == key->mCanvas;
  }

  static KeyTypePointer KeyToPointer(KeyType& key) { return &key; }
  static PLDHashNumber HashKey(KeyTypePointer key)
  {
    return HashGeneric(key->mImage, key->mCanvas);
  }
  enum { ALLOW_MEMMOVE = true };

  nsAutoPtr<ImageCacheEntryData> mData;
};

static bool sPrefsInitialized = false;
static int32_t sCanvasImageCacheLimit = 0;

class ImageCache MOZ_FINAL : public nsExpirationTracker<ImageCacheEntryData,4>,
                             public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  enum { GENERATION_MS = 1000 };
  ImageCache()
    : nsExpirationTracker<ImageCacheEntryData,4>(GENERATION_MS)
    , mSize(0)
  {
    if (!sPrefsInitialized) {
      sPrefsInitialized = true;
      Preferences::AddIntVarCache(&sCanvasImageCacheLimit, "canvas.image.cache.limit", 0);
    }
    mCache.Init();
  }
  ~ImageCache() {
    AgeAllGenerations();
  }

  void AddObject(ImageCacheEntryData* aObject)
  {
    nsExpirationTracker<ImageCacheEntryData,4>::AddObject(aObject);
    mSize += aObject->SizeInBytes();
  }

  void RemoveObject(ImageCacheEntryData* aObject)
  {
    nsExpirationTracker<ImageCacheEntryData,4>::RemoveObject(aObject);
    mSize -= aObject->SizeInBytes();
  }

  virtual void NotifyExpired(ImageCacheEntryData* aObject)
  {
    RemoveObject(aObject);
    
    mCache.RemoveEntry(ImageCacheKey(aObject->mImage, aObject->mCanvas));
  }

  nsTHashtable<ImageCacheEntry> mCache;
  size_t mSize;
};

static ImageCache* gImageCache = nullptr;

NS_IMPL_ISUPPORTS1(ImageCache, nsIObserver)

NS_IMETHODIMP
ImageCache::Observe(nsISupports *aSubject,
                    const char *aTopic,
                    const PRUnichar *aData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    AgeAllGenerations();
  }
  return NS_OK;
}

class CanvasImageCacheShutdownObserver MOZ_FINAL : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

void
CanvasImageCache::NotifyDrawImage(Element* aImage,
                                  HTMLCanvasElement* aCanvas,
                                  imgIRequest* aRequest,
                                  gfxASurface* aSurface,
                                  const gfxIntSize& aSize)
{
  if (!gImageCache) {
    gImageCache = new ImageCache();
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->AddObserver(gImageCache, "memory-pressure", false);
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

  if (!sCanvasImageCacheLimit)
    return;

  
  while (gImageCache->mSize > size_t(sCanvasImageCacheLimit))
    gImageCache->AgeOneGeneration();
}

gfxASurface*
CanvasImageCache::Lookup(Element* aImage,
                         HTMLCanvasElement* aCanvas,
                         gfxIntSize* aSize)
{
  if (!gImageCache)
    return nullptr;

  ImageCacheEntry* entry = gImageCache->mCache.GetEntry(ImageCacheKey(aImage, aCanvas));
  if (!entry || !entry->mData->mILC)
    return nullptr;

  nsCOMPtr<imgIRequest> request;
  entry->mData->mILC->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST, getter_AddRefs(request));
  if (request != entry->mData->mRequest)
    return nullptr;

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
    gImageCache = nullptr;

    nsContentUtils::UnregisterShutdownObserver(this);
  }

  return NS_OK;
}

} 
