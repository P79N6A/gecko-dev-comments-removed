




#ifndef GFX_BASICBUFFERS_H
#define GFX_BASICBUFFERS_H

#include "BasicLayersImpl.h"

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
  CreateBuffer(ContentType aType, const nsIntSize& aSize, PRUint32 aFlags);

  


  void SetBackingBuffer(gfxASurface* aBuffer,
                        const nsIntRect& aRect, const nsIntPoint& aRotation)
  {
    gfxIntSize prevSize = gfxIntSize(BufferRect().width, BufferRect().height);
    gfxIntSize newSize = aBuffer->GetSize();
    NS_ABORT_IF_FALSE(newSize == prevSize,
                      "Swapped-in buffer size doesn't match old buffer's!");
    nsRefPtr<gfxASurface> oldBuffer;
    oldBuffer = SetBuffer(aBuffer, aRect, aRotation);
  }

  void SetBackingBufferAndUpdateFrom(
    gfxASurface* aBuffer,
    gfxASurface* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
    const nsIntRegion& aUpdateRegion);

  











  void MapBuffer(gfxASurface* aBuffer)
  {
    SetBuffer(aBuffer);
  }
  void UnmapBuffer()
  {
    SetBuffer(nullptr);
  }

private:
  BasicThebesLayerBuffer(gfxASurface* aBuffer,
                         const nsIntRect& aRect, const nsIntPoint& aRotation)
    
    
    : ThebesLayerBuffer(ContainsVisibleBounds)
  {
    SetBuffer(aBuffer, aRect, aRotation);
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
  CreateBuffer(ContentType, const nsIntSize&, PRUint32)
  {
    NS_RUNTIMEABORT("ShadowThebesLayer can't paint content");
    return nullptr;
  }
};

}
}

#endif
