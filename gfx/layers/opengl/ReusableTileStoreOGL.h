



#ifndef GFX_REUSABLETILESTOREOGL_H
#define GFX_REUSABLETILESTOREOGL_H

#include "TiledThebesLayerOGL.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

namespace mozilla {

namespace gl {
class GLContext;
}

namespace layers {



class ReusableTiledTextureOGL
{
public:
  ReusableTiledTextureOGL(TiledTexture aTexture,
                          const nsIntRegion& aTileRegion,
                          uint16_t aTileSize,
                          gfxSize aResolution)
    : mTexture(aTexture)
    , mTileRegion(aTileRegion)
    , mTileSize(aTileSize)
    , mResolution(aResolution)
  {}

  ~ReusableTiledTextureOGL() {}

  TiledTexture mTexture;
  const nsIntRegion mTileRegion;
  uint16_t mTileSize;
  gfxSize mResolution;
};







class ReusableTileStoreOGL
{
public:
  ReusableTileStoreOGL(gl::GLContext* aContext, float aSizeLimit)
    : mContext(aContext)
    , mSizeLimit(aSizeLimit)
  {}

  ~ReusableTileStoreOGL();

  
  
  
  
  
  void HarvestTiles(TiledLayerBufferOGL* aVideoMemoryTiledBuffer,
                    const nsIntRegion& aOldValidRegion,
                    const nsIntRegion& aNewValidRegion,
                    const gfxSize& aOldResolution,
                    const gfxSize& aNewResolution);

  
  
  
  void DrawTiles(TiledThebesLayerOGL* aLayer,
                 const nsIntRegion& aValidRegion,
                 const gfxSize& aResolution,
                 const gfx3DMatrix& aTransform,
                 const nsIntPoint& aRenderOffset);

private:
  
  
  nsRefPtr<gl::GLContext> mContext;

  
  
  
  float mSizeLimit;

  
  nsTArray< nsAutoPtr<ReusableTiledTextureOGL> > mTiles;
};

} 
} 

#endif 
