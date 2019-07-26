




#ifndef MOZILLA_GFX_CONTENTCLIENT_H
#define MOZILLA_GFX_CONTENTCLIENT_H

#include <stdint.h>                     
#include "ThebesLayerBuffer.h"          
#include "gfxTypes.h"
#include "gfxPlatform.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   

class gfxContext;
struct gfxMatrix;
class gfxASurface;

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

class BasicLayerManager;
class ThebesLayer;




































class ContentClient : public CompositableClient
{
public:
  




  static TemporaryRef<ContentClient> CreateContentClient(CompositableForwarder* aFwd);

  ContentClient(CompositableForwarder* aForwarder)
  : CompositableClient(aForwarder)
  {}
  virtual ~ContentClient()
  {}


  virtual void Clear() = 0;
  virtual ThebesLayerBuffer::PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                                         ThebesLayerBuffer::ContentType aContentType,
                                                         uint32_t aFlags) = 0;

  
  
  
  
  virtual void SyncFrontBufferToBackBuffer() {}

  
  
  
  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) {}

  
  virtual void BeginPaint() {}
  virtual void EndPaint() {}
};




class ContentClientRemote : public ContentClient
{
public:
  ContentClientRemote(CompositableForwarder* aForwarder)
    : ContentClient(aForwarder)
  {}

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy) = 0;
};


class ContentClientBasic : public ContentClient
                         , protected ThebesLayerBuffer
{
public:
  ContentClientBasic(CompositableForwarder* aForwarder,
                     BasicLayerManager* aManager);

  typedef ThebesLayerBuffer::PaintState PaintState;
  typedef ThebesLayerBuffer::ContentType ContentType;

  virtual void Clear() { ThebesLayerBuffer::Clear(); }
  PaintState BeginPaintBuffer(ThebesLayer* aLayer, ContentType aContentType,
                              uint32_t aFlags)
  {
    return ThebesLayerBuffer::BeginPaint(aLayer, aContentType, aFlags);
  }

  void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity,
              gfxASurface* aMask, const gfxMatrix* aMaskTransform)
  {
    ThebesLayerBuffer::DrawTo(aLayer, aTarget, aOpacity, aMask, aMaskTransform);
  }

  virtual void CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
                            gfxASurface** aBlackSurface, gfxASurface** aWhiteSurface,
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) MOZ_OVERRIDE;
  virtual bool SupportsAzureContent() const;

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    MOZ_CRASH("Should not be called on non-remote ContentClient");
  }

  virtual void OnActorDestroy() MOZ_OVERRIDE {}

private:
  BasicLayerManager* mManager;
};
















class ContentClientRemoteBuffer : public ContentClientRemote
                                , protected ThebesLayerBuffer
{
  using ThebesLayerBuffer::BufferRect;
  using ThebesLayerBuffer::BufferRotation;
public:
  ContentClientRemoteBuffer(CompositableForwarder* aForwarder)
    : ContentClientRemote(aForwarder)
    , ThebesLayerBuffer(ContainsVisibleBounds)
    , mDeprecatedTextureClient(nullptr)
    , mIsNewBuffer(false)
    , mFrontAndBackBufferDiffer(false)
    , mContentType(GFX_CONTENT_COLOR_ALPHA)
  {}

  typedef ThebesLayerBuffer::PaintState PaintState;
  typedef ThebesLayerBuffer::ContentType ContentType;

  virtual void Clear() { ThebesLayerBuffer::Clear(); }
  PaintState BeginPaintBuffer(ThebesLayer* aLayer, ContentType aContentType,
                              uint32_t aFlags)
  {
    return ThebesLayerBuffer::BeginPaint(aLayer, aContentType, aFlags);
  }

  







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

  virtual void CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
                            gfxASurface** aBlackSurface, gfxASurface** aWhiteSurface,
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) MOZ_OVERRIDE;

  virtual bool SupportsAzureContent() const MOZ_OVERRIDE;

  void DestroyBuffers();

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

  virtual void OnActorDestroy() MOZ_OVERRIDE;

