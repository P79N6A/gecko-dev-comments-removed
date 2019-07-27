




#include "MaskLayerImageCache.h"
#include "ImageContainer.h"

using namespace mozilla::layers;

namespace mozilla {

MaskLayerImageCache::MaskLayerImageCache()
{
  MOZ_COUNT_CTOR(MaskLayerImageCache);
}
MaskLayerImageCache::~MaskLayerImageCache()
{
  MOZ_COUNT_DTOR(MaskLayerImageCache);
}


 PLDHashOperator
MaskLayerImageCache::SweepFunc(MaskLayerImageEntry* aEntry,
                               void* aUserArg)
{
  const MaskLayerImageCache::MaskLayerImageKey* key = aEntry->mKey;

  if (key->mLayerCount == 0) {
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

void
MaskLayerImageCache::Sweep() 
{
  mMaskImageContainers.EnumerateEntries(SweepFunc, nullptr);
}

ImageContainer*
MaskLayerImageCache::FindImageFor(const MaskLayerImageKey** aKey)
{
  if (MaskLayerImageEntry* entry = mMaskImageContainers.GetEntry(**aKey)) {
    *aKey = entry->mKey.get();
    return entry->mContainer;
  }

  return nullptr;
}

void
MaskLayerImageCache::PutImage(const MaskLayerImageKey* aKey, ImageContainer* aContainer)
{
  MaskLayerImageEntry* entry = mMaskImageContainers.PutEntry(*aKey);
  entry->mContainer = aContainer;
}

MaskLayerImageCache::MaskLayerImageKey::MaskLayerImageKey()
  : mLayerCount(0)
  , mRoundedClipRects()
{
  MOZ_COUNT_CTOR(MaskLayerImageKey);
}

MaskLayerImageCache::MaskLayerImageKey::MaskLayerImageKey(const MaskLayerImageKey& aKey)
  : mLayerCount(aKey.mLayerCount)
  , mRoundedClipRects(aKey.mRoundedClipRects)
{
  MOZ_COUNT_CTOR(MaskLayerImageKey);
}

MaskLayerImageCache::MaskLayerImageKey::~MaskLayerImageKey()
{
  MOZ_COUNT_DTOR(MaskLayerImageKey);
}

}
