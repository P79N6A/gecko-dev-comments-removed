




#include "SharedPlanarYCbCrImage.h"
#include <stddef.h>                     
#include <stdio.h>                      
#include "ISurfaceAllocator.h"          
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/Types.h"          
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/ImageClient.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "mozilla/mozalloc.h"           
#include "nsISupportsImpl.h"            

class gfxASurface;

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;

SharedPlanarYCbCrImage::SharedPlanarYCbCrImage(ImageClient* aCompositable)
: PlanarYCbCrImage(nullptr)
, mCompositable(aCompositable)
{
  mTextureClient = aCompositable->CreateBufferTextureClient(gfx::FORMAT_YUV);
  MOZ_COUNT_CTOR(SharedPlanarYCbCrImage);
}

SharedPlanarYCbCrImage::~SharedPlanarYCbCrImage() {
  MOZ_COUNT_DTOR(SharedPlanarYCbCrImage);
}


DeprecatedSharedPlanarYCbCrImage::~DeprecatedSharedPlanarYCbCrImage() {
  MOZ_COUNT_DTOR(DeprecatedSharedPlanarYCbCrImage);

  if (mAllocated) {
    SurfaceDescriptor desc;
    DropToSurfaceDescriptor(desc);
    mSurfaceAllocator->DestroySharedSurface(&desc);
  }
}

TextureClient*
SharedPlanarYCbCrImage::GetTextureClient()
{
  return mTextureClient.get();
}

uint8_t*
SharedPlanarYCbCrImage::GetBuffer()
{
  return mTextureClient->GetBuffer();
}

already_AddRefed<gfxASurface>
SharedPlanarYCbCrImage::GetAsSurface()
{
  if (!mTextureClient->IsAllocated()) {
    NS_WARNING("Can't get as surface");
    return nullptr;
  }
  return PlanarYCbCrImage::GetAsSurface();
}

void
SharedPlanarYCbCrImage::SetData(const PlanarYCbCrData& aData)
{
  
  
  
  if (!mTextureClient->IsAllocated()) {
    Data data = aData;
    if (!Allocate(data)) {
      NS_WARNING("SharedPlanarYCbCrImage::SetData failed to allocate");
      return;
    }
  }

  MOZ_ASSERT(mTextureClient->AsTextureClientYCbCr());

  if (!mTextureClient->AsTextureClientYCbCr()->UpdateYCbCr(aData)) {
    MOZ_ASSERT(false, "Failed to copy YCbCr data into the TextureClient");
    return;
  }

  
  
  
  mBufferSize = YCbCrImageDataSerializer::ComputeMinBufferSize(mData.mYSize,
                                                               mData.mCbCrSize);
  mSize = mData.mPicSize;

  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer());
  mData.mYChannel = serializer.GetYData();
  mData.mCbChannel = serializer.GetCbData();
  mData.mCrChannel = serializer.GetCrData();
  mTextureClient->MarkImmutable();
}



uint8_t*
SharedPlanarYCbCrImage::AllocateAndGetNewBuffer(uint32_t aSize)
{
  NS_ABORT_IF_FALSE(!mTextureClient->IsAllocated(), "This image already has allocated data");
  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(aSize);
  
  mBufferSize = size;

  
  bool status = mTextureClient->Allocate(mBufferSize);
  MOZ_ASSERT(status);
  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer());

  return serializer.GetData();
}

void
SharedPlanarYCbCrImage::SetDataNoCopy(const Data &aData)
{
  mData = aData;
  mSize = aData.mPicSize;
  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer());
  serializer.InitializeBufferInfo(aData.mYSize,
                                  aData.mCbCrSize,
                                  aData.mStereoMode);
}

uint8_t*
SharedPlanarYCbCrImage::AllocateBuffer(uint32_t aSize)
{
  NS_ABORT_IF_FALSE(!mTextureClient->IsAllocated(),
                    "This image already has allocated data");
  if (!mTextureClient->Allocate(aSize)) {
    return nullptr;
  }
  return mTextureClient->GetBuffer();
}

bool
SharedPlanarYCbCrImage::IsValid() {
  return mTextureClient->IsAllocated();
}

bool
SharedPlanarYCbCrImage::Allocate(PlanarYCbCrData& aData)
{
  NS_ABORT_IF_FALSE(!mTextureClient->IsAllocated(),
                    "This image already has allocated data");

  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(aData.mYSize,
                                                               aData.mCbCrSize);

  if (AllocateBuffer(static_cast<uint32_t>(size)) == nullptr) {
    return false;
  }

  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer());
  serializer.InitializeBufferInfo(aData.mYSize,
                                  aData.mCbCrSize,
                                  aData.mStereoMode);
  MOZ_ASSERT(serializer.IsValid());

  aData.mYChannel = serializer.GetYData();
  aData.mCbChannel = serializer.GetCbData();
  aData.mCrChannel = serializer.GetCrData();

  
  mData.mYChannel = aData.mYChannel;
  mData.mCbChannel = aData.mCbChannel;
  mData.mCrChannel = aData.mCrChannel;
  mData.mYSize = aData.mYSize;
  mData.mCbCrSize = aData.mCbCrSize;
  mData.mPicX = aData.mPicX;
  mData.mPicY = aData.mPicY;
  mData.mPicSize = aData.mPicSize;
  mData.mStereoMode = aData.mStereoMode;
  
  
  mData.mYSkip = 0;
  mData.mCbSkip = 0;
  mData.mCrSkip = 0;
  mData.mYStride = mData.mYSize.width;
  mData.mCbCrStride = mData.mCbCrSize.width;

  return true;
}

