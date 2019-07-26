




#ifndef GFX_BASICBUFFERS_H
#define GFX_BASICBUFFERS_H

#include "BasicLayersImpl.h"

#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {

class BasicThebesLayer;
class BasicThebesLayerBuffer : public ThebesLayerBuffer {
  typedef ThebesLayerBuffer Base;

public:
  BasicThebesLayerBuffer(BasicThebesLayer* aLayer)
    : Base(ContainsVisibleBounds)
    , mLayer(aLayer)
  {
  }

  virtual ~BasicThebesLayerBuffer()
  {}

  using Base::BufferRect;
  using Base::BufferRotation;

  




  void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity,
              Layer* aMaskLayer);

  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize, uint32_t aFlags);

  virtual TemporaryRef<mozilla::gfx::DrawTarget>
  CreateDrawTarget(const mozilla::gfx::IntSize& aSize,
                   mozilla::gfx::SurfaceFormat aFormat);

  


  void SetBackingBuffer(gfxASurface* aBuffer,
                        const nsIntRect& aRect, const nsIntPoint& aRotation)
  {
#ifdef DEBUG
    gfxIntSize prevSize = gfxIntSize(BufferRect().width, BufferRect().height);
    gfxIntSize newSize = aBuffer->GetSize();
    NS_ABORT_IF_FALSE(newSize == prevSize,
                      "Swapped-in buffer size doesn't match old buffer's!");
#endif
    nsRefPtr<gfxASurface> oldBuffer;
    oldBuffer = SetBuffer(aBuffer, aRect, aRotation);
  }

  void SetBackingBuffer(mozilla::gfx::DrawTarget* aBuffer,
                        const nsIntRect& aRect, const nsIntPoint& aRotation)
  {
    mozilla::gfx::IntSize prevSize = mozilla::gfx::IntSize(BufferRect().width, BufferRect().height);
    mozilla::gfx::IntSize newSize = aBuffer->GetSize();
    NS_ABORT_IF_FALSE(newSize == prevSize,
                      "Swapped-in buffer size doesn't match old buffer's!");
    RefPtr<mozilla::gfx::DrawTarget> oldBuffer;
    oldBuffer = SetDT(aBuffer, aRect, aRotation);
  }

  void SetBackingBufferAndUpdateFrom(
    gfxASurface* aBuffer,
    gfxASurface* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
    const nsIntRegion& aUpdateRegion);

  void SetBackingBufferAndUpdateFrom(
    mozilla::gfx::DrawTarget* aBuffer,
    mozilla::gfx::DrawTarget* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
    const nsIntRegion& aUpdateRegion);

  











  void ProvideBuffer(AutoOpenSurface* aProvider)
  {
    SetBufferProvider(aProvider);
  }
  void RevokeBuffer()
  {
    SetBufferProvider(nullptr);
  }

private:
  BasicThebesLayerBuffer(gfxASurface* aBuffer,
                         const nsIntRect& aRect, const nsIntPoint& aRotation)
    
    
    : ThebesLayerBuffer(ContainsVisibleBounds)
  {
    SetBuffer(aBuffer, aRect, aRotation);
  }

  BasicThebesLayerBuffer(mozilla::gfx::DrawTarget* aBuffer,
                         const nsIntRect& aRect, const nsIntPoint& aRotation)
    : ThebesLayerBuffer(ContainsVisibleBounds)
  {
    SetDT(aBuffer, aRect, aRotation);
  }


  BasicThebesLayer* mLayer;
};

class ShadowThebesLayerBuffer : public BasicThebesLayerBuffer
{
  typedef BasicThebesLayerBuffer Base;

public:
  ShadowThebesLayerBuffer()
    : Base(NULL)
  {
    MOZ_COUNT_CTOR(ShadowThebesLayerBuffer);
  }

  ~ShadowThebesLayerBuffer()
  {
    MOZ_COUNT_DTOR(ShadowThebesLayerBuffer);
  }

  









  void Swap(const nsIntRect& aNewRect, const nsIntPoint& aNewRotation,
            nsIntRect* aOldRect, nsIntPoint* aOldRotation)
  {
    *aOldRect = BufferRect();
    *aOldRotation = BufferRotation();

    nsRefPtr<gfxASurface> oldBuffer;
    oldBuffer = SetBuffer(nullptr, aNewRect, aNewRotation);
    MOZ_ASSERT(!oldBuffer);
  }

protected:
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType, const nsIntSize&, uint32_t)
  {
    NS_RUNTIMEABORT("ShadowThebesLayer can't paint content");
    return nullptr;
  }

  virtual TemporaryRef<mozilla::gfx::DrawTarget>
  CreateDrawTarget(const mozilla::gfx::IntSize&, mozilla::gfx::SurfaceFormat)
  {
    NS_RUNTIMEABORT("ShadowThebesLayer can't paint content");
    return nullptr;
  }
};

}
}

#endif
