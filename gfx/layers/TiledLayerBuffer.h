



#ifndef GFX_TILEDLAYERBUFFER_H
#define GFX_TILEDLAYERBUFFER_H

#define TILEDLAYERBUFFER_TILE_SIZE 256






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
  {}

  ~TiledLayerBuffer() {}

  
  
  
  
  
  Tile GetTile(const nsIntPoint& aTileOrigin) const;

  
  
  
  Tile GetTile(int x, int y) const;

  uint16_t GetTileLength() const { return TILEDLAYERBUFFER_TILE_SIZE; }

  const nsIntRegion& GetValidRegion() const { return mValidRegion; }
  const nsIntRegion& GetLastPaintRegion() const { return mLastPaintRegion; }
  void SetLastPaintRegion(const nsIntRegion& aLastPaintRegion) {
    mLastPaintRegion = aLastPaintRegion;
  }

  
  int RoundDownToTileEdge(int aX) const { return aX - aX % GetTileLength(); }

protected:
  
  
  
  
  void Update(const nsIntRegion& aNewValidRegion, const nsIntRegion& aPaintRegion);

  nsIntRegion     mValidRegion;
  nsIntRegion     mLastPaintRegion;

  






  nsTArray<Tile>  mRetainedTiles;
  int             mRetainedWidth;  
  int             mRetainedHeight; 

private:
  TiledLayerBuffer(const TiledLayerBuffer&) MOZ_DELETE;

  const Derived& AsDerived() const { return *static_cast<const Derived*>(this); }
  Derived& AsDerived() { return *static_cast<Derived*>(this); }

  bool IsPlaceholder(Tile aTile) const { return aTile == AsDerived().GetPlaceholderTile(); }
};

class BasicTiledLayerBuffer;



class TiledLayerComposer
{
public:
  





  virtual void PaintedTiledLayerBuffer(const BasicTiledLayerBuffer* aTiledBuffer) = 0;
};

template<typename Derived, typename Tile> Tile
TiledLayerBuffer<Derived, Tile>::GetTile(const nsIntPoint& aTileOrigin) const
{
  
  
  
  int firstTileX = mValidRegion.GetBounds().x / GetTileLength();
  int firstTileY = mValidRegion.GetBounds().y / GetTileLength();
  return GetTile(aTileOrigin.x / GetTileLength() - firstTileX,
                 aTileOrigin.y / GetTileLength() - firstTileY);
}

template<typename Derived, typename Tile> Tile
TiledLayerBuffer<Derived, Tile>::GetTile(int x, int y) const
{
  int index = x * mRetainedHeight + y;
  return mRetainedTiles.SafeElementAt(index, AsDerived().GetPlaceholderTile());
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
  int tileY;
  
  for (int x = newBound.x; x < newBound.XMost(); tileX++) {
    
    
    int width = GetTileLength() - x % GetTileLength();
    if (x + width > newBound.XMost()) {
      width = newBound.x + newBound.width - x;
    }

    tileY = 0;
    for (int y = newBound.y; y < newBound.YMost(); tileY++) {
      int height = GetTileLength() - y % GetTileLength();
      if (y + height > newBound.y + newBound.height) {
        height = newBound.y + newBound.height - y;
      }

      const nsIntRect tileRect(x,y,width,height);
      if (oldValidRegion.Intersects(tileRect) && newValidRegion.Intersects(tileRect)) {
        
        
        
        int tileX = (x - oldBufferOrigin.x) / GetTileLength();
        int tileY = (y - oldBufferOrigin.y) / GetTileLength();
        int index = tileX * oldRetainedHeight + tileY;

        NS_ABORT_IF_FALSE(!IsPlaceholder(oldRetainedTiles.
                                         SafeElementAt(index, AsDerived().GetPlaceholderTile())),
                          "Expected tile");

        Tile tileWithPartialValidContent = oldRetainedTiles[index];
        newRetainedTiles.AppendElement(tileWithPartialValidContent);

        oldRetainedTiles[index] = AsDerived().GetPlaceholderTile();
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
  for (int x = newBound.x; x < newBound.x + newBound.width; tileX++) {
    
    
    int tileStartX = RoundDownToTileEdge(x);
    int width = GetTileLength() - x % GetTileLength();
    if (x + width > newBound.XMost())
      width = newBound.XMost() - x;

    tileY = 0;
    for (int y = newBound.y; y < newBound.y + newBound.height; tileY++) {
      int tileStartY = RoundDownToTileEdge(y);
      int height = GetTileLength() - y % GetTileLength();
      if (y + height > newBound.YMost()) {
        height = newBound.YMost() - y;
      }

      const nsIntRect tileRect(x, y, width, height);

      nsIntRegion tileDrawRegion;
      tileDrawRegion.And(tileRect, regionToPaint);

      if (tileDrawRegion.IsEmpty()) {
        
        
        
#ifdef DEBUG
        int currTileX = (x - newBufferOrigin.x) / GetTileLength();
        int currTileY = (y - newBufferOrigin.y) / GetTileLength();
        int index = currTileX * mRetainedHeight + currTileY;
        NS_ABORT_IF_FALSE(!newValidRegion.Intersects(tileRect) ||
                          !IsPlaceholder(newRetainedTiles.
                                         SafeElementAt(index, AsDerived().GetPlaceholderTile())),
                          "If we don't draw a tile we shouldn't have a placeholder there.");
#endif
        y += height;
        continue;
      }

      int tileX = (x - newBufferOrigin.x) / GetTileLength();
      int tileY = (y - newBufferOrigin.y) / GetTileLength();
      int index = tileX * mRetainedHeight + tileY;
      NS_ABORT_IF_FALSE(index >= 0 && index < newRetainedTiles.Length(), "index out of range");
      Tile newTile = newRetainedTiles[index];
      while (IsPlaceholder(newTile) && oldRetainedTiles.Length() > 0) {
        AsDerived().SwapTiles(newTile, oldRetainedTiles[oldRetainedTiles.Length()-1]);
        oldRetainedTiles.RemoveElementAt(oldRetainedTiles.Length()-1);
      }

      
      
      
      nsIntPoint tileOrigin(tileStartX, tileStartY);
      newTile = AsDerived().ValidateTile(newTile, nsIntPoint(tileStartX, tileStartY),
                                         tileDrawRegion);
      NS_ABORT_IF_FALSE(!IsPlaceholder(newTile), "index out of range");
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
  mLastPaintRegion = aPaintRegion;
}

} 
} 

#endif 
