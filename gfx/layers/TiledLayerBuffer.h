



#ifndef GFX_TILEDLAYERBUFFER_H
#define GFX_TILEDLAYERBUFFER_H






#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxPlatform.h"                
#include "LayersLogging.h"              
#include "mozilla/gfx/Logging.h"        
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif

namespace mozilla {

struct TileUnit {};
template<> struct IsPixel<TileUnit> : mozilla::TrueType {};

namespace layers {



#define ENABLE_TILING_LOG 0

#if ENABLE_TILING_LOG
#  define TILING_LOG(...) printf_stderr(__VA_ARGS__);
#else
#  define TILING_LOG(...)
#endif



static inline int floor_div(int a, int b)
{
  int rem = a % b;
  int div = a/b;
  if (rem == 0) {
    return div;
  } else {
    
    int sub;
    sub = a ^ b;
    
    sub >>= 8*sizeof(int)-1;
    return div+sub;
  }
}





















































typedef gfx::IntSizeTyped<TileUnit> TileIntSize;
typedef gfx::IntPointTyped<TileUnit> TileIntPoint;










struct TilesPlacement {
  
  TileIntPoint mFirst;
  TileIntSize mSize;

  TilesPlacement(int aFirstX, int aFirstY,
                 int aRetainedWidth, int aRetainedHeight)
  : mFirst(aFirstX, aFirstY)
  , mSize(aRetainedWidth, aRetainedHeight)
  {}

  int TileIndex(TileIntPoint aPosition) const {
    return (aPosition.x - mFirst.x) * mSize.height + aPosition.y - mFirst.y;
  }

  TileIntPoint TilePosition(size_t aIndex) const {
    return TileIntPoint(
      mFirst.x + aIndex / mSize.height,
      mFirst.y + aIndex % mSize.height
    );
  }

  bool HasTile(TileIntPoint aPosition) const {
    return aPosition.x >= mFirst.x && aPosition.x < mFirst.x + mSize.width &&
           aPosition.y >= mFirst.y && aPosition.y < mFirst.y + mSize.height;
  }
};



inline int GetTileStart(int i, int aTileLength) {
  return (i >= 0) ? (i % aTileLength)
                  : ((aTileLength - (-i % aTileLength)) %
                     aTileLength);
}


inline int RoundDownToTileEdge(int aX, int aTileLength) { return aX - GetTileStart(aX, aTileLength); }

template<typename Derived, typename Tile>
class TiledLayerBuffer
{
public:
  TiledLayerBuffer()
    : mTiles(0, 0, 0, 0)
    , mResolution(1)
    , mTileSize(gfxPlatform::GetPlatform()->GetTileWidth(),
                gfxPlatform::GetPlatform()->GetTileHeight())
  {}

  ~TiledLayerBuffer() {}

  gfx::IntPoint GetTileOffset(TileIntPoint aPosition) const {
    gfx::IntSize scaledTileSize = GetScaledTileSize();
    return gfx::IntPoint(aPosition.x * scaledTileSize.width,
                         aPosition.y * scaledTileSize.height);
  }

  const TilesPlacement& GetPlacement() const { return mTiles; }

  const gfx::IntSize& GetTileSize() const { return mTileSize; }

  gfx::IntSize GetScaledTileSize() const { return RoundedToInt(gfx::Size(mTileSize) / mResolution); }

  unsigned int GetTileCount() const { return mRetainedTiles.Length(); }

  Tile& GetTile(size_t i) { return mRetainedTiles[i]; }

  const nsIntRegion& GetValidRegion() const { return mValidRegion; }
  const nsIntRegion& GetPaintedRegion() const { return mPaintedRegion; }
  void ClearPaintedRegion() { mPaintedRegion.SetEmpty(); }

  void ResetPaintedAndValidState() {
    mPaintedRegion.SetEmpty();
    mValidRegion.SetEmpty();
    mTiles.mSize.width = 0;
    mTiles.mSize.height = 0;
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (!mRetainedTiles[i].IsPlaceholderTile()) {
        AsDerived().ReleaseTile(mRetainedTiles[i]);
      }
    }
    mRetainedTiles.Clear();
  }

  
  
  
  
  
  float GetResolution() const { return mResolution; }
  bool IsLowPrecision() const { return mResolution < 1; }

  typedef Tile* Iterator;
  Iterator TilesBegin() { return mRetainedTiles.Elements(); }
  Iterator TilesEnd() { return mRetainedTiles.Elements() + mRetainedTiles.Length(); }

  void Dump(std::stringstream& aStream, const char* aPrefix, bool aDumpHtml);

protected:

  
  
  Tile mPlaceHolderTile;

  nsIntRegion     mValidRegion;
  nsIntRegion     mPaintedRegion;

  







  nsTArray<Tile>  mRetainedTiles;
  TilesPlacement  mTiles;
  float           mResolution;
  gfx::IntSize    mTileSize;

private:
  const Derived& AsDerived() const { return *static_cast<const Derived*>(this); }
  Derived& AsDerived() { return *static_cast<Derived*>(this); }
};

class ClientTiledLayerBuffer;
class SurfaceDescriptorTiles;
class ISurfaceAllocator;



class TiledLayerComposer
{
public:
  









  virtual bool UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) = 0;

  



  virtual const nsIntRegion& GetValidLowPrecisionRegion() const = 0;

  virtual const nsIntRegion& GetValidRegion() const = 0;
};

template<typename Derived, typename Tile> void
TiledLayerBuffer<Derived, Tile>::Dump(std::stringstream& aStream,
                                      const char* aPrefix,
                                      bool aDumpHtml)
{
  for (size_t i = 0; i < mRetainedTiles.Length(); ++i) {
    const TileIntPoint tilePosition = mTiles.TilePosition(i);
    gfx::IntPoint tileOffset = GetTileOffset(tilePosition);

    aStream << "\n" << aPrefix << "Tile (x=" <<
      tileOffset.x << ", y=" << tileOffset.y << "): ";
    if (!mRetainedTiles[i].IsPlaceholderTile()) {
      mRetainedTiles[i].DumpTexture(aStream);
    } else {
      aStream << "empty tile";
    }
  }
}

} 
} 

#endif 
