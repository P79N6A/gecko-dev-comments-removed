




#include "SharedPlanarYCbCrImage.h"
#include <stddef.h>                     
#include <stdio.h>                      
#include "gfx2DGlue.h"                  
#include "ISurfaceAllocator.h"          
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/Types.h"          
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/ImageClient.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "mozilla/layers/ImageBridgeChild.h"  
#include "mozilla/mozalloc.h"           
#include "nsISupportsImpl.h"            
#include "mozilla/ipc/Shmem.h"

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;

SharedPlanarYCbCrImage::SharedPlanarYCbCrImage(ImageClient* aCompositable)
: PlanarYCbCrImage(nullptr)
, mCompositable(aCompositable)
{
  MOZ_COUNT_CTOR(SharedPlanarYCbCrImage);
}

SharedPlanarYCbCrImage::~SharedPlanarYCbCrImage() {
  MOZ_COUNT_DTOR(SharedPlanarYCbCrImage);

  if (mCompositable->GetAsyncID() != 0 &&
      !InImageBridgeChildThread()) {
    ImageBridgeChild::DispatchReleaseTextureClient(mTextureClient.forget().take());
    ImageBridgeChild::DispatchReleaseImageClient(mCompositable.forget().take());
  }
}

size_t
SharedPlanarYCbCrImage::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  size_t size = PlanarYCbCrImage::SizeOfExcludingThis(aMallocSizeOf);
  return size;
}

TextureClient*
SharedPlanarYCbCrImage::GetTextureClient(CompositableClient* aClient)
{
  return mTextureClient.get();
}

uint8_t*
SharedPlanarYCbCrImage::GetBuffer()
{
  return mTextureClient->GetBuffer();
}

TemporaryRef<gfx::SourceSurface>
SharedPlanarYCbCrImage::GetAsSourceSurface()
{
  if (!mTextureClient) {
    NS_WARNING("Can't get as surface");
    return nullptr;
  }
  return PlanarYCbCrImage::GetAsSourceSurface();
}

void
SharedPlanarYCbCrImage::SetData(const PlanarYCbCrData& aData)
{
  
  
  
  PlanarYCbCrData data = aData;
  if (!mTextureClient && !Allocate(data)) {
    return;
  }

  MOZ_ASSERT(mTextureClient->AsTextureClientYCbCr());
  if (!mTextureClient->Lock(OpenMode::OPEN_WRITE_ONLY)) {
    MOZ_ASSERT(false, "Failed to lock the texture.");
    return;
  }
  TextureClientAutoUnlock unlock(mTextureClient);
  if (!mTextureClient->AsTextureClientYCbCr()->UpdateYCbCr(aData)) {
    MOZ_ASSERT(false, "Failed to copy YCbCr data into the TextureClient");
    return;
  }
  mTextureClient->MarkImmutable();
}



uint8_t*
SharedPlanarYCbCrImage::AllocateAndGetNewBuffer(uint32_t aSize)
{
  MOZ_ASSERT(!mTextureClient, "This image already has allocated data");
  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(aSize);
  if (!size) {
    return nullptr;
  }

  mTextureClient = TextureClient::CreateWithBufferSize(mCompositable->GetForwarder(),
                                                       gfx::SurfaceFormat::YUV, size,
                                                       mCompositable->GetTextureFlags());

  
  if (!mTextureClient) {
    return nullptr;
  }

  
  mBufferSize = size;

  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer(), mTextureClient->GetBufferSize());
  return serializer.GetData();
}

void
SharedPlanarYCbCrImage::SetDataNoCopy(const Data &aData)
{
  MOZ_ASSERT(mTextureClient, "This Image should have already allocated data");
  mData = aData;
  mSize = aData.mPicSize;
  





  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer(), mTextureClient->GetBufferSize());
  uint8_t *base = serializer.GetData();
  uint32_t yOffset = aData.mYChannel - base;
  uint32_t cbOffset = aData.mCbChannel - base;
  uint32_t crOffset = aData.mCrChannel - base;
  serializer.InitializeBufferInfo(yOffset,
                                  cbOffset,
                                  crOffset,
                                  aData.mYStride,
                                  aData.mCbCrStride,
                                  aData.mYSize,
                                  aData.mCbCrSize,
                                  aData.mStereoMode);
}

uint8_t*
SharedPlanarYCbCrImage::AllocateBuffer(uint32_t aSize)
{
  MOZ_ASSERT(!mTextureClient,
             "This image already has allocated data");
  mTextureClient = TextureClient::CreateWithBufferSize(mCompositable->GetForwarder(),
                                                       gfx::SurfaceFormat::YUV, aSize,
                                                       mCompositable->GetTextureFlags());
  if (!mTextureClient) {
    return nullptr;
  }
  return mTextureClient->GetBuffer();
}

bool
SharedPlanarYCbCrImage::IsValid() {
  return !!mTextureClient;
}

bool
SharedPlanarYCbCrImage::Allocate(PlanarYCbCrData& aData)
{
  MOZ_ASSERT(!mTextureClient,
             "This image already has allocated data");

  mTextureClient = TextureClient::CreateForYCbCr(mCompositable->GetForwarder(),
                                                 aData.mYSize, aData.mCbCrSize,
                                                 aData.mStereoMode,
                                                 mCompositable->GetTextureFlags());
  if (!mTextureClient) {
    NS_WARNING("SharedPlanarYCbCrImage::Allocate failed.");
    return false;
  }

  YCbCrImageDataSerializer serializer(mTextureClient->GetBuffer(), mTextureClient->GetBufferSize());
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

  
  
  
  mBufferSize = YCbCrImageDataSerializer::ComputeMinBufferSize(mData.mYSize,
                                                               mData.mCbCrSize);
  mSize = mData.mPicSize;

  return mBufferSize > 0;
}

} 
} 
