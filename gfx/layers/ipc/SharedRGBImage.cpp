



#include "SharedRGBImage.h"
#include "ImageTypes.h"                 
#include "Shmem.h"                      
#include "gfx2DGlue.h"                  
#include "gfxPlatform.h"                
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/ImageClient.h"  
#include "mozilla/layers/ImageDataSerializer.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/ImageBridgeChild.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "nsTraceRefcnt.h"              


#define MAX_FRAME_SIZE (16 * 1024 * 1024)

namespace mozilla {
namespace layers {

DeprecatedSharedRGBImage::DeprecatedSharedRGBImage(ISurfaceAllocator *aAllocator) :
  Image(nullptr, ImageFormat::SHARED_RGB),
  mSize(0, 0),
  mSurfaceAllocator(aAllocator),
  mAllocated(false),
  mShmem(new ipc::Shmem())
{
  MOZ_COUNT_CTOR(DeprecatedSharedRGBImage);
}

DeprecatedSharedRGBImage::~DeprecatedSharedRGBImage()
{
  MOZ_COUNT_DTOR(DeprecatedSharedRGBImage);

  if (mAllocated) {
    SurfaceDescriptor desc;
    DropToSurfaceDescriptor(desc);
    mSurfaceAllocator->DestroySharedSurface(&desc);
  }
  delete mShmem;
}

already_AddRefed<Image>
CreateSharedRGBImage(ImageContainer *aImageContainer,
                     nsIntSize aSize,
                     gfxImageFormat aImageFormat)
{
  NS_ASSERTION(aImageFormat == gfxImageFormat::ARGB32 ||
               aImageFormat == gfxImageFormat::RGB24 ||
               aImageFormat == gfxImageFormat::RGB16_565,
               "RGB formats supported only");

  if (!aImageContainer) {
    NS_WARNING("No ImageContainer to allocate DeprecatedSharedRGBImage");
    return nullptr;
  }

  nsRefPtr<Image> image = aImageContainer->CreateImage(ImageFormat::SHARED_RGB);

  if (!image) {
    NS_WARNING("Failed to create DeprecatedSharedRGBImage");
    return nullptr;
  }

  if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
    nsRefPtr<DeprecatedSharedRGBImage> rgbImageDep = static_cast<DeprecatedSharedRGBImage*>(image.get());
    rgbImageDep->mSize = aSize.ToIntSize();
    rgbImageDep->mImageFormat = aImageFormat;

    if (!rgbImageDep->AllocateBuffer(aSize, aImageFormat)) {
      NS_WARNING("Failed to allocate shared memory for DeprecatedSharedRGBImage");
      return nullptr;
    }
    return rgbImageDep.forget();
  }
  nsRefPtr<SharedRGBImage> rgbImage = static_cast<SharedRGBImage*>(image.get());
  if (!rgbImage->Allocate(gfx::ToIntSize(aSize),
                          gfx::ImageFormatToSurfaceFormat(aImageFormat))) {
    NS_WARNING("Failed to allocate a shared image");
    return nullptr;
  }
  return image.forget();
}

uint8_t *
DeprecatedSharedRGBImage::GetBuffer()
{
  return mShmem->get<uint8_t>();
}

size_t
DeprecatedSharedRGBImage::GetBufferSize()
{
  return mSize.width * mSize.height * gfxASurface::BytesPerPixel(mImageFormat);
}

gfx::IntSize
DeprecatedSharedRGBImage::GetSize()
{
  return mSize;
}

bool
DeprecatedSharedRGBImage::AllocateBuffer(nsIntSize aSize, gfxImageFormat aImageFormat)
{
  if (mAllocated) {
    NS_WARNING("Already allocated shmem");
    return false;
  }

  size_t size = GetBufferSize();

  if (size == 0 || size > MAX_FRAME_SIZE) {
    NS_WARNING("Invalid frame size");
  }
  if (mSurfaceAllocator->AllocUnsafeShmem(size, OptimalShmemType(), mShmem)) {
    mAllocated = true;
  }

  return mAllocated;
}

already_AddRefed<gfxASurface>
DeprecatedSharedRGBImage::DeprecatedGetAsSurface()
{
  return nullptr;
}

TemporaryRef<gfx::SourceSurface>
DeprecatedSharedRGBImage::GetAsSourceSurface()
{
  return nullptr;
}

bool
DeprecatedSharedRGBImage::ToSurfaceDescriptor(SurfaceDescriptor& aResult)
{
  if (!mAllocated) {
    return false;
  }
  this->AddRef();
  aResult = RGBImage(*mShmem,
                     nsIntRect(0, 0, mSize.width, mSize.height),
                     mImageFormat,
                     reinterpret_cast<uint64_t>(this));
  return true;
}

bool
DeprecatedSharedRGBImage::DropToSurfaceDescriptor(SurfaceDescriptor& aResult)
{
  if (!mAllocated) {
    return false;
  }
  aResult = RGBImage(*mShmem,
                     nsIntRect(0, 0, mSize.width, mSize.height),
                     mImageFormat,
                     0);
  *mShmem = ipc::Shmem();
  mAllocated = false;
  return true;
}

DeprecatedSharedRGBImage*
DeprecatedSharedRGBImage::FromSurfaceDescriptor(const SurfaceDescriptor& aDescriptor)
{
  if (aDescriptor.type() != SurfaceDescriptor::TRGBImage) {
    return nullptr;
  }
  const RGBImage& rgb = aDescriptor.get_RGBImage();
  if (rgb.owner() == 0) {
    return nullptr;
  }
  return reinterpret_cast<DeprecatedSharedRGBImage*>(rgb.owner());
}

SharedRGBImage::SharedRGBImage(ImageClient* aCompositable)
: Image(nullptr, ImageFormat::SHARED_RGB)
, mCompositable(aCompositable)
{
  MOZ_COUNT_CTOR(SharedRGBImage);
}

SharedRGBImage::~SharedRGBImage()
{
  MOZ_COUNT_DTOR(SharedRGBImage);

  if (mCompositable->GetAsyncID() != 0 &&
      !InImageBridgeChildThread()) {
    ImageBridgeChild::DispatchReleaseTextureClient(mTextureClient.forget().drop());
    ImageBridgeChild::DispatchReleaseImageClient(mCompositable.forget().drop());
  }
}

bool
SharedRGBImage::Allocate(gfx::IntSize aSize, gfx::SurfaceFormat aFormat)
{
  mSize = aSize;
  mTextureClient = mCompositable->CreateBufferTextureClient(aFormat);
  return mTextureClient->AllocateForSurface(aSize);
}

uint8_t*
SharedRGBImage::GetBuffer()
{
  if (!mTextureClient) {
    return nullptr;
  }

  ImageDataSerializer serializer(mTextureClient->GetBuffer());
  return serializer.GetData();
}

gfx::IntSize
SharedRGBImage::GetSize()
{
  return mSize;
}

size_t
SharedRGBImage::GetBufferSize()
{
  return mTextureClient ? mTextureClient->GetBufferSize()
                        : 0;
}

TextureClient*
SharedRGBImage::GetTextureClient()
{
  return mTextureClient.get();
}

already_AddRefed<gfxASurface>
SharedRGBImage::DeprecatedGetAsSurface()
{
  return nullptr;
}

TemporaryRef<gfx::SourceSurface>
SharedRGBImage::GetAsSourceSurface()
{
  return nullptr;
}


} 
} 
