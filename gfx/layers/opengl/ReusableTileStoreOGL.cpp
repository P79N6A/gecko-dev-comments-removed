



#include "ReusableTileStoreOGL.h"

namespace mozilla {
namespace layers {

ReusableTileStoreOGL::~ReusableTileStoreOGL()
{
  if (mTiles.Length() == 0)
    return;

  mContext->MakeCurrent();
  for (PRUint32 i = 0; i < mTiles.Length(); i++)
    mContext->fDeleteTextures(1, &mTiles[i]->mTexture.mTextureHandle);
  mTiles.Clear();
}

void
ReusableTileStoreOGL::HarvestTiles(TiledLayerBufferOGL* aVideoMemoryTiledBuffer,
                                   const nsIntSize& aContentSize,
                                   const nsIntRegion& aOldValidRegion,
                                   const nsIntRegion& aNewValidRegion,
                                   const gfxSize& aOldResolution,
                                   const gfxSize& aNewResolution)
{
  gfxSize scaleFactor = gfxSize(aNewResolution.width / aOldResolution.width,
                                aNewResolution.height / aOldResolution.height);

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Seeing if there are any tiles we can reuse\n");
#endif

  
  
  mContext->MakeCurrent();
  for (PRUint32 i = 0; i < mTiles.Length();) {
    ReusableTiledTextureOGL* tile = mTiles[i];

    nsIntRect tileRect;
    bool release = false;
    if (tile->mResolution == aNewResolution) {
      if (aNewValidRegion.Contains(tile->mTileRegion)) {
        release = true;
      } else {
        tileRect = tile->mTileRegion.GetBounds();
      }
    } else {
      nsIntRegion transformedTileRegion(tile->mTileRegion);
      transformedTileRegion.ScaleRoundOut(tile->mResolution.width / aNewResolution.width,
                                          tile->mResolution.height / aNewResolution.height);
      if (aNewValidRegion.Contains(transformedTileRegion))
        release = true;
      else
        tileRect = transformedTileRegion.GetBounds();
    }

    if (!release) {
      if (tileRect.width > aContentSize.width ||
          tileRect.height > aContentSize.height)
        release = true;
    }

    if (release) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
      nsIntRect tileBounds = tile->mTileRegion.GetBounds();
      printf_stderr("Releasing obsolete reused tile at %d,%d, x%f\n",
                    tileBounds.x, tileBounds.y, tile->mResolution.width);
#endif
      mContext->fDeleteTextures(1, &tile->mTexture.mTextureHandle);
      mTiles.RemoveElementAt(i);
      continue;
    }

    i++;
  }

  
  
  
  
  
  
  uint16_t tileSize = aVideoMemoryTiledBuffer->GetTileLength();
  nsIntRect validBounds = aOldValidRegion.GetBounds();
  for (int x = validBounds.x; x < validBounds.XMost();) {
    int w = tileSize - x % tileSize;
    if (x + w > validBounds.x + validBounds.width)
      w = validBounds.x + validBounds.width - x;

    for (int y = validBounds.y; y < validBounds.YMost();) {
      int h = tileSize - y % tileSize;
      if (y + h > validBounds.y + validBounds.height)
        h = validBounds.y + validBounds.height - y;

      
      
      nsIntRegion tileRegion;
      tileRegion.And(aOldValidRegion, nsIntRect(x, y, w, h));

      nsIntRegion intersectingRegion;
      bool retainTile = false;
      if (aNewResolution != aOldResolution) {
        
        
        
        
        nsIntRegion transformedTileRegion(tileRegion);
        transformedTileRegion.ScaleRoundOut(scaleFactor.width, scaleFactor.height);
        if (!aNewValidRegion.Contains(transformedTileRegion))
          retainTile = true;
      } else if (intersectingRegion.And(tileRegion, aNewValidRegion).IsEmpty()) {
        retainTile = true;
      }

      if (retainTile) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
        printf_stderr("Retaining tile at %d,%d, x%f for reuse\n", x, y, aOldResolution.width);
#endif
        TiledTexture removedTile;
        if (aVideoMemoryTiledBuffer->RemoveTile(nsIntPoint(x, y), removedTile)) {
          ReusableTiledTextureOGL* reusedTile =
            new ReusableTiledTextureOGL(removedTile, nsIntPoint(x, y), tileRegion,
                                        tileSize, aOldResolution);
          mTiles.AppendElement(reusedTile);
        }
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
        else
          printf_stderr("Failed to retain tile for reuse\n");
#endif
      }

      y += h;
    }

    x += w;
  }

  
  while (mTiles.Length() > aVideoMemoryTiledBuffer->GetTileCount() * mSizeLimit) {
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
    nsIntRect tileBounds = mTiles[0]->mTileRegion.GetBounds();
    printf_stderr("Releasing old reused tile at %d,%d, x%f\n",
                  tileBounds.x, tileBounds.y, mTiles[0]->mResolution.width);
#endif
    mContext->fDeleteTextures(1, &mTiles[0]->mTexture.mTextureHandle);
    mTiles.RemoveElementAt(0);
  }

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Retained %d tiles\n", mTiles.Length());
#endif
}

void
ReusableTileStoreOGL::DrawTiles(TiledThebesLayerOGL* aLayer,
                                const nsIntSize& aContentSize,
                                const nsIntRegion& aValidRegion,
                                const gfxSize& aResolution,
                                const gfx3DMatrix& aTransform,
                                const nsIntPoint& aRenderOffset,
                                Layer* aMaskLayer)
{
  
  for (PRUint32 i = 0; i < mTiles.Length(); i++) {
    ReusableTiledTextureOGL* tile = mTiles[i];

    
    gfxSize scaleFactor = gfxSize(aResolution.width / tile->mResolution.width,
                                  aResolution.height / tile->mResolution.height);

    
    nsIntRegion transformedTileRegion(tile->mTileRegion);
    if (aResolution != tile->mResolution)
      transformedTileRegion.ScaleRoundOut(scaleFactor.width, scaleFactor.height);

    
    if (aValidRegion.Contains(transformedTileRegion))
      continue;

    
    
    nsIntRect transformedTileRect = transformedTileRegion.GetBounds();
    if (transformedTileRect.XMost() > aContentSize.width ||
        transformedTileRect.YMost() > aContentSize.height)
      continue;

    
    gfx3DMatrix transform = aTransform;
    if (aResolution != tile->mResolution)
      transform.Scale(scaleFactor.width, scaleFactor.height, 1);

    
    
    
    
    
    uint16_t tileStartX = tile->mTileOrigin.x % tile->mTileSize;
    uint16_t tileStartY = tile->mTileOrigin.y % tile->mTileSize;
    nsIntPoint tileOffset(tile->mTileOrigin.x - tileStartX, tile->mTileOrigin.y - tileStartY);
    nsIntSize textureSize(tile->mTileSize, tile->mTileSize);
    aLayer->RenderTile(tile->mTexture, transform, aRenderOffset, tile->mTileRegion, tileOffset, textureSize, aMaskLayer);
  }
}

} 
} 
