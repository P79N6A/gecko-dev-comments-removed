




#ifndef MOZILLA_GFX_CONTENTCLIENT_H
#define MOZILLA_GFX_CONTENTCLIENT_H

#include <stdint.h>                     
#include "RotatedBuffer.h"              
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
#include "ReadbackProcessor.h"          
#include "nsCOMPtr.h"                   
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   

class gfxContext;

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

  explicit ContentClient(CompositableForwarder* aForwarder)
  : CompositableClient(aForwarder)
  {}
  virtual ~ContentClient()
  {}


  virtual void Clear() = 0;
  virtual RotatedContentBuffer::PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                                            uint32_t aFlags) = 0;
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(RotatedContentBuffer::PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) = 0;
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) = 0;

  
  
  
  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) {}

  
  virtual void BeginPaint() {}
  virtual void EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates = nullptr);
};




class ContentClientRemote : public ContentClient
{
public:
  explicit ContentClientRemote(CompositableForwarder* aForwarder)
    : ContentClient(aForwarder)
  {}

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy) = 0;
};


class ContentClientBasic : public ContentClient
                         , protected RotatedContentBuffer
{
public:
  ContentClientBasic();

  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  virtual void Clear() { RotatedContentBuffer::Clear(); }
  virtual PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                      uint32_t aFlags) MOZ_OVERRIDE
  {
    return RotatedContentBuffer::BeginPaint(aLayer, aFlags);
  }
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) MOZ_OVERRIDE
  {
    return RotatedContentBuffer::BorrowDrawTargetForPainting(aPaintState, aIter);
  }
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) MOZ_OVERRIDE
  {
    BorrowDrawTarget::ReturnDrawTarget(aReturned);
  }

  void DrawTo(ThebesLayer* aLayer,
              gfx::DrawTarget* aTarget,
              float aOpacity,
              gfx::CompositionOp aOp,
              gfx::SourceSurface* aMask,
              const gfx::Matrix* aMaskTransform)
  {
    RotatedContentBuffer::DrawTo(aLayer, aTarget, aOpacity, aOp,
                                 aMask, aMaskTransform);
  }

  virtual void CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) MOZ_OVERRIDE;

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    MOZ_CRASH("Should not be called on non-remote ContentClient");
  }
};

















class ContentClientRemoteBuffer : public ContentClientRemote
                                , protected RotatedContentBuffer
{
  using RotatedContentBuffer::BufferRect;
  using RotatedContentBuffer::BufferRotation;
public:
  explicit ContentClientRemoteBuffer(CompositableForwarder* aForwarder)
    : ContentClientRemote(aForwarder)
    , RotatedContentBuffer(ContainsVisibleBounds)
    , mIsNewBuffer(false)
    , mFrontAndBackBufferDiffer(false)
    , mSurfaceFormat(gfx::SurfaceFormat::B8G8R8A8)
  {}

  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  virtual void Clear()
  {
    RotatedContentBuffer::Clear();
    mTextureClient = nullptr;
    mTextureClientOnWhite = nullptr;
  }

  virtual PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                      uint32_t aFlags) MOZ_OVERRIDE
  {
    return RotatedContentBuffer::BeginPaint(aLayer, aFlags);
  }
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) MOZ_OVERRIDE
  {
    return RotatedContentBuffer::BorrowDrawTargetForPainting(aPaintState, aIter);
  }
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) MOZ_OVERRIDE
  {
    BorrowDrawTarget::ReturnDrawTarget(aReturned);
  }

  







  virtual void BeginPaint() MOZ_OVERRIDE;
  virtual void EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates = nullptr) MOZ_OVERRIDE;

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy);

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) MOZ_OVERRIDE;

  
  virtual const nsIntRect& BufferRect() const
  {
    return RotatedContentBuffer::BufferRect();
  }
  virtual const nsIntPoint& BufferRotation() const
  {
    return RotatedContentBuffer::BufferRotation();
  }

  virtual void CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) MOZ_OVERRIDE;

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

