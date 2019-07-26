



#include "mozilla/layers/PLayersChild.h"
#include "TiledThebesLayerOGL.h"
#include "ReusableTileStoreOGL.h"
#include "BasicTiledThebesLayer.h"
#include "gfxImageSurface.h"

namespace mozilla {
namespace layers {

using mozilla::gl::GLContext;

TiledLayerBufferOGL::~TiledLayerBufferOGL()
{
  if (mRetainedTiles.Length() == 0)
    return;

  mContext->MakeCurrent();
  for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
    if (mRetainedTiles[i] == GetPlaceholderTile())
      continue;
    mContext->fDeleteTextures(1, &mRetainedTiles[i].mTextureHandle);
  }
}

void
TiledLayerBufferOGL::ReleaseTile(TiledTexture aTile)
{
  
  if (aTile == GetPlaceholderTile())
    return;
  mContext->fDeleteTextures(1, &aTile.mTextureHandle);
}

void
TiledLayerBufferOGL::Upload(const BasicTiledLayerBuffer* aMainMemoryTiledBuffer,
                            const nsIntRegion& aNewValidRegion,
                            const nsIntRegion& aInvalidateRegion,
                            const gfxSize& aResolution)
{
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Upload %i, %i, %i, %i\n", aInvalidateRegion.GetBounds().x, aInvalidateRegion.GetBounds().y, aInvalidateRegion.GetBounds().width, aInvalidateRegion.GetBounds().height);
  long start = PR_IntervalNow();
#endif

  mFrameResolution = aResolution;
  mMainMemoryTiledBuffer = aMainMemoryTiledBuffer;
  mContext->MakeCurrent();
  Update(aNewValidRegion, aInvalidateRegion);
  mMainMemoryTiledBuffer = nullptr;
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 10) {
    printf_stderr("Time to upload %i\n", PR_IntervalNow() - start);
  }
#endif
}

void
TiledLayerBufferOGL::GetFormatAndTileForImageFormat(gfxASurface::gfxImageFormat aFormat,
                                                    GLenum& aOutFormat,
                                                    GLenum& aOutType)
{
  if (aFormat == gfxASurface::ImageFormatRGB16_565) {
    aOutFormat = LOCAL_GL_RGB;
    aOutType = LOCAL_GL_UNSIGNED_SHORT_5_6_5;
  } else {
    aOutFormat = LOCAL_GL_RGBA;
    aOutType = LOCAL_GL_UNSIGNED_BYTE;
  }
}

TiledTexture
TiledLayerBufferOGL::ValidateTile(TiledTexture aTile,
                                  const nsIntPoint& aTileOrigin,
                                  const nsIntRegion& aDirtyRect)
{
#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  printf_stderr("Upload tile %i, %i\n", aTileOrigin.x, aTileOrigin.y);
  long start = PR_IntervalNow();
#endif
  if (aTile == GetPlaceholderTile()) {
    mContext->fGenTextures(1, &aTile.mTextureHandle);
    mContext->fBindTexture(LOCAL_GL_TEXTURE_2D, aTile.mTextureHandle);
    mContext->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
    mContext->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
    mContext->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
    mContext->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
  } else {
    mContext->fBindTexture(LOCAL_GL_TEXTURE_2D, aTile.mTextureHandle);
  }

  nsRefPtr<gfxReusableSurfaceWrapper> reusableSurface = mMainMemoryTiledBuffer->GetTile(aTileOrigin).mSurface.get();
  GLenum format, type;
  GetFormatAndTileForImageFormat(reusableSurface->Format(), format, type);

  const unsigned char* buf = reusableSurface->GetReadOnlyData();
  mContext->fTexImage2D(LOCAL_GL_TEXTURE_2D, 0, format,
                       GetTileLength(), GetTileLength(), 0,
                       format, type, buf);

  aTile.mFormat = format;

#ifdef GFX_TILEDLAYER_PREF_WARNINGS
  if (PR_IntervalNow() - start > 1) {
    printf_stderr("Tile Time to upload %i\n", PR_IntervalNow() - start);
  }
#endif
  return aTile;
}

