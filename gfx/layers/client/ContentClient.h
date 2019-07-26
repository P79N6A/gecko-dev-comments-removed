




#ifndef MOZILLA_GFX_CONTENTCLIENT_H
#define MOZILLA_GFX_CONTENTCLIENT_H

#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/CompositableClient.h"
#include "gfxReusableSurfaceWrapper.h"
#include "mozilla/layers/TextureClient.h"
#include "ThebesLayerBuffer.h"
#include "ipc/AutoOpenSurface.h"
#include "ipc/ShadowLayerChild.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {

class BasicLayerManager;




































class ContentClient : public CompositableClient
                    , protected ThebesLayerBuffer
{
public:
  




  static TemporaryRef<ContentClient> CreateContentClient(LayersBackend aBackendType,
                                                         CompositableForwarder* aFwd);

  ContentClient(CompositableForwarder* aForwarder)
  : CompositableClient(aForwarder)
  , ThebesLayerBuffer(ContainsVisibleBounds)
  {}
  virtual ~ContentClient()
  {}

  typedef ThebesLayerBuffer::PaintState PaintState;
  typedef ThebesLayerBuffer::ContentType ContentType;

  virtual void Clear() { ThebesLayerBuffer::Clear(); }
  PaintState BeginPaintBuffer(ThebesLayer* aLayer, ContentType aContentType,
                              uint32_t aFlags)
  {
    return ThebesLayerBuffer::BeginPaint(aLayer, aContentType, aFlags);
  }

  virtual void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity,
                      gfxASurface* aMask, const gfxMatrix* aMaskTransform)
  {
    ThebesLayerBuffer::DrawTo(aLayer, aTarget, aOpacity, aMask, aMaskTransform);
  }

  
  
  
  
  virtual void SyncFrontBufferToBackBuffer() {}

  
  
  
  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) {}

  
  virtual void BeginPaint() {}
  virtual void EndPaint() {}
};


class ContentClientBasic : public ContentClient
{
public:
  ContentClientBasic(CompositableForwarder* aForwarder,
                     BasicLayerManager* aManager);

  virtual already_AddRefed<gfxASurface> CreateBuffer(ContentType aType,
                                                     const nsIntRect& aRect,
                                                     uint32_t aFlags) MOZ_OVERRIDE;
  virtual TemporaryRef<gfx::DrawTarget>
    CreateDTBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags);

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    MOZ_NOT_REACHED("Should not be called on non-remote ContentClient");
    return TextureInfo();
  }


private:
  BasicLayerManager* mManager;
};















class ContentClientRemote : public ContentClient
{
  using ThebesLayerBuffer::BufferRect;
  using ThebesLayerBuffer::BufferRotation;
public:
  ContentClientRemote(CompositableForwarder* aForwarder)
    : ContentClient(aForwarder)
    , mTextureClient(nullptr)
    , mIsNewBuffer(false)
    , mFrontAndBackBufferDiffer(false)
    , mContentType(gfxASurface::CONTENT_COLOR_ALPHA)
  {}

  







  virtual void BeginPaint() MOZ_OVERRIDE;
  virtual void EndPaint() MOZ_OVERRIDE;

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy);

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) MOZ_OVERRIDE;

  
  virtual const nsIntRect& BufferRect() const
  {
    return ThebesLayerBuffer::BufferRect();
  }
  virtual const nsIntPoint& BufferRotation() const
  {
    return ThebesLayerBuffer::BufferRotation();
  }

  virtual already_AddRefed<gfxASurface> CreateBuffer(ContentType aType,
                                                     const nsIntRect& aRect,
                                                     uint32_t aFlags) MOZ_OVERRIDE;
  virtual TemporaryRef<gfx::DrawTarget> CreateDTBuffer(ContentType aType,
                                                       const nsIntRect& aRect,
                                                       uint32_t aFlags) MOZ_OVERRIDE;

  virtual bool SupportsAzureContent() const MOZ_OVERRIDE
  {
    return gfxPlatform::GetPlatform()->SupportsAzureContent();
  }

  void DestroyBuffers();

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

protected:
  


  void SetBackingBuffer(gfxASurface* aBuffer,
                        const nsIntRect& aRect,
                        const nsIntPoint& aRotation);

  virtual nsIntRegion GetUpdatedRegion(const nsIntRegion& aRegionToDraw,
                                       const nsIntRegion& aVisibleRegion,
                                       bool aDidSelfCopy);

  
  
  virtual void CreateFrontBufferAndNotify(const nsIntRect& aBufferRect) = 0;
  virtual void DestroyFrontBuffer() {}
  
  
  virtual void LockFrontBuffer() {}

  RefPtr<TextureClient> mTextureClient;
  
  
  nsTArray<RefPtr<TextureClient> > mOldTextures;

  TextureInfo mTextureInfo;
  bool mIsNewBuffer;
  bool mFrontAndBackBufferDiffer;
  gfx::IntSize mSize;
  ContentType mContentType;
};












class ContentClientDoubleBuffered : public ContentClientRemote
{
public:
  ContentClientDoubleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemote(aFwd)
  {
    mTextureInfo.mCompositableType = BUFFER_CONTENT_DIRECT;
  }
  ~ContentClientDoubleBuffered();

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) MOZ_OVERRIDE;

  virtual void SyncFrontBufferToBackBuffer() MOZ_OVERRIDE;

protected:
  virtual void CreateFrontBufferAndNotify(const nsIntRect& aBufferRect) MOZ_OVERRIDE;
  virtual void DestroyFrontBuffer() MOZ_OVERRIDE;
  virtual void LockFrontBuffer() MOZ_OVERRIDE;

private:
  
  
  ContentClientDoubleBuffered(gfxASurface* aBuffer,
                              const nsIntRect& aRect,
                              const nsIntPoint& aRotation)
    : ContentClientRemote(nullptr)
  {
    SetBuffer(aBuffer, aRect, aRotation);
  }

  void UpdateDestinationFrom(const RotatedBuffer& aSource,
                             const nsIntRegion& aUpdateRegion);

  RefPtr<TextureClient> mFrontClient;
  nsIntRegion mFrontUpdatedRegion;
  nsIntRect mFrontBufferRect;
  nsIntPoint mFrontBufferRotation;
};









class ContentClientSingleBuffered : public ContentClientRemote
{
public:
  ContentClientSingleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemote(aFwd)
  {
    mTextureInfo.mCompositableType = BUFFER_CONTENT;    
  }
  ~ContentClientSingleBuffered();

  virtual void SyncFrontBufferToBackBuffer() MOZ_OVERRIDE;

protected:
  virtual void CreateFrontBufferAndNotify(const nsIntRect& aBufferRect) MOZ_OVERRIDE;
};

}
}

#endif