protected:
  void DestroyBuffers();

  virtual nsIntRegion GetUpdatedRegion(const nsIntRegion& aRegionToDraw,
                                       const nsIntRegion& aVisibleRegion,
                                       bool aDidSelfCopy);

  void BuildTextureClients(gfx::SurfaceFormat aFormat,
                           const nsIntRect& aRect,
                           uint32_t aFlags);

  void CreateBackBuffer(const nsIntRect& aBufferRect);


  
  
  virtual void EnsureBackBufferIfFrontBuffer() {}

  
  
  virtual void DestroyFrontBuffer() {}

  virtual void AbortTextureClientCreation()
  {
    mTextureClient = nullptr;
    mTextureClientOnWhite = nullptr;
    mIsNewBuffer = false;
  }

  RefPtr<TextureClient> mTextureClient;
  RefPtr<TextureClient> mTextureClientOnWhite;
  
  
  
  nsTArray<RefPtr<TextureClient> > mOldTextures;

  TextureInfo mTextureInfo;
  bool mIsNewBuffer;
  bool mFrontAndBackBufferDiffer;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mSurfaceFormat;
};












class ContentClientDoubleBuffered : public ContentClientRemoteBuffer
{
public:
  explicit ContentClientDoubleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemoteBuffer(aFwd)
  {
    mTextureInfo.mCompositableType = CompositableType::CONTENT_DOUBLE;
  }
  virtual ~ContentClientDoubleBuffered() {}

  virtual void Clear() MOZ_OVERRIDE
  {
    ContentClientRemoteBuffer::Clear();
    mFrontClient = nullptr;
    mFrontClientOnWhite = nullptr;
  }

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy) MOZ_OVERRIDE;

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) MOZ_OVERRIDE;

  virtual void BeginPaint() MOZ_OVERRIDE;

  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) MOZ_OVERRIDE;

  virtual void EnsureBackBufferIfFrontBuffer() MOZ_OVERRIDE;

protected:
  virtual void DestroyFrontBuffer() MOZ_OVERRIDE;

private:
  void UpdateDestinationFrom(const RotatedBuffer& aSource,
                             const nsIntRegion& aUpdateRegion);

  virtual void AbortTextureClientCreation() MOZ_OVERRIDE
  {
    mTextureClient = nullptr;
    mTextureClientOnWhite = nullptr;
    mFrontClient = nullptr;
    mFrontClientOnWhite = nullptr;
  }

  RefPtr<TextureClient> mFrontClient;
  RefPtr<TextureClient> mFrontClientOnWhite;
  nsIntRegion mFrontUpdatedRegion;
  nsIntRect mFrontBufferRect;
  nsIntPoint mFrontBufferRotation;
};









class ContentClientSingleBuffered : public ContentClientRemoteBuffer
{
public:
  explicit ContentClientSingleBuffered(CompositableForwarder* aFwd)
    : ContentClientRemoteBuffer(aFwd)
  {
    mTextureInfo.mCompositableType = CompositableType::CONTENT_SINGLE;
  }
  virtual ~ContentClientSingleBuffered() {}

  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) MOZ_OVERRIDE;
};







class ContentClientIncremental : public ContentClientRemote
                               , public BorrowDrawTarget
{
public:
  explicit ContentClientIncremental(CompositableForwarder* aFwd)
    : ContentClientRemote(aFwd)
    , mContentType(gfxContentType::COLOR_ALPHA)
    , mHasBuffer(false)
    , mHasBufferOnWhite(false)
  {
    mTextureInfo.mCompositableType = CompositableType::BUFFER_CONTENT_INC;
  }

  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

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

  virtual PaintState BeginPaintBuffer(ThebesLayer* aLayer,
                                      uint32_t aFlags) MOZ_OVERRIDE;
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) MOZ_OVERRIDE;
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) MOZ_OVERRIDE
  {
    BorrowDrawTarget::ReturnDrawTarget(aReturned);
  }

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy);

  virtual void EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates = nullptr)
  {
    if (IsSurfaceDescriptorValid(mUpdateDescriptor)) {
      mForwarder->DestroySharedSurface(&mUpdateDescriptor);
    }
    if (IsSurfaceDescriptorValid(mUpdateDescriptorOnWhite)) {
      mForwarder->DestroySharedSurface(&mUpdateDescriptorOnWhite);
    }
    ContentClientRemote::EndPaint(aReadbackUpdates);
  }

private:

  enum BufferType{
    BUFFER_BLACK,
    BUFFER_WHITE
  };

  void NotifyBufferCreated(ContentType aType, TextureFlags aFlags);

  TemporaryRef<gfx::DrawTarget> GetUpdateSurface(BufferType aType,
                                                 const nsIntRegion& aUpdateRegion);

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