TiledThebesLayerOGL::TiledThebesLayerOGL(LayerManagerOGL *aManager)
  : ShadowThebesLayer(aManager, nullptr)
  , LayerOGL(aManager)
  , mVideoMemoryTiledBuffer(aManager->gl())
  , mReusableTileStore(nullptr)
{
  mImplData = static_cast<LayerOGL*>(this);
}

TiledThebesLayerOGL::~TiledThebesLayerOGL()
{
  mMainMemoryTiledBuffer.ReadUnlock();
  if (mReusableTileStore)
    delete mReusableTileStore;
}

void
TiledThebesLayerOGL::MemoryPressure()
{
  if (mReusableTileStore) {
    delete mReusableTileStore;
    mReusableTileStore = new ReusableTileStoreOGL(gl(), 1);
  }
}

void
TiledThebesLayerOGL::PaintedTiledLayerBuffer(const BasicTiledLayerBuffer* mTiledBuffer)
{
  mMainMemoryTiledBuffer.ReadUnlock();
  mMainMemoryTiledBuffer = *mTiledBuffer;
  
  delete mTiledBuffer;
  mRegionToUpload.Or(mRegionToUpload, mMainMemoryTiledBuffer.GetPaintedRegion());
  mMainMemoryTiledBuffer.ClearPaintedRegion();
}

void
TiledThebesLayerOGL::ProcessUploadQueue()
{
  if (mRegionToUpload.IsEmpty())
    return;

  
  
  
  if (mReusableTileStore && mIsFixedPosition) {
    delete mReusableTileStore;
    mReusableTileStore = nullptr;
  } else if (!mReusableTileStore && !mIsFixedPosition) {
    
    mReusableTileStore = new ReusableTileStoreOGL(gl(), 2);
  }

  gfxSize resolution(1, 1);
  if (mReusableTileStore) {
    
    
    
    
    
    for (ContainerLayer* parent = GetParent(); parent; parent = parent->GetParent()) {
      const FrameMetrics& metrics = parent->GetFrameMetrics();
      resolution.width *= metrics.mResolution.width;
      resolution.height *= metrics.mResolution.height;
    }

    mReusableTileStore->HarvestTiles(this,
                                     &mVideoMemoryTiledBuffer,
                                     mVideoMemoryTiledBuffer.GetValidRegion(),
                                     mMainMemoryTiledBuffer.GetValidRegion(),
                                     mVideoMemoryTiledBuffer.GetFrameResolution(),
                                     resolution);
  }

  
  
  mRegionToUpload.And(mRegionToUpload, mMainMemoryTiledBuffer.GetValidRegion());

  mVideoMemoryTiledBuffer.Upload(&mMainMemoryTiledBuffer,
                                 mMainMemoryTiledBuffer.GetValidRegion(),
                                 mRegionToUpload, resolution);
  mVideoMemoryTiledBuffer.SetResolution(mMainMemoryTiledBuffer.GetResolution());
  mValidRegion = mVideoMemoryTiledBuffer.GetValidRegion();

  mMainMemoryTiledBuffer.ReadUnlock();
  
  
  
  
  
  mMainMemoryTiledBuffer = BasicTiledLayerBuffer();
  mRegionToUpload = nsIntRegion();

}

