




#ifndef GRALLOCIMAGES_H
#define GRALLOCIMAGES_H

#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/LayersSurfaces.h"
#include "ImageLayers.h"
#include "ImageContainer.h"

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






















class GrallocImage : public PlanarYCbCrImage {
  typedef PlanarYCbCrImage::Data Data;
  static uint32_t sColorIdMap[];

public:
  struct GrallocData {
      nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
      gfxIntSize mPicSize;
  };

  GrallocImage();

  virtual ~GrallocImage();

  



  virtual void SetData(const Data& aData);

  



  virtual void SetData(const GrallocData& aData);

  
  enum {
    
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_P            = 0x103,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
  };

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  void* GetNativeBuffer()
  {
    if (IsValid()) {
      return GrallocBufferActor::GetFrom(GetSurfaceDescriptor())->getNativeBuffer();
    } else {
      return nullptr;
    }
  }

  virtual bool IsValid() { return GetSurfaceDescriptor().type() != SurfaceDescriptor::T__None; }

  SurfaceDescriptor GetSurfaceDescriptor() {
    if (mGraphicBuffer.get()) {
      return mGraphicBuffer->GetSurfaceDescriptor();
    }
    return SurfaceDescriptor();
  }

private:
  bool mBufferAllocated;
  nsRefPtr<GraphicBufferLocked> mGraphicBuffer;
};

} 
} 
#endif

#endif 
