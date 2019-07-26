



#ifndef GFX_TILEDTHEBESLAYEROGL_H
#define GFX_TILEDTHEBESLAYEROGL_H

#include "mozilla/layers/ShadowLayers.h"
#include "TiledLayerBuffer.h"
#include "Layers.h"
#include "LayerManagerOGL.h"
#include "BasicTiledThebesLayer.h"
#include <algorithm>

namespace mozilla {

namespace gl {
class GLContext;
}

namespace layers {

class ReusableTileStoreOGL;

class TiledTexture {
public:
  
  
  
  
  
  
  TiledTexture()
    : mTextureHandle(0)
    , mFormat(0)
  {}

  
  TiledTexture(GLuint aTextureHandle, GLenum aFormat)
    : mTextureHandle(aTextureHandle)
    , mFormat(aFormat)
  {}

  TiledTexture(const TiledTexture& o) {
    mTextureHandle = o.mTextureHandle;
    mFormat = o.mFormat;
  }
  TiledTexture& operator=(const TiledTexture& o) {
    if (this == &o) return *this;
    mTextureHandle = o.mTextureHandle;
    mFormat = o.mFormat;
    return *this;
  }
  bool operator== (const TiledTexture& o) const {
    return mTextureHandle == o.mTextureHandle;
  }
  bool operator!= (const TiledTexture& o) const {
    return mTextureHandle != o.mTextureHandle;
  }

  GLuint mTextureHandle;
  GLenum mFormat;
};

class TiledLayerBufferOGL : public TiledLayerBuffer<TiledLayerBufferOGL, TiledTexture>
{
  friend class TiledLayerBuffer<TiledLayerBufferOGL, TiledTexture>;

public:
  TiledLayerBufferOGL(gl::GLContext* aContext)
    : mContext(aContext)
  {}

  ~TiledLayerBufferOGL();

  void Upload(const BasicTiledLayerBuffer* aMainMemoryTiledBuffer,
              const nsIntRegion& aNewValidRegion,
              const nsIntRegion& aInvalidateRegion,
              const gfxSize& aResolution);

  TiledTexture GetPlaceholderTile() const { return TiledTexture(); }

  const gfxSize& GetResolution() { return mResolution; }

protected:
  TiledTexture ValidateTile(TiledTexture aTile,
                            const nsIntPoint& aTileRect,
                            const nsIntRegion& dirtyRect);

  void ReleaseTile(TiledTexture aTile);

  void SwapTiles(TiledTexture& aTileA, TiledTexture& aTileB) {
    std::swap(aTileA, aTileB);
  }

private:
  nsRefPtr<gl::GLContext> mContext;
  const BasicTiledLayerBuffer* mMainMemoryTiledBuffer;
  gfxSize mResolution;

  void GetFormatAndTileForImageFormat(gfxASurface::gfxImageFormat aFormat,
                                      GLenum& aOutFormat,
                                      GLenum& aOutType);
};

class TiledThebesLayerOGL : public ShadowThebesLayer,
                            public LayerOGL,
                            public TiledLayerComposer
{
public:
  TiledThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~TiledThebesLayerOGL();

  
  void Destroy() {}
  Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources() { }

  
  virtual TiledLayerComposer* AsTiledLayerComposer() { return this; }
  virtual void DestroyFrontBuffer() {}
  void Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion)
  {
    NS_ABORT_IF_FALSE(false, "Not supported");
  }
  void PaintedTiledLayerBuffer(const BasicTiledLayerBuffer* mTiledBuffer);
  void ProcessUploadQueue();

  
  void RenderTile(const TiledTexture& aTile,
                  const gfx3DMatrix& aTransform,
                  const nsIntPoint& aOffset,
                  const nsIntRegion& aScreenRegion,
                  const nsIntPoint& aTextureOffset,
                  const nsIntSize& aTextureBounds,
                  Layer* aMaskLayer);

private:
  nsIntRegion                  mRegionToUpload;
  BasicTiledLayerBuffer        mMainMemoryTiledBuffer;
  TiledLayerBufferOGL          mVideoMemoryTiledBuffer;
  ReusableTileStoreOGL*        mReusableTileStore;
};

} 
} 

#endif
