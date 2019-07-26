




#ifndef GRALLOCIMAGES_H
#define GRALLOCIMAGES_H

#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/LayersSurfaces.h"
#include "ImageLayers.h"

#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {






















class GrallocPlanarYCbCrImage : public PlanarYCbCrImage {
  typedef PlanarYCbCrImage::Data Data;

public:
  GrallocPlanarYCbCrImage();

  virtual ~GrallocPlanarYCbCrImage();

  



  virtual void SetData(const Data& aData);

  virtual uint32_t GetDataSize() { return 0; }

  virtual bool IsValid() { return mSurfaceDescriptor.type() != SurfaceDescriptor::T__None; }

  SurfaceDescriptor GetSurfaceDescriptor() {
    return mSurfaceDescriptor;
  }

private:
  SurfaceDescriptor mSurfaceDescriptor;
};

} 
} 
#endif

#endif 
