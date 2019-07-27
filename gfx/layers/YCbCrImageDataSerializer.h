




#ifndef MOZILLA_LAYERS_BLOBYCBCRSURFACE_H
#define MOZILLA_LAYERS_BLOBYCBCRSURFACE_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "ImageTypes.h"                 
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          

namespace mozilla {
namespace gfx {
class DataSourceSurface;
}

namespace layers {

class Image;






class YCbCrImageDataDeserializerBase
{
public:
  bool IsValid() const { return mIsValid; }

  


  uint8_t* GetYData();
  


  uint8_t* GetCbData();
  


  uint8_t* GetCrData();

  


  uint32_t GetYStride();
  


  uint32_t GetCbCrStride();

  


  gfx::IntSize GetYSize();

  


  gfx::IntSize GetCbCrSize();

  


  StereoMode GetStereoMode();

  


  uint8_t* GetData();

  




  static size_t ComputeMinBufferSize(const gfx::IntSize& aYSize,
                                     uint32_t aYStride,
                                     const gfx::IntSize& aCbCrSize,
                                     uint32_t aCbCrStride);
  static size_t ComputeMinBufferSize(const gfx::IntSize& aYSize,
                                     const gfx::IntSize& aCbCrSize);
  static size_t ComputeMinBufferSize(uint32_t aSize);

protected:
  YCbCrImageDataDeserializerBase(uint8_t* aData, size_t aDataSize)
    : mData (aData)
    , mDataSize(aDataSize)
    , mIsValid(false)
  {}

  void Validate();

  uint8_t* mData;
  size_t mDataSize;
  bool mIsValid;
};













class MOZ_STACK_CLASS YCbCrImageDataSerializer : public YCbCrImageDataDeserializerBase
{
public:
  YCbCrImageDataSerializer(uint8_t* aData, size_t aDataSize)
    : YCbCrImageDataDeserializerBase(aData, aDataSize)
  {
    
    mIsValid = !!mData;
  }

  




  void InitializeBufferInfo(uint32_t aYOffset,
                            uint32_t aCbOffset,
                            uint32_t aCrOffset,
                            uint32_t aYStride,
                            uint32_t aCbCrStride,
                            const gfx::IntSize& aYSize,
                            const gfx::IntSize& aCbCrSize,
                            StereoMode aStereoMode);
  void InitializeBufferInfo(uint32_t aYStride,
                            uint32_t aCbCrStride,
                            const gfx::IntSize& aYSize,
                            const gfx::IntSize& aCbCrSize,
                            StereoMode aStereoMode);
  void InitializeBufferInfo(const gfx::IntSize& aYSize,
                            const gfx::IntSize& aCbCrSize,
                            StereoMode aStereoMode);
  bool CopyData(const uint8_t* aYData,
                const uint8_t* aCbData, const uint8_t* aCrData,
                gfx::IntSize aYSize, uint32_t aYStride,
                gfx::IntSize aCbCrSize, uint32_t aCbCrStride,
                uint32_t aYSkip, uint32_t aCbCrSkip);
};













class MOZ_STACK_CLASS YCbCrImageDataDeserializer : public YCbCrImageDataDeserializerBase
{
public:
  YCbCrImageDataDeserializer(uint8_t* aData, size_t aDataSize)
    : YCbCrImageDataDeserializerBase(aData, aDataSize)
  {
    Validate();
  }

  




  already_AddRefed<gfx::DataSourceSurface> ToDataSourceSurface();
};

} 
} 

#endif
