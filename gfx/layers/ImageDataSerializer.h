






#ifndef GFX_LAYERS_BLOBSURFACE_H
#define GFX_LAYERS_BLOBSURFACE_H

#include "mozilla/gfx/Point.h"
#include "mozilla/RefPtr.h"

#include <stdint.h>

class gfxImageSurface;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
} 
} 

namespace mozilla {
namespace layers {

class ImageDataSerializerBase
{
public:
  bool IsValid() const;

  uint8_t* GetData();
  gfx::IntSize GetSize() const;
  gfx::SurfaceFormat GetFormat() const;
  TemporaryRef<gfx::DataSourceSurface> GetAsSurface();
  TemporaryRef<gfxImageSurface> GetAsThebesSurface();

protected:
  ImageDataSerializerBase(uint8_t* aData)
  : mData(aData) {}
  uint8_t* mData;
};








class MOZ_STACK_CLASS ImageDataSerializer : public ImageDataSerializerBase
{
public:
  ImageDataSerializer(uint8_t* aData) : ImageDataSerializerBase(aData) {}
  void InitializeBufferInfo(gfx::IntSize aSize,
                            gfx::SurfaceFormat aFormat);
  static uint32_t ComputeMinBufferSize(gfx::IntSize aSize,
                                       gfx::SurfaceFormat aFormat);
};





class MOZ_STACK_CLASS ImageDataDeserializer : public ImageDataSerializerBase
{
public:
  ImageDataDeserializer(uint8_t* aData) : ImageDataSerializerBase(aData) {}
};

} 
} 

#endif
