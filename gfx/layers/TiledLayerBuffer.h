



#ifndef GFX_TILEDLAYERBUFFER_H
#define GFX_TILEDLAYERBUFFER_H





#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxPrefs.h"                   
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif

namespace mozilla {
namespace layers {



#define ENABLE_TILING_LOG 0

#if ENABLE_TILING_LOG
#  define TILING_LOG(...) printf_stderr(__VA_ARGS__);
#else
#  define TILING_LOG(...)
#endif




















































template<typename Derived, typename Tile>
class TiledLayerBuffer
{
public:
  TiledLayerBuffer()
    : mRetainedWidth(0)
    , mRetainedHeight(0)
    , mResolution(1)
    , mTileSize(gfxPrefs::LayersTileWidth(), gfxPrefs::LayersTileHeight())
  {}

  ~TiledLayerBuffer() {}

  
  
  
  
  
  
  Tile GetTile(const nsIntPoint& aTileOrigin) const;

  
  
  
  
  Tile GetTile(int x, int y) const;

  
  
  
  bool RemoveTile(const nsIntPoint& aTileOrigin, Tile& aRemovedTile);

  
  
  
  bool RemoveTile(int x, int y, Tile& aRemovedTile);

  const gfx::IntSize& GetTileSize() const { return mTileSize; }

  gfx::IntSize GetScaledTileSize() const { return RoundedToInt(gfx::Size(mTileSize) / mResolution); }

  unsigned int GetTileCount() const { return mRetainedTiles.Length(); }

  const nsIntRegion& GetValidRegion() const { return mValidRegion; }
  const nsIntRegion& GetPaintedRegion() const { return mPaintedRegion; }
  void ClearPaintedRegion() { mPaintedRegion.SetEmpty(); }

  void ResetPaintedAndValidState() {
    mPaintedRegion.SetEmpty();
    mValidRegion.SetEmpty();
    mRetainedWidth = 0;
    mRetainedHeight = 0;
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (!IsPlaceholder(mRetainedTiles[i])) {
        AsDerived().ReleaseTile(mRetainedTiles[i]);
      }
    }
    mRetainedTiles.Clear();
  }

  
  int GetTileStart(int i, int aTileLength) const {
    return (i >= 0) ? (i % aTileLength)
                    : ((aTileLength - (-i % aTileLength)) %
                       aTileLength);
  }

  
  int RoundDownToTileEdge(int aX, int aTileLength) const { return aX - GetTileStart(aX, aTileLength); }

  
  
  
  
  
  float GetResolution() const { return mResolution; }
  void SetResolution(float aResolution) {
    if (mResolution == aResolution) {
      return;
    }

    Update(nsIntRegion(), nsIntRegion());
    mResolution = aResolution;
  }
  bool IsLowPrecision() const { return mResolution < 1; }

  typedef Tile* Iterator;
  Iterator TilesBegin() { return mRetainedTiles.Elements(); }
  Iterator TilesEnd() { return mRetainedTiles.Elements() + mRetainedTiles.Length(); }

protected:
  
  
  
  
  void Update(const nsIntRegion& aNewValidRegion, const nsIntRegion& aPaintRegion);

  nsIntRegion     mValidRegion;
  nsIntRegion     mPaintedRegion;

  







  nsTArray<Tile>  mRetainedTiles;
  int             mRetainedWidth;  
  int             mRetainedHeight; 
  float           mResolution;
  gfx::IntSize    mTileSize;

private:
  const Derived& AsDerived() const { return *static_cast<const Derived*>(this); }
  Derived& AsDerived() { return *static_cast<Derived*>(this); }

  bool IsPlaceholder(Tile aTile) const { return aTile == AsDerived().GetPlaceholderTile(); }
};

class ClientTiledLayerBuffer;
class SurfaceDescriptorTiles;
class ISurfaceAllocator;



class TiledLayerComposer
{
public:
  






  virtual void UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) = 0;

  



  virtual const nsIntRegion& GetValidLowPrecisionRegion() const = 0;

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  



  virtual void SetReleaseFence(const android::sp<android::Fence>& aReleaseFence) = 0;
#endif
};



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

template<typename Derived, typename Tile> Tile
TiledLayerBuffer<Derived, Tile>::GetTile(const nsIntPoint& aTileOrigin) const
{
  
  
  
  gfx::IntSize scaledTileSize = GetScaledTileSize();
  int firstTileX = floor_div(mValidRegion.GetBounds().x, scaledTileSize.width);
  int firstTileY = floor_div(mValidRegion.GetBounds().y, scaledTileSize.height);
  return GetTile(floor_div(aTileOrigin.x, scaledTileSize.width) - firstTileX,
                 floor_div(aTileOrigin.y, scaledTileSize.height) - firstTileY);
}

