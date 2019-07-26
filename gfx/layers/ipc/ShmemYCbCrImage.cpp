




#include "ShmemYCbCrImage.h"

#define MOZ_ALIGN_WORD(x) (((x) + 3) & ~3)

using namespace mozilla::ipc;

namespace mozilla {
namespace layers {






















struct YCbCrBufferInfo
{
  uint32_t mYOffset;
  uint32_t mCbOffset;
  uint32_t mCrOffset;
  uint32_t mYWidth;
  uint32_t mYHeight;
  uint32_t mCbCrWidth;
  uint32_t mCbCrHeight;
};

static YCbCrBufferInfo* GetYCbCrBufferInfo(Shmem& aShmem, size_t aOffset)
{
  return reinterpret_cast<YCbCrBufferInfo*>(aShmem.get<uint8_t>() + aOffset);
}


uint8_t* ShmemYCbCrImage::GetYData()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return reinterpret_cast<uint8_t*>(info) + info->mYOffset;
}

uint8_t* ShmemYCbCrImage::GetCbData()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return reinterpret_cast<uint8_t*>(info) + info->mCbOffset;
}

uint8_t* ShmemYCbCrImage::GetCrData()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return reinterpret_cast<uint8_t*>(info) + info->mCrOffset;
}

uint32_t ShmemYCbCrImage::GetYStride()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return info->mYWidth;
}

uint32_t ShmemYCbCrImage::GetCbCrStride()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return info->mCbCrWidth;
}

gfxIntSize ShmemYCbCrImage::GetYSize()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return gfxIntSize(info->mYWidth, info->mYHeight);
}

gfxIntSize ShmemYCbCrImage::GetCbCrSize()
{
  YCbCrBufferInfo* info = GetYCbCrBufferInfo(mShmem, mOffset);
  return gfxIntSize(info->mCbCrWidth, info->mCbCrHeight);
}


bool ShmemYCbCrImage::Open(Shmem& aShmem, size_t aOffset)
{
    mShmem = aShmem;
    mOffset = aOffset;

    return IsValid();
}


static size_t ComputeOffset(uint32_t aHeight, uint32_t aStride)
{
  return MOZ_ALIGN_WORD(aHeight * aStride);
}


size_t ShmemYCbCrImage::ComputeMinBufferSize(const gfxIntSize& aYSize,
                                              const gfxIntSize& aCbCrSize)
{
  uint32_t yStride = aYSize.width;
  uint32_t CbCrStride = aCbCrSize.width;

  return ComputeOffset(aYSize.height, yStride)
         + 2 * ComputeOffset(aCbCrSize.height, CbCrStride)
         + MOZ_ALIGN_WORD(sizeof(YCbCrBufferInfo));
}

void ShmemYCbCrImage::InitializeBufferInfo(uint8_t* aBuffer,
                                           const gfxIntSize& aYSize,
                                           const gfxIntSize& aCbCrSize)
{
  YCbCrBufferInfo* info = reinterpret_cast<YCbCrBufferInfo*>(aBuffer);
  info->mYOffset = MOZ_ALIGN_WORD(sizeof(YCbCrBufferInfo));
  info->mCbOffset = info->mYOffset
                  + MOZ_ALIGN_WORD(aYSize.width * aYSize.height);
  info->mCrOffset = info->mCbOffset
                  + MOZ_ALIGN_WORD(aCbCrSize.width * aCbCrSize.height);

  info->mYWidth = aYSize.width;
  info->mYHeight = aYSize.height;
  info->mCbCrWidth = aCbCrSize.width;
  info->mCbCrHeight = aCbCrSize.height;
}

bool ShmemYCbCrImage::IsValid()
{
  if (mShmem == Shmem()) {
    return false;
  }
  size_t bufferInfoSize = MOZ_ALIGN_WORD(sizeof(YCbCrBufferInfo));
  if (mShmem.Size<uint8_t>() < bufferInfoSize ||
      GetYCbCrBufferInfo(mShmem, mOffset)->mYOffset != bufferInfoSize ||
      mShmem.Size<uint8_t>() < mOffset + ComputeMinBufferSize(GetYSize(),GetCbCrSize())) {
    return false;
  }
  return true;
}

bool ShmemYCbCrImage::CopyData(uint8_t* aYData, uint8_t* aCbData, uint8_t* aCrData,
                               gfxIntSize aYSize, uint32_t aYStride,
                               gfxIntSize aCbCrSize, uint32_t aCbCrStride)
{
  if (!IsValid() || GetYSize() != aYSize || GetCbCrSize() != aCbCrSize) {
    return false;
  }
  for (int i = 0; i < aYSize.height; i++) {
    memcpy(GetYData() + i * GetYStride(),
           aYData + i * aYStride,
           aYSize.width);
  }
  for (int i = 0; i < aCbCrSize.height; i++) {
    memcpy(GetCbData() + i * GetCbCrStride(),
           aCbData + i * aCbCrStride,
           aCbCrSize.width);
    memcpy(GetCrData() + i * GetCbCrStride(),
           aCrData + i * aCbCrStride,
           aCbCrSize.width);
  }
  return true;
}


} 
} 
