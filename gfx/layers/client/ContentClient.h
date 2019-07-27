




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

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

class PaintedLayer;


































class ContentClient : public CompositableClient
{
public:
  




  static TemporaryRef<ContentClient> CreateContentClient(CompositableForwarder* aFwd);

  explicit ContentClient(CompositableForwarder* aForwarder)
  : CompositableClient(aForwarder)
  {}
  virtual ~ContentClient()
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) {};

  virtual void Clear() = 0;
  virtual RotatedContentBuffer::PaintState BeginPaintBuffer(PaintedLayer* aLayer,
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


class ContentClientBasic final : public ContentClient
                               , protected RotatedContentBuffer
{
public:
  ContentClientBasic();

  typedef RotatedContentBuffer::PaintState PaintState;
  typedef RotatedContentBuffer::ContentType ContentType;

  virtual void Clear() override { RotatedContentBuffer::Clear(); }
  virtual PaintState BeginPaintBuffer(PaintedLayer* aLayer,
                                      uint32_t aFlags) override
  {
    return RotatedContentBuffer::BeginPaint(aLayer, aFlags);
  }
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) override
  {
    return RotatedContentBuffer::BorrowDrawTargetForPainting(aPaintState, aIter);
  }
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) override
  {
    BorrowDrawTarget::ReturnDrawTarget(aReturned);
  }

  void DrawTo(PaintedLayer* aLayer,
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
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) override;

  virtual TextureInfo GetTextureInfo() const override
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

  virtual void Clear() override
  {
    RotatedContentBuffer::Clear();
    mTextureClient = nullptr;
    mTextureClientOnWhite = nullptr;
  }

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) override;

  virtual PaintState BeginPaintBuffer(PaintedLayer* aLayer,
                                      uint32_t aFlags) override
  {
    return RotatedContentBuffer::BeginPaint(aLayer, aFlags);
  }
  virtual gfx::DrawTarget* BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                       RotatedContentBuffer::DrawIterator* aIter = nullptr) override
  {
    return RotatedContentBuffer::BorrowDrawTargetForPainting(aPaintState, aIter);
  }
  virtual void ReturnDrawTargetToBuffer(gfx::DrawTarget*& aReturned) override
  {
    BorrowDrawTarget::ReturnDrawTarget(aReturned);
  }

  







  virtual void BeginPaint() override;
  virtual void EndPaint(nsTArray<ReadbackProcessor::Update>* aReadbackUpdates = nullptr) override;

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy) override;

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) override;

  
  virtual const nsIntRect& BufferRect() const
  {
    return RotatedContentBuffer::BufferRect();
  }
  virtual const nsIntPoint& BufferRotation() const
  {
    return RotatedContentBuffer::BufferRotation();
  }

  virtual void CreateBuffer(ContentType aType, const nsIntRect& aRect, uint32_t aFlags,
                            RefPtr<gfx::DrawTarget>* aBlackDT, RefPtr<gfx::DrawTarget>* aWhiteDT) override;

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
  {}

  virtual ~ContentClientDoubleBuffered() {}

  virtual void Clear() override
  {
    ContentClientRemoteBuffer::Clear();
    mFrontClient = nullptr;
    mFrontClientOnWhite = nullptr;
  }

  virtual void Updated(const nsIntRegion& aRegionToDraw,
                       const nsIntRegion& aVisibleRegion,
                       bool aDidSelfCopy) override;

  virtual void SwapBuffers(const nsIntRegion& aFrontUpdatedRegion) override;

  virtual void BeginPaint() override;

  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) override;

  virtual void EnsureBackBufferIfFrontBuffer() override;

  virtual TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(CompositableType::CONTENT_DOUBLE, mTextureFlags);
  }

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) override;
protected:
  virtual void DestroyFrontBuffer() override;

private:
  void UpdateDestinationFrom(const RotatedBuffer& aSource,
                             const nsIntRegion& aUpdateRegion);

  virtual void AbortTextureClientCreation() override
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
  }
  virtual ~ContentClientSingleBuffered() {}

  virtual void FinalizeFrame(const nsIntRegion& aRegionToDraw) override;

  virtual TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(CompositableType::CONTENT_SINGLE, mTextureFlags);
  }
};

}
}

#endif