template<typename Derived, typename Tile> Tile
TiledLayerBuffer<Derived, Tile>::GetTile(int x, int y) const
{
  int index = x * mRetainedHeight + y;
  return mRetainedTiles.SafeElementAt(index, AsDerived().GetPlaceholderTile());
}

template<typename Derived, typename Tile> bool
TiledLayerBuffer<Derived, Tile>::RemoveTile(const nsIntPoint& aTileOrigin,
                                            Tile& aRemovedTile)
{
  gfx::IntSize scaledTileSize = GetScaledTileSize();
  int firstTileX = floor_div(mValidRegion.GetBounds().x, scaledTileSize.width);
  int firstTileY = floor_div(mValidRegion.GetBounds().y, scaledTileSize.height);
  return RemoveTile(floor_div(aTileOrigin.x, scaledTileSize.width) - firstTileX,
                    floor_div(aTileOrigin.y, scaledTileSize.height) - firstTileY,
                    aRemovedTile);
}

template<typename Derived, typename Tile> bool
TiledLayerBuffer<Derived, Tile>::RemoveTile(int x, int y, Tile& aRemovedTile)
{
  int index = x * mRetainedHeight + y;
  const Tile& tileToRemove = mRetainedTiles.SafeElementAt(index, AsDerived().GetPlaceholderTile());
  if (!IsPlaceholder(tileToRemove)) {
    aRemovedTile = tileToRemove;
    mRetainedTiles[index] = AsDerived().GetPlaceholderTile();
    return true;
  }
  return false;
}

