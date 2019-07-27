



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
    , mTileSize(gfxPlatform::GetPlatform()->GetTileWidth(), gfxPlatform::GetPlatform()->GetTileHeight())
  {}

  ~TiledLayerBuffer() {}

  
  
  
  
  
  
  Tile GetTile(const nsIntPoint& aTileOrigin) const;

  
  
  
  
  Tile GetTile(int x, int y) const;

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
      if (!mRetainedTiles[i].IsPlaceholderTile()) {
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

  void Dump(std::stringstream& aStream, const char* aPrefix, bool aDumpHtml);

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

template<typename Derived, typename Tile> void
TiledLayerBuffer<Derived, Tile>::Dump(std::stringstream& aStream,
                                      const char* aPrefix,
                                      bool aDumpHtml)
{
  nsIntRect visibleRect = GetValidRegion().GetBounds();
  gfx::IntSize scaledTileSize = GetScaledTileSize();
  for (int32_t x = visibleRect.x; x < visibleRect.x + visibleRect.width;) {
    int32_t tileStartX = GetTileStart(x, scaledTileSize.width);
    int32_t w = scaledTileSize.width - tileStartX;

    for (int32_t y = visibleRect.y; y < visibleRect.y + visibleRect.height;) {
      int32_t tileStartY = GetTileStart(y, scaledTileSize.height);
      Tile tileTexture =
        GetTile(nsIntPoint(RoundDownToTileEdge(x, scaledTileSize.width),
                           RoundDownToTileEdge(y, scaledTileSize.height)));
      int32_t h = scaledTileSize.height - tileStartY;

      aStream << "\n" << aPrefix << "Tile (x=" <<
        RoundDownToTileEdge(x, scaledTileSize.width) << ", y=" <<
        RoundDownToTileEdge(y, scaledTileSize.height) << "): ";
      if (tileTexture != AsDerived().GetPlaceholderTile()) {
        tileTexture.DumpTexture(aStream);
      } else {
        aStream << "empty tile";
      }
      y += h;
    }
    x += w;
  }
}

template<typename Derived, typename Tile> void
TiledLayerBuffer<Derived, Tile>::Update(const nsIntRegion& newValidRegion,
                                        const nsIntRegion& aPaintRegion)
{
  gfx::IntSize scaledTileSize = GetScaledTileSize();

  nsTArray<Tile>  newRetainedTiles;
  nsTArray<Tile>& oldRetainedTiles = mRetainedTiles;
  const nsIntRect oldBound = mValidRegion.GetBounds();
  const nsIntRect newBound = newValidRegion.GetBounds();
  const nsIntPoint oldBufferOrigin(RoundDownToTileEdge(oldBound.x, scaledTileSize.width),
                                   RoundDownToTileEdge(oldBound.y, scaledTileSize.height));
  const nsIntPoint newBufferOrigin(RoundDownToTileEdge(newBound.x, scaledTileSize.width),
                                   RoundDownToTileEdge(newBound.y, scaledTileSize.height));

  
  
  const nsIntRegion& oldValidRegion = mValidRegion;
  const int oldRetainedHeight = mRetainedHeight;

#ifdef GFX_TILEDLAYER_RETAINING_LOG
  { 
    std::stringstream ss;
    ss << "TiledLayerBuffer " << this << " starting update"
       << " on bounds ";
    AppendToString(ss, newBound);
    ss << " with mResolution=" << mResolution << "\n";
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      ss << "mRetainedTiles[" << i << "] = ";
      mRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    print_stderr(ss);
  }
#endif

  
  
  
  
  
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

        
        if (oldRetainedTiles.
                          SafeElementAt(index, AsDerived().GetPlaceholderTile()).IsPlaceholderTile()) {
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

#ifdef GFX_TILEDLAYER_RETAINING_LOG
  { 
    std::stringstream ss;
    ss << "TiledLayerBuffer " << this << " finished pass 1 of update;"
       << " tilesMissing=" << tilesMissing << "\n";
    for (size_t i = 0; i < oldRetainedTiles.Length(); i++) {
      ss << "oldRetainedTiles[" << i << "] = ";
      oldRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    print_stderr(ss);
  }
#endif


  
  
  
  
  
  int oldTileCount = 0;
  for (size_t i = 0; i < oldRetainedTiles.Length(); i++) {
    Tile oldTile = oldRetainedTiles[i];
    if (oldTile.IsPlaceholderTile()) {
      continue;
    }

    if (oldTileCount >= tilesMissing) {
      oldRetainedTiles[i] = AsDerived().GetPlaceholderTile();
      AsDerived().ReleaseTile(oldTile);
    } else {
      oldTileCount ++;
    }
  }

  if (!newValidRegion.Contains(aPaintRegion)) {
    gfxCriticalError() << "Painting outside visible:"
		       << " paint " << aPaintRegion.ToString().get()
                       << " old valid " << oldValidRegion.ToString().get()
                       << " new valid " << newValidRegion.ToString().get();
  }
#ifdef DEBUG
  nsIntRegion oldAndPainted(oldValidRegion);
  oldAndPainted.Or(oldAndPainted, aPaintRegion);
  if (!oldAndPainted.Contains(newValidRegion)) {
    gfxCriticalError() << "Not fully painted:"
		       << " paint " << aPaintRegion.ToString().get()
                       << " old valid " << oldValidRegion.ToString().get()
                       << " old painted " << oldAndPainted.ToString().get()
                       << " new valid " << newValidRegion.ToString().get();
  }
#endif

  nsIntRegion regionToPaint(aPaintRegion);

#ifdef GFX_TILEDLAYER_RETAINING_LOG
  { 
    std::stringstream ss;
    ss << "TiledLayerBuffer " << this << " finished pass 1.5 of update\n";
    for (size_t i = 0; i < oldRetainedTiles.Length(); i++) {
      ss << "oldRetainedTiles[" << i << "] = ";
      oldRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    for (size_t i = 0; i < newRetainedTiles.Length(); i++) {
      ss << "newRetainedTiles[" << i << "] = ";
      newRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    print_stderr(ss);
  }
#endif

  
  
  
  
  
  
  
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
                     !newRetainedTiles.
                                    SafeElementAt(index, AsDerived().GetPlaceholderTile()).IsPlaceholderTile(),
                     "Unexpected placeholder tile");

#endif
        y += height;
        continue;
      }

      int tileX = floor_div(x - newBufferOrigin.x, scaledTileSize.width);
      int tileY = floor_div(y - newBufferOrigin.y, scaledTileSize.height);
      int index = tileX * mRetainedHeight + tileY;
      MOZ_ASSERT(index >= 0 &&
                 static_cast<unsigned>(index) < newRetainedTiles.Length(),
                 "index out of range");

      Tile newTile = newRetainedTiles[index];

      
      
      while (newTile.IsPlaceholderTile() && oldRetainedTiles.Length() > 0) {
        AsDerived().SwapTiles(newTile, oldRetainedTiles[oldRetainedTiles.Length()-1]);
        oldRetainedTiles.RemoveElementAt(oldRetainedTiles.Length()-1);
        if (!newTile.IsPlaceholderTile()) {
          oldTileCount--;
        }
      }

      
      
      
      nsIntPoint tileOrigin(tileStartX, tileStartY);
      newTile = AsDerived().ValidateTile(newTile, nsIntPoint(tileStartX, tileStartY),
                                         tileDrawRegion);
      NS_ASSERTION(!newTile.IsPlaceholderTile(), "Unexpected placeholder tile - failed to allocate?");
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

#ifdef GFX_TILEDLAYER_RETAINING_LOG
  { 
    std::stringstream ss;
    ss << "TiledLayerBuffer " << this << " finished pass 2 of update;"
       << " oldTileCount=" << oldTileCount << "\n";
    for (size_t i = 0; i < oldRetainedTiles.Length(); i++) {
      ss << "oldRetainedTiles[" << i << "] = ";
      oldRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    for (size_t i = 0; i < newRetainedTiles.Length(); i++) {
      ss << "newRetainedTiles[" << i << "] = ";
      newRetainedTiles[i].Dump(ss);
      ss << "\n";
    }
    print_stderr(ss);
  }
#endif

  
  MOZ_ASSERT(oldTileCount == 0, "Failed to release old tiles");

  mRetainedTiles = newRetainedTiles;
  mValidRegion = newValidRegion;
  mPaintedRegion.Or(mPaintedRegion, aPaintRegion);
}

} 
} 

#endif 