protected:
  virtual nsIntRegion GetUpdatedRegion(const nsIntRegion& aRegionToDraw,
                                       const nsIntRegion& aVisibleRegion,
                                       bool aDidSelfCopy);

  
  void BuildDeprecatedTextureClients(ContentType aType,
                                     const nsIntRect& aRect,
                                     uint32_t aFlags);

  
  
  virtual void CreateFrontBufferAndNotify(const nsIntRect& aBufferRect) = 0;
  virtual void DestroyFrontBuffer() {}
  
  
  virtual void LockFrontBuffer() {}

  bool CreateAndAllocateDeprecatedTextureClient(RefPtr<DeprecatedTextureClient>& aClient);

  RefPtr<DeprecatedTextureClient> mDeprecatedTextureClient;
  RefPtr<DeprecatedTextureClient> mDeprecatedTextureClientOnWhite;
  
  
  nsTArray<RefPtr<DeprecatedTextureClient> > mOldTextures;

  TextureInfo mTextureInfo;
  bool mIsNewBuffer;
  bool mFrontAndBackBufferDiffer;
  gfx::IntSize mSize;
  ContentType mContentType;
};












class ContentClientDoubleBuffered : public ContentClientRemoteBuffer
{
public:
  ContentClientDoubleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemoteBuffer(aFwd)
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

  virtual void OnActorDestroy() MOZ_OVERRIDE;

private:
  void UpdateDestinationFrom(const RotatedBuffer& aSource,
                             const nsIntRegion& aUpdateRegion);

  RefPtr<DeprecatedTextureClient> mFrontClient;
  RefPtr<DeprecatedTextureClient> mFrontClientOnWhite;
  nsIntRegion mFrontUpdatedRegion;
  nsIntRect mFrontBufferRect;
  nsIntPoint mFrontBufferRotation;
};









class ContentClientSingleBuffered : public ContentClientRemoteBuffer
{
public:
  ContentClientSingleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemoteBuffer(aFwd)
  {
    mTextureInfo.mCompositableType = BUFFER_CONTENT;    
  }
  ~ContentClientSingleBuffered();

  virtual void SyncFrontBufferToBackBuffer() MOZ_OVERRIDE;

protected:
  virtual void CreateFrontBufferAndNotify(const nsIntRect& aBufferRect) MOZ_OVERRIDE;
};







class ContentClientIncremental : public ContentClientRemote
{
public:
  ContentClientIncremental(CompositableForwarder* aFwd)
    : ContentClientRemote(aFwd)
    , mContentType(GFX_CONTENT_COLOR_ALPHA)
    , mHasBuffer(false)
    , mHasBufferOnWhite(false)
  {
    mTextureInfo.mCompositableType = BUFFER_CONTENT_INC;
  }

  typedef ThebesLayerBuffer::PaintState PaintState;
  typedef ThebesLayerBuffer::ContentType ContentType;

  virtual TextureInfo GetTextureInfo() const
  {
    return mTextureInfo;
  }

  virtual void Clear()
  {
    mBufferRect.SetEmpty();
    mHasBuffer = false;
    mHasBufferOnWhite = false;
  }
  virtual ThebesLayerBuffer::PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                                         ThebesLayerBuffer::ContentType aContentType,
                                                         uint32_t aFlags);

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy);

  virtual void EndPaint()
  {
    if (IsSurfaceDescriptorValid(mUpdateDescriptor)) {
      mForwarder->DestroySharedSurface(&mUpdateDescriptor);
    }
    if (IsSurfaceDescriptorValid(mUpdateDescriptorOnWhite)) {
      mForwarder->DestroySharedSurface(&mUpdateDescriptorOnWhite);
    }
  }

private:

  enum BufferType{
    BUFFER_BLACK,
    BUFFER_WHITE
  };

  void NotifyBufferCreated(ContentType aType, uint32_t aFlags)
  {
    mTextureInfo.mTextureFlags = aFlags | TEXTURE_DEALLOCATE_HOST;
    mContentType = aType;

    mForwarder->CreatedIncrementalBuffer(this,
                                         mTextureInfo,
                                         mBufferRect);

  }

  already_AddRefed<gfxASurface> GetUpdateSurface(BufferType aType, nsIntRegion& aUpdateRegion);

  TextureInfo mTextureInfo;
  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;

  SurfaceDescriptor mUpdateDescriptor;
  SurfaceDescriptor mUpdateDescriptorOnWhite;

  ContentType mContentType;

  bool mHasBuffer;
  bool mHasBufferOnWhite;
};

}
}

#endif
