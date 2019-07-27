









#ifndef MOZILLA_IMAGELIB_SURFACECACHE_H_
#define MOZILLA_IMAGELIB_SURFACECACHE_H_

#include "mozilla/Maybe.h"          
#include "mozilla/HashFunctions.h"  
#include "gfxPoint.h"               
#include "nsCOMPtr.h"               
#include "mozilla/gfx/Point.h"      
#include "mozilla/gfx/2D.h"         
#include "SVGImageContext.h"        

namespace mozilla {
namespace image {

class DrawableFrameRef;
class Image;
class imgFrame;





typedef Image* ImageKey;








class SurfaceKey
{
  typedef gfx::IntSize IntSize;

public:
  bool operator==(const SurfaceKey& aOther) const
  {
    return aOther.mSize == mSize &&
           aOther.mSVGContext == mSVGContext &&
           aOther.mAnimationTime == mAnimationTime &&
           aOther.mFlags == mFlags;
  }

  uint32_t Hash() const
  {
    uint32_t hash = HashGeneric(mSize.width, mSize.height);
    hash = AddToHash(hash, mSVGContext.map(HashSIC).valueOr(0));
    hash = AddToHash(hash, mAnimationTime, mFlags);
    return hash;
  }

  IntSize Size() const { return mSize; }

private:
  SurfaceKey(const IntSize& aSize,
             const Maybe<SVGImageContext>& aSVGContext,
             const float aAnimationTime,
             const uint32_t aFlags)
    : mSize(aSize)
    , mSVGContext(aSVGContext)
    , mAnimationTime(aAnimationTime)
    , mFlags(aFlags)
  { }

  static uint32_t HashSIC(const SVGImageContext& aSIC) {
    return aSIC.Hash();
  }

  friend SurfaceKey RasterSurfaceKey(const IntSize&, const uint32_t);
  friend SurfaceKey VectorSurfaceKey(const IntSize&,
                                     const Maybe<SVGImageContext>&,
                                     const float);

  IntSize                mSize;
  Maybe<SVGImageContext> mSVGContext;
  float                  mAnimationTime;
  uint32_t               mFlags;
};

inline SurfaceKey
RasterSurfaceKey(const gfx::IntSize& aSize,
                 const uint32_t aFlags)
{
  
  
  
  return SurfaceKey(aSize, Nothing(), 0.0f, aFlags);
}

inline SurfaceKey
VectorSurfaceKey(const gfx::IntSize& aSize,
                 const Maybe<SVGImageContext>& aSVGContext,
                 const float aAnimationTime)
{
  
  
  
  return SurfaceKey(aSize, aSVGContext, aAnimationTime, 0);
}














struct SurfaceCache
{
  typedef gfx::IntSize IntSize;

  


  static void Initialize();

  


  static void Shutdown();

  













  static DrawableFrameRef Lookup(const ImageKey    aImageKey,
                                 const SurfaceKey& aSurfaceKey);

  









  static void Insert(imgFrame*         aSurface,
                     const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey);

  













  static bool CanHold(const IntSize& aSize);

  







  static void Discard(const ImageKey aImageKey);

  


  static void DiscardAll();

private:
  virtual ~SurfaceCache() = 0;  
};

} 
} 

#endif 