void
DeprecatedSharedPlanarYCbCrImage::SetData(const PlanarYCbCrData& aData)
{
  
  
  
  if (!mAllocated) {
    Data data = aData;
    if (!Allocate(data)) {
      return;
    }
  }

  
  
  
  mBufferSize = YCbCrImageDataSerializer::ComputeMinBufferSize(mData.mYSize,
                                                               mData.mCbCrSize);
  mSize = mData.mPicSize;

  YCbCrImageDataSerializer serializer(mShmem.get<uint8_t>());
  MOZ_ASSERT(aData.mCbSkip == aData.mCrSkip);
  if (!serializer.CopyData(aData.mYChannel, aData.mCbChannel, aData.mCrChannel,
                          aData.mYSize, aData.mYStride,
                          aData.mCbCrSize, aData.mCbCrStride,
                          aData.mYSkip, aData.mCbSkip)) {
    NS_WARNING("Failed to copy image data!");
  }
  mData.mYChannel = serializer.GetYData();
  mData.mCbChannel = serializer.GetCbData();
  mData.mCrChannel = serializer.GetCrData();
}



uint8_t*
DeprecatedSharedPlanarYCbCrImage::AllocateAndGetNewBuffer(uint32_t aSize)
{
  NS_ABORT_IF_FALSE(!mAllocated, "This image already has allocated data");
  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(aSize);
  
  mBufferSize = size;

  
  AllocateBuffer(mBufferSize);
  YCbCrImageDataSerializer serializer(mShmem.get<uint8_t>());

  return serializer.GetData();
}


void
DeprecatedSharedPlanarYCbCrImage::SetDataNoCopy(const Data &aData)
{
  mData = aData;
  mSize = aData.mPicSize;
  YCbCrImageDataSerializer serializer(mShmem.get<uint8_t>());
  serializer.InitializeBufferInfo(aData.mYSize,
                                  aData.mCbCrSize,
                                  aData.mStereoMode);
}

uint8_t* 
DeprecatedSharedPlanarYCbCrImage::AllocateBuffer(uint32_t aSize)
{
  NS_ABORT_IF_FALSE(!mAllocated, "This image already has allocated data");
  SharedMemory::SharedMemoryType shmType = OptimalShmemType();
  if (!mSurfaceAllocator->AllocUnsafeShmem(aSize, shmType, &mShmem)) {
    return nullptr;
  }
  mAllocated = true;
  return mShmem.get<uint8_t>();
}


bool
DeprecatedSharedPlanarYCbCrImage::Allocate(PlanarYCbCrData& aData)
{
  NS_ABORT_IF_FALSE(!mAllocated, "This image already has allocated data");

  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(aData.mYSize,
                                                               aData.mCbCrSize);

  if (AllocateBuffer(static_cast<uint32_t>(size)) == nullptr) {
    return false;
  }

  YCbCrImageDataSerializer serializer(mShmem.get<uint8_t>());
  serializer.InitializeBufferInfo(aData.mYSize,
                                  aData.mCbCrSize,
                                  aData.mStereoMode);
  if (!serializer.IsValid() || mShmem.Size<uint8_t>() < size) {
    mSurfaceAllocator->DeallocShmem(mShmem);
    return false;
  }

  aData.mYChannel = serializer.GetYData();
  aData.mCbChannel = serializer.GetCbData();
  aData.mCrChannel = serializer.GetCrData();

  
  mData.mYChannel = aData.mYChannel;
  mData.mCbChannel = aData.mCbChannel;
  mData.mCrChannel = aData.mCrChannel;
  mData.mYSize = aData.mYSize;
  mData.mCbCrSize = aData.mCbCrSize;
  mData.mPicX = aData.mPicX;
  mData.mPicY = aData.mPicY;
  mData.mPicSize = aData.mPicSize;
  mData.mStereoMode = aData.mStereoMode;
  
  
  mData.mYSkip = 0;
  mData.mCbSkip = 0;
  mData.mCrSkip = 0;
  mData.mYStride = mData.mYSize.width;
  mData.mCbCrStride = mData.mCbCrSize.width;

  mAllocated = true;
  return true;
}

bool
DeprecatedSharedPlanarYCbCrImage::ToSurfaceDescriptor(SurfaceDescriptor& aDesc) {
  if (!mAllocated) {
    return false;
  }
  aDesc = YCbCrImage(mShmem, reinterpret_cast<uint64_t>(this));
  this->AddRef();
  return true;
}

bool
DeprecatedSharedPlanarYCbCrImage::DropToSurfaceDescriptor(SurfaceDescriptor& aDesc) {
  if (!mAllocated) {
    return false;
  }
  aDesc = YCbCrImage(mShmem, 0);
  mShmem = Shmem();
  mAllocated = false;
  return true;
}

DeprecatedSharedPlanarYCbCrImage*
DeprecatedSharedPlanarYCbCrImage::FromSurfaceDescriptor(const SurfaceDescriptor& aDescriptor)
{
  if (aDescriptor.type() != SurfaceDescriptor::TYCbCrImage) {
    return nullptr;
  }
  const YCbCrImage& ycbcr = aDescriptor.get_YCbCrImage();
  if (ycbcr.owner() == 0) {
    return nullptr;
  }
  return reinterpret_cast<DeprecatedSharedPlanarYCbCrImage*>(ycbcr.owner());
}


} 
} 
