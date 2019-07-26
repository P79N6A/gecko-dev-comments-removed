




#include "SharedPlanarYCbCrImage.h"
#include "ShmemYCbCrImage.h"
#include "ISurfaceAllocator.h"
#include "mozilla/layers/LayersSurfaces.h"

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;

SharedPlanarYCbCrImage::~SharedPlanarYCbCrImage() {
  MOZ_COUNT_DTOR(SharedPlanarYCbCrImage);

  if (mAllocated) {
    SurfaceDescriptor desc;
    DropToSurfaceDescriptor(desc);
    mSurfaceAllocator->DestroySharedSurface(&desc);
  }
}


void
SharedPlanarYCbCrImage::SetData(const PlanarYCbCrImage::Data& aData)
{
  
  
  
  if (!mAllocated) {
    Data data = aData;
    if (!Allocate(data)) {
      return;
    }
  }

  
  
  
  mBufferSize = ShmemYCbCrImage::ComputeMinBufferSize(mData.mYSize,
                                                      mData.mCbCrSize);
  mSize = mData.mPicSize;

  ShmemYCbCrImage shmImg(mShmem);
  MOZ_ASSERT(aData.mCbSkip == aData.mCrSkip);
  if (!shmImg.CopyData(aData.mYChannel, aData.mCbChannel, aData.mCrChannel,
                       aData.mYSize, aData.mYStride,
                       aData.mCbCrSize, aData.mCbCrStride,
                       aData.mYSkip, aData.mCbSkip)) {
    NS_WARNING("Failed to copy image data!");
  }
  mData.mYChannel = shmImg.GetYData();
  mData.mCbChannel = shmImg.GetCbData();
  mData.mCrChannel = shmImg.GetCrData();
}



uint8_t*
SharedPlanarYCbCrImage::AllocateAndGetNewBuffer(uint32_t aSize)
{
  NS_ABORT_IF_FALSE(!mAllocated, "This image already has allocated data");
  size_t size = ShmemYCbCrImage::ComputeMinBufferSize(aSize);
  
  mBufferSize = size;

  
  AllocateBuffer(mBufferSize);
  ShmemYCbCrImage shmImg(mShmem);

  return shmImg.GetData();
}


void
SharedPlanarYCbCrImage::SetDataNoCopy(const Data &aData)
{
  mData = aData;
  mSize = aData.mPicSize;
  ShmemYCbCrImage::InitializeBufferInfo(mShmem.get<uint8_t>(),
                                        aData.mYSize,
                                        aData.mCbCrSize);
}

uint8_t* 
SharedPlanarYCbCrImage::AllocateBuffer(uint32_t aSize)
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
SharedPlanarYCbCrImage::Allocate(PlanarYCbCrImage::Data& aData)
{
  NS_ABORT_IF_FALSE(!mAllocated, "This image already has allocated data");

  size_t size = ShmemYCbCrImage::ComputeMinBufferSize(aData.mYSize,
                                                      aData.mCbCrSize);

  if (AllocateBuffer(static_cast<uint32_t>(size)) == nullptr) {
    return false;
  }

  ShmemYCbCrImage::InitializeBufferInfo(mShmem.get<uint8_t>(),
                                        aData.mYSize,
                                        aData.mCbCrSize);
  ShmemYCbCrImage shmImg(mShmem);
  if (!shmImg.IsValid() || mShmem.Size<uint8_t>() < size) {
    mSurfaceAllocator->DeallocShmem(mShmem);
    return false;
  }

  aData.mYChannel = shmImg.GetYData();
  aData.mCbChannel = shmImg.GetCbData();
  aData.mCrChannel = shmImg.GetCrData();

  
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
SharedPlanarYCbCrImage::ToSurfaceDescriptor(SurfaceDescriptor& aDesc) {
  if (!mAllocated) {
    return false;
  }
  aDesc = YCbCrImage(mShmem, 0, reinterpret_cast<uint64_t>(this));
  this->AddRef();
  return true;
}

bool
SharedPlanarYCbCrImage::DropToSurfaceDescriptor(SurfaceDescriptor& aDesc) {
  if (!mAllocated) {
    return false;
  }
  aDesc = YCbCrImage(mShmem, 0, 0);
  mShmem = Shmem();
  mAllocated = false;
  return true;
}

SharedPlanarYCbCrImage*
SharedPlanarYCbCrImage::FromSurfaceDescriptor(const SurfaceDescriptor& aDescriptor)
{
  if (aDescriptor.type() != SurfaceDescriptor::TYCbCrImage) {
    return nullptr;
  }
  const YCbCrImage& ycbcr = aDescriptor.get_YCbCrImage();
  if (ycbcr.owner() == 0) {
    return nullptr;
  }
  return reinterpret_cast<SharedPlanarYCbCrImage*>(ycbcr.owner());
}


} 
} 