void
TiledThebesLayerOGL::RenderTile(const TiledTexture& aTile,
                                const gfx3DMatrix& aTransform,
                                const nsIntPoint& aOffset,
                                const nsIntRegion& aScreenRegion,
                                const nsIntPoint& aTextureOffset,
                                const nsIntSize& aTextureBounds,
                                Layer* aMaskLayer)
{
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, aTile.mTextureHandle);
    ShaderProgramOGL *program;
    if (aTile.mFormat == LOCAL_GL_RGB) {
      program = mOGLManager->GetProgram(gl::RGBXLayerProgramType, aMaskLayer);
    } else {
      program = mOGLManager->GetProgram(gl::BGRALayerProgramType, aMaskLayer);
    }
    program->Activate();
    program->SetTextureUnit(0);
    program->SetLayerOpacity(GetEffectiveOpacity());
    program->SetLayerTransform(aTransform);
    program->SetRenderOffset(aOffset);
    program->LoadMask(aMaskLayer);

    nsIntRegionRectIterator it(aScreenRegion);
    for (const nsIntRect* rect = it.Next(); rect != nullptr; rect = it.Next()) {
      nsIntRect textureRect(rect->x - aTextureOffset.x, rect->y - aTextureOffset.y,
                            rect->width, rect->height);
      program->SetLayerQuadRect(*rect);
      mOGLManager->BindAndDrawQuadWithTextureRect(program,
                                                  textureRect,
                                                  aTextureBounds);
    }
}

void
TiledThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer, const nsIntPoint& aOffset)
{
  gl()->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  ProcessUploadQueue();

  Layer* maskLayer = GetMaskLayer();

  
  if (mReusableTileStore) {
    mReusableTileStore->DrawTiles(this,
                                  mVideoMemoryTiledBuffer.GetValidRegion(),
                                  mVideoMemoryTiledBuffer.GetFrameResolution(),
                                  GetEffectiveTransform(), aOffset, maskLayer);
  }

  
  const nsIntRegion& visibleRegion = GetEffectiveVisibleRegion();
  const nsIntRect visibleRect = visibleRegion.GetBounds();
  float resolution = mVideoMemoryTiledBuffer.GetResolution();
  gfx3DMatrix transform = GetEffectiveTransform();
  transform.Scale(1/resolution, 1/resolution, 1);

  uint32_t rowCount = 0;
  uint32_t tileX = 0;
  for (int32_t x = visibleRect.x; x < visibleRect.x + visibleRect.width;) {
    rowCount++;
    int32_t tileStartX = mVideoMemoryTiledBuffer.GetTileStart(x);
    int32_t w = mVideoMemoryTiledBuffer.GetScaledTileLength() - tileStartX;
    if (x + w > visibleRect.x + visibleRect.width)
      w = visibleRect.x + visibleRect.width - x;
    int tileY = 0;
    for (int32_t y = visibleRect.y; y < visibleRect.y + visibleRect.height;) {
      int32_t tileStartY = mVideoMemoryTiledBuffer.GetTileStart(y);
      int32_t h = mVideoMemoryTiledBuffer.GetScaledTileLength() - tileStartY;
      if (y + h > visibleRect.y + visibleRect.height)
        h = visibleRect.y + visibleRect.height - y;

      TiledTexture tileTexture = mVideoMemoryTiledBuffer.
        GetTile(nsIntPoint(mVideoMemoryTiledBuffer.RoundDownToTileEdge(x),
                           mVideoMemoryTiledBuffer.RoundDownToTileEdge(y)));
      if (tileTexture != mVideoMemoryTiledBuffer.GetPlaceholderTile()) {
        nsIntRegion tileDrawRegion = nsIntRegion(nsIntRect(x, y, w, h));
        tileDrawRegion.And(tileDrawRegion, mValidRegion);
        tileDrawRegion.ScaleRoundOut(resolution, resolution);

        nsIntPoint tileOffset((x - tileStartX) * resolution,
                              (y - tileStartY) * resolution);
        uint32_t tileSize = mVideoMemoryTiledBuffer.GetTileLength();
        RenderTile(tileTexture, transform, aOffset, tileDrawRegion,
                   tileOffset, nsIntSize(tileSize, tileSize), maskLayer);
      }
      tileY++;
      y += h;
    }
    tileX++;
    x += w;
  }
}

} 
} 
