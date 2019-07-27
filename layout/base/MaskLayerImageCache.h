




#ifndef MASKLAYERIMAGECACHE_H_
#define MASKLAYERIMAGECACHE_H_

#include "DisplayItemClip.h"
#include "nsPresContext.h"
#include "mozilla/gfx/Matrix.h"

namespace mozilla {

namespace layers {
class ImageContainer;
}














class MaskLayerImageCache
{
  typedef mozilla::layers::ImageContainer ImageContainer;
public:
  MaskLayerImageCache();
  ~MaskLayerImageCache();

  





  struct PixelRoundedRect
  {
    PixelRoundedRect(const DisplayItemClip::RoundedRect& aRRect,
                     nsPresContext* aPresContext)
      : mRect(aPresContext->AppUnitsToGfxUnits(aRRect.mRect.x),
              aPresContext->AppUnitsToGfxUnits(aRRect.mRect.y),
              aPresContext->AppUnitsToGfxUnits(aRRect.mRect.width),
              aPresContext->AppUnitsToGfxUnits(aRRect.mRect.height))
    {
      MOZ_COUNT_CTOR(PixelRoundedRect);
      NS_FOR_CSS_HALF_CORNERS(corner) {
        mRadii[corner] = aPresContext->AppUnitsToGfxUnits(aRRect.mRadii[corner]);
      }
    }
    PixelRoundedRect(const PixelRoundedRect& aPRR)
      : mRect(aPRR.mRect)
    {
      MOZ_COUNT_CTOR(PixelRoundedRect);
      NS_FOR_CSS_HALF_CORNERS(corner) {
        mRadii[corner] = aPRR.mRadii[corner];
      }
    }

    ~PixelRoundedRect()
    {
      MOZ_COUNT_DTOR(PixelRoundedRect);
    }

    
    
    
    void ScaleAndTranslate(const gfx::Matrix& aTransform)
    {
      NS_ASSERTION(aTransform._12 == 0 && aTransform._21 == 0,
                   "Transform has a component other than scale and translate");

      mRect = aTransform.TransformBounds(mRect);

      for (size_t i = 0; i < ArrayLength(mRadii); i += 2) {
        mRadii[i] *= aTransform._11;
        mRadii[i + 1] *= aTransform._22;
      }
    }

    bool operator==(const PixelRoundedRect& aOther) const {
      if (!mRect.IsEqualInterior(aOther.mRect)) {
        return false;
      }

      NS_FOR_CSS_HALF_CORNERS(corner) {
        if (mRadii[corner] != aOther.mRadii[corner]) {
          return false;
        }
      }
      return true;
    }
    bool operator!=(const PixelRoundedRect& aOther) const {
      return !(*this == aOther);
    }

    
    PLDHashNumber Hash() const
    {
      PLDHashNumber hash = HashBytes(&mRect.x, 4*sizeof(gfxFloat));
      hash = AddToHash(hash, HashBytes(mRadii, 8*sizeof(gfxFloat)));

      return hash;
    }

    gfx::Rect mRect;
    
    gfxFloat mRadii[8];

  private:
    PixelRoundedRect() MOZ_DELETE;
  };

  









  struct MaskLayerImageKey
  {
    MaskLayerImageKey();
    MaskLayerImageKey(const MaskLayerImageKey& aKey);

    ~MaskLayerImageKey();

    void AddRef() const { ++mLayerCount; }
    void Release() const
    {
      NS_ASSERTION(mLayerCount > 0, "Inconsistent layer count");
      --mLayerCount;
    }

    PLDHashNumber Hash() const
    {
      PLDHashNumber hash = 0;

      for (uint32_t i = 0; i < mRoundedClipRects.Length(); ++i) {
        hash = AddToHash(hash, mRoundedClipRects[i].Hash());
      }

      return hash;
    }

    bool operator==(const MaskLayerImageKey& aOther) const
    {
      return mRoundedClipRects == aOther.mRoundedClipRects;
    }

    mutable uint32_t mLayerCount;
    nsTArray<PixelRoundedRect> mRoundedClipRects;
  };

  
  
  
  
  ImageContainer* FindImageFor(const MaskLayerImageKey** aKey);

  
  
  
  void PutImage(const MaskLayerImageKey* aKey,
                ImageContainer* aContainer);

  
  void Sweep();

protected:

  class MaskLayerImageEntry : public PLDHashEntryHdr
  {
  public:
    typedef const MaskLayerImageKey& KeyType;
    typedef const MaskLayerImageKey* KeyTypePointer;

    explicit MaskLayerImageEntry(KeyTypePointer aKey)
      : mKey(aKey)
    {
      MOZ_COUNT_CTOR(MaskLayerImageEntry);
    }
    MaskLayerImageEntry(const MaskLayerImageEntry& aOther)
      : mKey(aOther.mKey.get())
    {
      NS_ERROR("ALLOW_MEMMOVE == true, should never be called");
    }
    ~MaskLayerImageEntry()
    {
      MOZ_COUNT_DTOR(MaskLayerImageEntry);
    }

    
    bool KeyEquals(KeyTypePointer aKey) const
    {
      return *mKey == *aKey;
    }

    
    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

    
    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return aKey->Hash();
    }

    
    
    enum { ALLOW_MEMMOVE = true };

    bool operator==(const MaskLayerImageEntry& aOther) const
    {
      return KeyEquals(aOther.mKey);
    }

    nsAutoPtr<const MaskLayerImageKey> mKey;
    nsRefPtr<ImageContainer> mContainer;
  };

  nsTHashtable<MaskLayerImageEntry> mMaskImageContainers;

  
  static PLDHashOperator SweepFunc(MaskLayerImageEntry* aEntry, void* aUserArg);
};


}


#endif
