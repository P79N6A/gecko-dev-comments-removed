




#ifndef GONKIOSURFACEIMAGE_H
#define GONKIOSURFACEIMAGE_H

#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/LayersSurfaces.h"
#include "ImageLayers.h"

#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {










class GraphicBufferLocked {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GraphicBufferLocked)

public:
  GraphicBufferLocked(SurfaceDescriptor aGraphicBuffer)
    : mSurfaceDescriptor(aGraphicBuffer)
  {}

  virtual ~GraphicBufferLocked() {}

  virtual void Unlock() {}

  SurfaceDescriptor GetSurfaceDescriptor()
  {
    return mSurfaceDescriptor;
  }

protected:
  SurfaceDescriptor mSurfaceDescriptor;
};

class THEBES_API GonkIOSurfaceImage : public Image {
public:
  struct Data {
    nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
    gfxIntSize mPicSize;
  };
  GonkIOSurfaceImage()
    : Image(NULL, GONK_IO_SURFACE)
    , mSize(0, 0)
    {}

  virtual ~GonkIOSurfaceImage()
  {
    mGraphicBuffer->Unlock();
  }

  virtual void SetData(const Data& aData)
  {
    mGraphicBuffer = aData.mGraphicBuffer;
    mSize = aData.mPicSize;
  }

  virtual gfxIntSize GetSize()
  {
    return mSize;
  }

  virtual already_AddRefed<gfxASurface> GetAsSurface()
  {
    
    return nullptr;
  }

  void* GetNativeBuffer()
  {
    return GrallocBufferActor::GetFrom(GetSurfaceDescriptor())->getNativeBuffer();
  }

  SurfaceDescriptor GetSurfaceDescriptor()
  {
    return mGraphicBuffer->GetSurfaceDescriptor();
  }

private:
  nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
  gfxIntSize mSize;
};

} 
} 
#endif

#endif 
