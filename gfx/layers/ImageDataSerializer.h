






#ifndef GFX_LAYERS_BLOBSURFACE_H
#define GFX_LAYERS_BLOBSURFACE_H

#include <stdint.h>                     
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          

namespace mozilla {
namespace gfx {
class DataSourceSurface;
class DrawTarget;
} 
} 

namespace mozilla {
namespace layers {

class ImageDataSerializerBase
{
public:
  bool IsValid() const { return mIsValid; }

  uint8_t* GetData();
  uint32_t GetStride() const;
  gfx::IntSize GetSize() const;
  gfx::SurfaceFormat GetFormat() const;
  already_AddRefed<gfx::DataSourceSurface> GetAsSurface();
  already_AddRefed<gfx::DrawTarget> GetAsDrawTarget(gfx::BackendType aBackend);

  static uint32_t ComputeMinBufferSize(gfx::IntSize aSize,
                                       gfx::SurfaceFormat aFormat);

protected:

  ImageDataSerializerBase(uint8_t* aData, size_t aDataSize)
    : mData(aData)
    , mDataSize(aDataSize)
    , mIsValid(false)
  {}

  void Validate();

  uint8_t* mData;
  size_t mDataSize;
  bool mIsValid;
};








class MOZ_STACK_CLASS ImageDataSerializer : public ImageDataSerializerBase
{
public:
  ImageDataSerializer(uint8_t* aData, size_t aDataSize)
    : ImageDataSerializerBase(aData, aDataSize)
  {
    
    mIsValid = !!mData;
  }
  void InitializeBufferInfo(gfx::IntSize aSize,
                            gfx::SurfaceFormat aFormat);
};





class MOZ_STACK_CLASS ImageDataDeserializer : public ImageDataSerializerBase
{
public:
  ImageDataDeserializer(uint8_t* aData, size_t aDataSize)
    : ImageDataSerializerBase(aData, aDataSize)
  {
    Validate();
  }

};

} 
} 

#endif
