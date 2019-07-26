








#ifndef MOZILLA_IMAGELIB_SURFACECACHE_H_
#define MOZILLA_IMAGELIB_SURFACECACHE_H_

#include "mozilla/HashFunctions.h"  
#include "gfxPoint.h"               
#include "nsCOMPtr.h"               
#include "nsSize.h"                 
#include "SVGImageContext.h"        

class gfxDrawable;

namespace mozilla {

namespace gfx {
class DrawTarget;
} 

namespace image {

class Image;





typedef Image* ImageKey;








class SurfaceKey
{
public:
  SurfaceKey(const nsIntSize aSize,
             const gfxSize aScale,
             const SVGImageContext* aSVGContext,
             const float aAnimationTime,
             const uint32_t aFlags)
    : mSize(aSize)
    , mScale(aScale)
    , mSVGContextIsValid(aSVGContext != nullptr)
    , mAnimationTime(aAnimationTime)
    , mFlags(aFlags)
  {
    
    if (mSVGContextIsValid)
      mSVGContext = *aSVGContext;
  }

  bool operator==(const SurfaceKey& aOther) const
  {
    bool matchesSVGContext = aOther.mSVGContextIsValid == mSVGContextIsValid &&
                             (!mSVGContextIsValid || aOther.mSVGContext == mSVGContext);
    return aOther.mSize == mSize &&
           aOther.mScale == mScale &&
           matchesSVGContext &&
           aOther.mAnimationTime == mAnimationTime &&
           aOther.mFlags == mFlags;
  }

  uint32_t Hash() const
  {
    uint32_t hash = HashGeneric(mSize.width, mSize.height);
    hash = AddToHash(hash, mScale.width, mScale.height);
    hash = AddToHash(hash, mSVGContextIsValid, mSVGContext.Hash());
    hash = AddToHash(hash, mAnimationTime, mFlags);
    return hash;
  }

  nsIntSize Size() const { return mSize; }

private:
  nsIntSize       mSize;
  gfxSize         mScale;
  SVGImageContext mSVGContext;
  bool            mSVGContextIsValid;
  float           mAnimationTime;
  uint32_t        mFlags;
};









struct SurfaceCache
{
  


  static void Initialize();

  


  static void Shutdown();

  







  static already_AddRefed<gfxDrawable> Lookup(const ImageKey    aImageKey,
                                              const SurfaceKey& aSurfaceKey);

  









  static void Insert(mozilla::gfx::DrawTarget* aTarget,
                     const ImageKey            aImageKey,
                     const SurfaceKey&         aSurfaceKey);

  













  static bool CanHold(const nsIntSize& aSize);

  







  static void Discard(const ImageKey aImageKey);

private:
  virtual ~SurfaceCache() = 0;  
};

} 
} 

#endif 
