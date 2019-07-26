




#ifndef MOZILLA_LAYERS_BLOBYCBCRSURFACE_H
#define MOZILLA_LAYERS_BLOBYCBCRSURFACE_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "ImageTypes.h"                 
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "Units.h"

namespace mozilla {
namespace gfx {
class DataSourceSurface;
}

namespace layers {

class Image;






class YCbCrImageDataDeserializerBase
{
public:
  bool IsValid();

  


  uint8_t* GetYData();
  


  uint8_t* GetCbData();
  


  uint8_t* GetCrData();

  


  uint32_t GetYStride();
  


  uint32_t GetCbCrStride();

  


  LayerIntSize GetYSize();

  


  LayerIntSize GetCbCrSize();

  


  StereoMode GetStereoMode();

  


  uint8_t* GetData();
protected:
  YCbCrImageDataDeserializerBase(uint8_t* aData)
  : mData (aData) {}

  uint8_t* mData;
};













class MOZ_STACK_CLASS YCbCrImageDataSerializer : public YCbCrImageDataDeserializerBase
{
public:
  YCbCrImageDataSerializer(uint8_t* aData) : YCbCrImageDataDeserializerBase(aData) {}

  




  static size_t ComputeMinBufferSize(const LayerIntSize& aYSize,
                                     const LayerIntSize& aCbCrSize);
  static size_t ComputeMinBufferSize(const gfxIntSize& aYSize,
                                     const gfxIntSize& aCbCrSize);
  static size_t ComputeMinBufferSize(uint32_t aSize);

  




  void InitializeBufferInfo(const LayerIntSize& aYSize,
                            const LayerIntSize& aCbCrSize,
                            StereoMode aStereoMode);
  void InitializeBufferInfo(const gfxIntSize& aYSize,
                            const gfxIntSize& aCbCrSize,
                            StereoMode aStereoMode);

  bool CopyData(const uint8_t* aYData,
                const uint8_t* aCbData, const uint8_t* aCrData,
                LayerIntSize aYSize, uint32_t aYStride,
                LayerIntSize aCbCrSize, uint32_t aCbCrStride,
                uint32_t aYSkip, uint32_t aCbCrSkip);
};













class MOZ_STACK_CLASS YCbCrImageDataDeserializer : public YCbCrImageDataDeserializerBase
{
public:
  YCbCrImageDataDeserializer(uint8_t* aData) : YCbCrImageDataDeserializerBase(aData) {}

  




  TemporaryRef<gfx::DataSourceSurface> ToDataSourceSurface();
};

} 
} 

#endif
