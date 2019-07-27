









#ifndef mozilla_image_src_SurfaceCache_h
#define mozilla_image_src_SurfaceCache_h

#include "mozilla/Maybe.h"           
#include "mozilla/MemoryReporting.h" 
#include "mozilla/HashFunctions.h"   
#include "gfx2DGlue.h"               
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
  Maybe<SVGImageContext> SVGContext() const { return mSVGContext; }
  float AnimationTime() const { return mAnimationTime; }
  uint32_t Flags() const { return mFlags; }

  SurfaceKey WithNewFlags(uint32_t aFlags) const
  {
    return SurfaceKey(mSize, mSVGContext, mAnimationTime, aFlags);
  }

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

  friend SurfaceKey RasterSurfaceKey(const IntSize&, uint32_t, uint32_t);
  friend SurfaceKey VectorSurfaceKey(const IntSize&,
                                     const Maybe<SVGImageContext>&,
                                     float);

  IntSize                mSize;
  Maybe<SVGImageContext> mSVGContext;
  float                  mAnimationTime;
  uint32_t               mFlags;
};

inline SurfaceKey
RasterSurfaceKey(const gfx::IntSize& aSize,
                 uint32_t aFlags,
                 uint32_t aFrameNum)
{
  return SurfaceKey(aSize, Nothing(), float(aFrameNum), aFlags);
}

inline SurfaceKey
VectorSurfaceKey(const gfx::IntSize& aSize,
                 const Maybe<SVGImageContext>& aSVGContext,
                 float aAnimationTime)
{
  
  
  
  return SurfaceKey(aSize, aSVGContext, aAnimationTime, 0);
}

enum class Lifetime : uint8_t {
  Transient,
  Persistent
};

enum class InsertOutcome : uint8_t {
  SUCCESS,                 
  FAILURE,                 
  FAILURE_ALREADY_PRESENT  
};























struct SurfaceCache
{
  typedef gfx::IntSize IntSize;

  


  static void Initialize();

  


  static void Shutdown();

  




























  static DrawableFrameRef Lookup(const ImageKey    aImageKey,
                                 const SurfaceKey& aSurfaceKey,
                                 const Maybe<uint32_t>& aAlternateFlags
                                   = Nothing());

  



















  static DrawableFrameRef LookupBestMatch(const ImageKey    aImageKey,
                                          const SurfaceKey& aSurfaceKey,
                                          const Maybe<uint32_t>& aAlternateFlags
                                            = Nothing());

  















































  static InsertOutcome Insert(imgFrame*         aSurface,
                              const ImageKey    aImageKey,
                              const SurfaceKey& aSurfaceKey,
                              Lifetime          aLifetime);

  













  static bool CanHold(const IntSize& aSize);
  static bool CanHold(size_t aSize);

  



























  static void LockImage(const ImageKey aImageKey);

  







  static void UnlockImage(const ImageKey aImageKey);

  


















  static void UnlockSurfaces(const ImageKey aImageKey);

  












  static void RemoveSurface(const ImageKey    aImageKey,
                            const SurfaceKey& aSurfaceKey);

  









  static void RemoveImage(const ImageKey aImageKey);

  






  static void DiscardAll();

  












  static size_t SizeOfSurfaces(const ImageKey    aImageKey,
                               gfxMemoryLocation aLocation,
                               MallocSizeOf      aMallocSizeOf);

private:
  virtual ~SurfaceCache() = 0;  
};

} 
} 

#endif
