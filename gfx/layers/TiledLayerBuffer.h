



#ifndef GFX_TILEDLAYERBUFFER_H
#define GFX_TILEDLAYERBUFFER_H

#define TILEDLAYERBUFFER_TILE_SIZE 256


#ifdef MOZ_ANDROID_OMTC
  
  
  #define FORCE_BASICTILEDTHEBESLAYER
#endif



#include "nsRect.h"
#include "nsRegion.h"
#include "nsTArray.h"

namespace mozilla {
namespace layers {




















































template<typename Derived, typename Tile>
class TiledLayerBuffer
{
public:
  TiledLayerBuffer()
    : mRetainedWidth(0)
    , mRetainedHeight(0)
    , mResolution(1)
  {}

  ~TiledLayerBuffer() {}

  
  
  
  
  
  
  Tile GetTile(const nsIntPoint& aTileOrigin) const;

  
  
  
  
  Tile GetTile(int x, int y) const;

  
  
  
  bool RemoveTile(const nsIntPoint& aTileOrigin, Tile& aRemovedTile);

  
  
  
  bool RemoveTile(int x, int y, Tile& aRemovedTile);

  uint16_t GetTileLength() const { return TILEDLAYERBUFFER_TILE_SIZE; }
  uint32_t GetScaledTileLength() const { return TILEDLAYERBUFFER_TILE_SIZE / mResolution; }

  unsigned int GetTileCount() const { return mRetainedTiles.Length(); }

  const nsIntRegion& GetValidRegion() const { return mValidRegion; }
  const nsIntRegion& GetPaintedRegion() const { return mPaintedRegion; }
  void ClearPaintedRegion() { mPaintedRegion.SetEmpty(); }

  
  int GetTileStart(int i) const {
    return (i >= 0) ? (i % GetScaledTileLength())
                    : ((GetScaledTileLength() - (-i % GetScaledTileLength())) %
                       GetScaledTileLength());
  }

  
  int RoundDownToTileEdge(int aX) const { return aX - GetTileStart(aX); }

  
  
  
  
  
  float GetResolution() const { return mResolution; }
  void SetResolution(float aResolution) {
    if (mResolution == aResolution) {
      return;
    }

    Update(nsIntRegion(), nsIntRegion());
    mResolution = aResolution;
  }
  bool IsLowPrecision() const { return mResolution < 1; }

protected:
  
  
  
  
  void Update(const nsIntRegion& aNewValidRegion, const nsIntRegion& aPaintRegion);

  nsIntRegion     mValidRegion;
  nsIntRegion     mPaintedRegion;

  







  nsTArray<Tile>  mRetainedTiles;
  int             mRetainedWidth;  
  int             mRetainedHeight; 
  float           mResolution;

private:
  const Derived& AsDerived() const { return *static_cast<const Derived*>(this); }
  Derived& AsDerived() { return *static_cast<Derived*>(this); }

  bool IsPlaceholder(Tile aTile) const { return aTile == AsDerived().GetPlaceholderTile(); }
};

class BasicTiledLayerBuffer;



class TiledLayerComposer
{
public:
  





  virtual void PaintedTiledLayerBuffer(const BasicTiledLayerBuffer* aTiledBuffer) = 0;

  virtual void MemoryPressure() = 0;

  



  virtual const nsIntRegion& GetValidLowPrecisionRegion() const = 0;
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
  
  
  
