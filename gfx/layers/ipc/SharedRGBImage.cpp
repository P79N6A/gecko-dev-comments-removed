



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


#define MAX_FRAME_SIZE (16 * 1024 * 1024)

namespace mozilla {
namespace layers {

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
    NS_WARNING("No ImageContainer to allocate SharedRGBImage");
    return nullptr;
  }

  nsRefPtr<Image> image = aImageContainer->CreateImage(ImageFormat::SHARED_RGB);

  if (!image) {
    NS_WARNING("Failed to create SharedRGBImage");
    return nullptr;
  }

  nsRefPtr<SharedRGBImage> rgbImage = static_cast<SharedRGBImage*>(image.get());
  if (!rgbImage->Allocate(aSize, gfx::ImageFormatToSurfaceFormat(aImageFormat))) {
    NS_WARNING("Failed to allocate a shared image");
    return nullptr;
  }
  return image.forget();
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
    ImageBridgeChild::DispatchReleaseTextureClient(mTextureClient.forget().take());
    ImageBridgeChild::DispatchReleaseImageClient(mCompositable.forget().take());
  }
}

bool
SharedRGBImage::Allocate(gfx::IntSize aSize, gfx::SurfaceFormat aFormat)
{
  mSize = aSize;
  mTextureClient = mCompositable->CreateBufferTextureClient(aFormat, aSize,
                                                            gfx::BackendType::NONE,
                                                            TextureFlags::DEFAULT);
  return !!mTextureClient;
}

uint8_t*
SharedRGBImage::GetBuffer()
{
  if (!mTextureClient) {
    return nullptr;
  }

  ImageDataSerializer serializer(mTextureClient->GetBuffer(), mTextureClient->GetBufferSize());
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
SharedRGBImage::GetTextureClient(CompositableClient* aClient)
{
  return mTextureClient.get();
}

TemporaryRef<gfx::SourceSurface>
SharedRGBImage::GetAsSourceSurface()
{
  return nullptr;
}

} 
} 