template<typename Derived, typename Tile> void
TiledLayerBuffer<Derived, Tile>::Update(const nsIntRegion& aNewValidRegion,
                                        const nsIntRegion& aPaintRegion)
{
  gfx::IntSize scaledTileSize = GetScaledTileSize();

  nsTArray<Tile>  newRetainedTiles;
  nsTArray<Tile>& oldRetainedTiles = mRetainedTiles;
  const nsIntRect oldBound = mValidRegion.GetBounds();
  const nsIntRect newBound = aNewValidRegion.GetBounds();
  const nsIntPoint oldBufferOrigin(RoundDownToTileEdge(oldBound.x, scaledTileSize.width),
                                   RoundDownToTileEdge(oldBound.y, scaledTileSize.height));
  const nsIntPoint newBufferOrigin(RoundDownToTileEdge(newBound.x, scaledTileSize.width),
                                   RoundDownToTileEdge(newBound.y, scaledTileSize.height));
  const nsIntRegion& oldValidRegion = mValidRegion;
  const nsIntRegion& newValidRegion = aNewValidRegion;
  const int oldRetainedHeight = mRetainedHeight;

  
  
  
  
  
  int tileX = 0;
  int tileY = 0;
  int tilesMissing = 0;
  
  for (int32_t x = newBound.x; x < newBound.XMost(); tileX++) {
    
    
    int width = scaledTileSize.width - GetTileStart(x, scaledTileSize.width);
    if (x + width > newBound.XMost()) {
      width = newBound.x + newBound.width - x;
    }

    tileY = 0;
    for (int32_t y = newBound.y; y < newBound.YMost(); tileY++) {
      int height = scaledTileSize.height - GetTileStart(y, scaledTileSize.height);
      if (y + height > newBound.y + newBound.height) {
        height = newBound.y + newBound.height - y;
      }

      const nsIntRect tileRect(x,y,width,height);
      if (oldValidRegion.Intersects(tileRect) && newValidRegion.Intersects(tileRect)) {
        
        
        
        int tileX = floor_div(x - oldBufferOrigin.x, scaledTileSize.width);
        int tileY = floor_div(y - oldBufferOrigin.y, scaledTileSize.height);
        int index = tileX * oldRetainedHeight + tileY;

        
        if (IsPlaceholder(oldRetainedTiles.
                          SafeElementAt(index, AsDerived().GetPlaceholderTile()))) {
          newRetainedTiles.AppendElement(AsDerived().GetPlaceholderTile());
        } else {
          Tile tileWithPartialValidContent = oldRetainedTiles[index];
          newRetainedTiles.AppendElement(tileWithPartialValidContent);
          oldRetainedTiles[index] = AsDerived().GetPlaceholderTile();
        }

      } else {
        
        
        
        
        
        
        
        newRetainedTiles.AppendElement(AsDerived().GetPlaceholderTile());

        if (aPaintRegion.Intersects(tileRect)) {
          tilesMissing++;
        }
      }

      y += height;
    }

    x += width;
  }

  
  
  mRetainedWidth = tileX;
  mRetainedHeight = tileY;

  
  
  
  
  
  int oldTileCount = 0;
  for (size_t i = 0; i < oldRetainedTiles.Length(); i++) {
    Tile oldTile = oldRetainedTiles[i];
    if (IsPlaceholder(oldTile)) {
      continue;
    }

    if (oldTileCount >= tilesMissing) {
      oldRetainedTiles[i] = AsDerived().GetPlaceholderTile();
      AsDerived().ReleaseTile(oldTile);
    } else {
      oldTileCount ++;
    }
  }

  NS_ABORT_IF_FALSE(aNewValidRegion.Contains(aPaintRegion), "Painting a region outside the visible region");
#ifdef DEBUG
  nsIntRegion oldAndPainted(oldValidRegion);
  oldAndPainted.Or(oldAndPainted, aPaintRegion);
#endif
  NS_ABORT_IF_FALSE(oldAndPainted.Contains(newValidRegion), "newValidRegion has not been fully painted");

  nsIntRegion regionToPaint(aPaintRegion);

  
  
  
  
  
  
  
  tileX = 0;
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Update %i, %i, %i, %i\n", newBound.x, newBound.y, newBound.width, newBound.height);
#endif
  for (int x = newBound.x; x < newBound.x + newBound.width; tileX++) {
    
    
    int tileStartX = RoundDownToTileEdge(x, scaledTileSize.width);
    int width = scaledTileSize.width - GetTileStart(x, scaledTileSize.width);
    if (x + width > newBound.XMost())
      width = newBound.XMost() - x;

    tileY = 0;
    for (int y = newBound.y; y < newBound.y + newBound.height; tileY++) {
      int tileStartY = RoundDownToTileEdge(y, scaledTileSize.height);
      int height = scaledTileSize.height - GetTileStart(y, scaledTileSize.height);
      if (y + height > newBound.YMost()) {
        height = newBound.YMost() - y;
      }

      const nsIntRect tileRect(x, y, width, height);

      nsIntRegion tileDrawRegion;
      tileDrawRegion.And(tileRect, regionToPaint);

      if (tileDrawRegion.IsEmpty()) {
        
        
        
#ifdef DEBUG
        int currTileX = floor_div(x - newBufferOrigin.x, scaledTileSize.width);
        int currTileY = floor_div(y - newBufferOrigin.y, scaledTileSize.height);
        int index = currTileX * mRetainedHeight + currTileY;
        
        
        NS_ASSERTION(!newValidRegion.Intersects(tileRect) ||
                     !IsPlaceholder(newRetainedTiles.
                                    SafeElementAt(index, AsDerived().GetPlaceholderTile())),
                     "Unexpected placeholder tile");

#endif
        y += height;
        continue;
      }

      int tileX = floor_div(x - newBufferOrigin.x, scaledTileSize.width);
      int tileY = floor_div(y - newBufferOrigin.y, scaledTileSize.height);
      int index = tileX * mRetainedHeight + tileY;
      NS_ABORT_IF_FALSE(index >= 0 &&
                        static_cast<unsigned>(index) < newRetainedTiles.Length(),
                        "index out of range");

      Tile newTile = newRetainedTiles[index];

      
      
      while (IsPlaceholder(newTile) && oldRetainedTiles.Length() > 0) {
        AsDerived().SwapTiles(newTile, oldRetainedTiles[oldRetainedTiles.Length()-1]);
        oldRetainedTiles.RemoveElementAt(oldRetainedTiles.Length()-1);
        if (!IsPlaceholder(newTile)) {
          oldTileCount--;
        }
      }

      
      
      
      nsIntPoint tileOrigin(tileStartX, tileStartY);
      newTile = AsDerived().ValidateTile(newTile, nsIntPoint(tileStartX, tileStartY),
                                         tileDrawRegion);
      NS_ASSERTION(!IsPlaceholder(newTile), "Unexpected placeholder tile - failed to allocate?");
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
      printf_stderr("Store Validate tile %i, %i -> %i\n", tileStartX, tileStartY, index);
#endif
      newRetainedTiles[index] = newTile;

      y += height;
    }

    x += width;
  }

  AsDerived().PostValidate(aPaintRegion);
  for (unsigned int i = 0; i < newRetainedTiles.Length(); ++i) {
    AsDerived().UnlockTile(newRetainedTiles[i]);
  }

  
  NS_ABORT_IF_FALSE(oldTileCount == 0, "Failed to release old tiles");

  mRetainedTiles = newRetainedTiles;
  mValidRegion = aNewValidRegion;
  mPaintedRegion.Or(mPaintedRegion, aPaintRegion);
}

} 
} 

#endif 