  int firstTileX = floor_div(mValidRegion.GetBounds().x, GetScaledTileLength());
  int firstTileY = floor_div(mValidRegion.GetBounds().y, GetScaledTileLength());
  return GetTile(floor_div(aTileOrigin.x, GetScaledTileLength()) - firstTileX,
                 floor_div(aTileOrigin.y, GetScaledTileLength()) - firstTileY);
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
  int firstTileX = floor_div(mValidRegion.GetBounds().x, GetScaledTileLength());
  int firstTileY = floor_div(mValidRegion.GetBounds().y, GetScaledTileLength());
  return RemoveTile(floor_div(aTileOrigin.x, GetScaledTileLength()) - firstTileX,
                    floor_div(aTileOrigin.y, GetScaledTileLength()) - firstTileY,
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
  nsTArray<Tile>  newRetainedTiles;
  nsTArray<Tile>& oldRetainedTiles = mRetainedTiles;
  const nsIntRect oldBound = mValidRegion.GetBounds();
  const nsIntRect newBound = aNewValidRegion.GetBounds();
  const nsIntPoint oldBufferOrigin(RoundDownToTileEdge(oldBound.x),
                                   RoundDownToTileEdge(oldBound.y));
  const nsIntPoint newBufferOrigin(RoundDownToTileEdge(newBound.x),
                                   RoundDownToTileEdge(newBound.y));
  const nsIntRegion& oldValidRegion = mValidRegion;
  const nsIntRegion& newValidRegion = aNewValidRegion;
  const int oldRetainedHeight = mRetainedHeight;

  
  
  
  
  
  int tileX = 0;
  int tileY = 0;
  
  for (int32_t x = newBound.x; x < newBound.XMost(); tileX++) {
    
    
    int width = GetScaledTileLength() - GetTileStart(x);
    if (x + width > newBound.XMost()) {
      width = newBound.x + newBound.width - x;
    }

    tileY = 0;
    for (int32_t y = newBound.y; y < newBound.YMost(); tileY++) {
      int height = GetScaledTileLength() - GetTileStart(y);
      if (y + height > newBound.y + newBound.height) {
        height = newBound.y + newBound.height - y;
      }

      const nsIntRect tileRect(x,y,width,height);
      if (oldValidRegion.Intersects(tileRect) && newValidRegion.Intersects(tileRect)) {
        
        
        
        int tileX = floor_div(x - oldBufferOrigin.x, GetScaledTileLength());
        int tileY = floor_div(y - oldBufferOrigin.y, GetScaledTileLength());
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
      }

      y += height;
    }

    x += width;
  }

  
  
  mRetainedWidth = tileX;
  mRetainedHeight = tileY;

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
    
    
    int tileStartX = RoundDownToTileEdge(x);
    int width = GetScaledTileLength() - GetTileStart(x);
    if (x + width > newBound.XMost())
      width = newBound.XMost() - x;

    tileY = 0;
    for (int y = newBound.y; y < newBound.y + newBound.height; tileY++) {
      int tileStartY = RoundDownToTileEdge(y);
      int height = GetScaledTileLength() - GetTileStart(y);
      if (y + height > newBound.YMost()) {
        height = newBound.YMost() - y;
      }

      const nsIntRect tileRect(x, y, width, height);

      nsIntRegion tileDrawRegion;
      tileDrawRegion.And(tileRect, regionToPaint);

      if (tileDrawRegion.IsEmpty()) {
        
        
        
#ifdef DEBUG
        int currTileX = floor_div(x - newBufferOrigin.x, GetScaledTileLength());
        int currTileY = floor_div(y - newBufferOrigin.y, GetScaledTileLength());
        int index = currTileX * mRetainedHeight + currTileY;
        NS_ABORT_IF_FALSE(!newValidRegion.Intersects(tileRect) ||
                          !IsPlaceholder(newRetainedTiles.
                                         SafeElementAt(index, AsDerived().GetPlaceholderTile())),
                          "If we don't draw a tile we shouldn't have a placeholder there.");
#endif
        y += height;
        continue;
      }

      int tileX = floor_div(x - newBufferOrigin.x, GetScaledTileLength());
      int tileY = floor_div(y - newBufferOrigin.y, GetScaledTileLength());
      int index = tileX * mRetainedHeight + tileY;
      NS_ABORT_IF_FALSE(index >= 0 &&
                        static_cast<unsigned>(index) < newRetainedTiles.Length(),
                        "index out of range");

      Tile newTile = newRetainedTiles[index];
      while (IsPlaceholder(newTile) && oldRetainedTiles.Length() > 0) {
        AsDerived().SwapTiles(newTile, oldRetainedTiles[oldRetainedTiles.Length()-1]);
        oldRetainedTiles.RemoveElementAt(oldRetainedTiles.Length()-1);
      }

      
      
      
      nsIntPoint tileOrigin(tileStartX, tileStartY);
      newTile = AsDerived().ValidateTile(newTile, nsIntPoint(tileStartX, tileStartY),
                                         tileDrawRegion);
      NS_ABORT_IF_FALSE(!IsPlaceholder(newTile), "index out of range");
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
      printf_stderr("Store Validate tile %i, %i -> %i\n", tileStartX, tileStartY, index);
#endif
      newRetainedTiles[index] = newTile;

      y += height;
    }

    x += width;
  }

  
  
  while (oldRetainedTiles.Length() > 0) {
    Tile oldTile = oldRetainedTiles[oldRetainedTiles.Length()-1];
    oldRetainedTiles.RemoveElementAt(oldRetainedTiles.Length()-1);
    AsDerived().ReleaseTile(oldTile);
  }

  mRetainedTiles = newRetainedTiles;
  mValidRegion = aNewValidRegion;
  mPaintedRegion.Or(mPaintedRegion, aPaintRegion);
}

} 
} 

#endif 
