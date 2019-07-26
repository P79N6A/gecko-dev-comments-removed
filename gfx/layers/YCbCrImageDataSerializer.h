




#ifndef MOZILLA_LAYERS_BLOBYCBCRSURFACE_H
#define MOZILLA_LAYERS_BLOBYCBCRSURFACE_H

#include "mozilla/DebugOnly.h"

#include "base/basictypes.h"
#include "Shmem.h"
#include "gfxPoint.h"

namespace mozilla {
namespace ipc {
  class Shmem;
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

  


  gfxIntSize GetYSize();

  


  gfxIntSize GetCbCrSize();

  


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

  




  static size_t ComputeMinBufferSize(const gfx::IntSize& aYSize,
                                     const gfx::IntSize& aCbCrSize);
  static size_t ComputeMinBufferSize(const gfxIntSize& aYSize,
                                     const gfxIntSize& aCbCrSize);
  static size_t ComputeMinBufferSize(uint32_t aSize);

  




  void InitializeBufferInfo(const gfx::IntSize& aYSize,
                            const gfx::IntSize& aCbCrSize);
  void InitializeBufferInfo(const gfxIntSize& aYSize,
                            const gfxIntSize& aCbCrSize);

  bool CopyData(const uint8_t* aYData,
                const uint8_t* aCbData, const uint8_t* aCrData,
                gfxIntSize aYSize, uint32_t aYStride,
                gfxIntSize aCbCrSize, uint32_t aCbCrStride,
                uint32_t aYSkip, uint32_t aCbCrSkip);
};













class MOZ_STACK_CLASS YCbCrImageDataDeserializer : public YCbCrImageDataDeserializerBase
{
public:
  YCbCrImageDataDeserializer(uint8_t* aData) : YCbCrImageDataDeserializerBase(aData) {}
};

} 
} 

#endif
