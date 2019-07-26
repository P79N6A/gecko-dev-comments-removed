




#include "ImageClient.h"
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "SharedTextureImage.h"         
#include "gfx2DGlue.h"                  
#include "gfxASurface.h"                
#include "gfxPlatform.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/layers/SharedPlanarYCbCrImage.h"
#include "mozilla/layers/SharedRGBImage.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "mozilla/gfx/2D.h"
#ifdef MOZ_WIDGET_GONK
#include "GrallocImages.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

 TemporaryRef<ImageClient>
ImageClient::CreateImageClient(CompositableType aCompositableHostType,
                               CompositableForwarder* aForwarder,
                               TextureFlags aFlags)
{
  RefPtr<ImageClient> result = nullptr;
  switch (aCompositableHostType) {
  case COMPOSITABLE_IMAGE:
  case BUFFER_IMAGE_SINGLE:
    result = new ImageClientSingle(aForwarder, aFlags, COMPOSITABLE_IMAGE);
    break;
  case BUFFER_IMAGE_BUFFERED:
    result = new ImageClientBuffered(aForwarder, aFlags, COMPOSITABLE_IMAGE);
    break;
  case BUFFER_BRIDGE:
    result = new ImageClientBridge(aForwarder, aFlags);
    break;
  case BUFFER_UNKNOWN:
    result = nullptr;
    break;
  default:
    MOZ_CRASH("unhandled program type");
  }

  NS_ASSERTION(result, "Failed to create ImageClient");

  return result.forget();
}

ImageClientSingle::ImageClientSingle(CompositableForwarder* aFwd,
                                     TextureFlags aFlags,
                                     CompositableType aType)
  : ImageClient(aFwd, aFlags, aType)
{
}

ImageClientBuffered::ImageClientBuffered(CompositableForwarder* aFwd,
                                         TextureFlags aFlags,
                                         CompositableType aType)
  : ImageClientSingle(aFwd, aFlags, aType)
{
}

TextureInfo ImageClientSingle::GetTextureInfo() const
{
  return TextureInfo(COMPOSITABLE_IMAGE);
}

void
ImageClientSingle::FlushAllImages(bool aExceptFront)
{
  if (!aExceptFront && mFrontBuffer) {
    GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
    mFrontBuffer = nullptr;
  }
}

void
ImageClientBuffered::FlushAllImages(bool aExceptFront)
{
  if (!aExceptFront && mFrontBuffer) {
    GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
    mFrontBuffer = nullptr;
  }
  if (mBackBuffer) {
    GetForwarder()->RemoveTextureFromCompositable(this, mBackBuffer);
    mBackBuffer = nullptr;
  }
}

bool
ImageClientSingle::UpdateImage(ImageContainer* aContainer,
                               uint32_t aContentFlags)
{
  bool isSwapped = false;
  return UpdateImageInternal(aContainer, aContentFlags, &isSwapped);
}

bool
ImageClientSingle::UpdateImageInternal(ImageContainer* aContainer,
                                       uint32_t aContentFlags, bool* aIsSwapped)
{
  AutoLockImage autoLock(aContainer);
  *aIsSwapped = false;

  Image *image = autoLock.GetImage();
  if (!image) {
    return false;
  }

  if (mLastPaintedImageSerial == image->GetSerial()) {
    return true;
  }

  if (image->AsSharedImage() && image->AsSharedImage()->GetTextureClient(this)) {
    
    RefPtr<TextureClient> texture = image->AsSharedImage()->GetTextureClient(this);


    if (mFrontBuffer) {
      GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
    }
    mFrontBuffer = texture;
    if (!AddTextureClient(texture)) {
      mFrontBuffer = nullptr;
      return false;
    }
    GetForwarder()->UpdatedTexture(this, texture, nullptr);
    GetForwarder()->UseTexture(this, texture);
  } else if (image->GetFormat() == ImageFormat::PLANAR_YCBCR) {
    PlanarYCbCrImage* ycbcr = static_cast<PlanarYCbCrImage*>(image);
    const PlanarYCbCrData* data = ycbcr->GetData();
    if (!data) {
      return false;
    }

    if (mFrontBuffer && mFrontBuffer->IsImmutable()) {
      GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
      mFrontBuffer = nullptr;
    }

    bool bufferCreated = false;
    if (!mFrontBuffer) {
      mFrontBuffer = CreateBufferTextureClient(gfx::SurfaceFormat::YUV, TEXTURE_FLAGS_DEFAULT);
      gfx::IntSize ySize(data->mYSize.width, data->mYSize.height);
      gfx::IntSize cbCrSize(data->mCbCrSize.width, data->mCbCrSize.height);
      if (!mFrontBuffer->AsTextureClientYCbCr()->AllocateForYCbCr(ySize, cbCrSize, data->mStereoMode)) {
        mFrontBuffer = nullptr;
        return false;
      }
      bufferCreated = true;
    }

    if (!mFrontBuffer->Lock(OPEN_WRITE_ONLY)) {
      return false;
    }
    bool status = mFrontBuffer->AsTextureClientYCbCr()->UpdateYCbCr(*data);
    mFrontBuffer->Unlock();

    if (bufferCreated) {
      if (!AddTextureClient(mFrontBuffer)) {
        mFrontBuffer = nullptr;
        return false;
      }
    }

    if (status) {
      GetForwarder()->UpdatedTexture(this, mFrontBuffer, nullptr);
      GetForwarder()->UseTexture(this, mFrontBuffer);
    } else {
      MOZ_ASSERT(false);
      return false;
    }

  } else if (image->GetFormat() == ImageFormat::SHARED_TEXTURE) {
    SharedTextureImage* sharedImage = static_cast<SharedTextureImage*>(image);
    const SharedTextureImage::Data *data = sharedImage->GetData();
    gfx::IntSize size = gfx::IntSize(image->GetSize().width, image->GetSize().height);

    if (mFrontBuffer) {
      GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
      mFrontBuffer = nullptr;
    }

    RefPtr<SharedTextureClientOGL> buffer = new SharedTextureClientOGL(mTextureFlags);
    buffer->InitWith(data->mHandle, size, data->mShareType, data->mInverted);
    mFrontBuffer = buffer;
    if (!AddTextureClient(mFrontBuffer)) {
      mFrontBuffer = nullptr;
      return false;
    }

    GetForwarder()->UseTexture(this, mFrontBuffer);
  } else {
    RefPtr<gfx::SourceSurface> surface = image->GetAsSourceSurface();
    MOZ_ASSERT(surface);

    gfx::IntSize size = image->GetSize();

    if (mFrontBuffer &&
        (mFrontBuffer->IsImmutable() || mFrontBuffer->GetSize() != size)) {
      GetForwarder()->RemoveTextureFromCompositable(this, mFrontBuffer);
      mFrontBuffer = nullptr;
    }

    bool bufferCreated = false;
    if (!mFrontBuffer) {
      gfxImageFormat format
        = gfxPlatform::GetPlatform()->OptimalFormatForContent(gfx::ContentForFormat(surface->GetFormat()));
      mFrontBuffer = CreateTextureClientForDrawing(gfx::ImageFormatToSurfaceFormat(format),
                                                   mTextureFlags, gfx::BackendType::NONE, size);
      MOZ_ASSERT(mFrontBuffer->CanExposeDrawTarget());
      if (!mFrontBuffer->AllocateForSurface(size)) {
        mFrontBuffer = nullptr;
        return false;
      }

      bufferCreated = true;
    }

    if (!mFrontBuffer->Lock(OPEN_WRITE_ONLY)) {
      return false;
    }

    {
      
      RefPtr<DrawTarget> dt = mFrontBuffer->GetAsDrawTarget();
      MOZ_ASSERT(surface.get());
      dt->CopySurface(surface, IntRect(IntPoint(), surface->GetSize()), IntPoint());
    }

    mFrontBuffer->Unlock();

    if (bufferCreated) {
      if (!AddTextureClient(mFrontBuffer)) {
        mFrontBuffer = nullptr;
        return false;
      }
    }

    GetForwarder()->UpdatedTexture(this, mFrontBuffer, nullptr);
    GetForwarder()->UseTexture(this, mFrontBuffer);
  }

  UpdatePictureRect(image->GetPictureRect());

  mLastPaintedImageSerial = image->GetSerial();
  aContainer->NotifyPaintedImage(image);
  *aIsSwapped = true;
  return true;
}

bool
ImageClientBuffered::UpdateImage(ImageContainer* aContainer,
                                 uint32_t aContentFlags)
{
  RefPtr<TextureClient> temp = mFrontBuffer;
  mFrontBuffer = mBackBuffer;
  mBackBuffer = temp;

  bool isSwapped = false;
  bool ret = ImageClientSingle::UpdateImageInternal(aContainer, aContentFlags, &isSwapped);

  if (!isSwapped) {
    
    RefPtr<TextureClient> temp = mFrontBuffer;
    mFrontBuffer = mBackBuffer;
    mBackBuffer = temp;
  }
  return ret;
}

bool
ImageClientSingle::AddTextureClient(TextureClient* aTexture)
{
  MOZ_ASSERT((mTextureFlags & aTexture->GetFlags()) == mTextureFlags);
  return CompositableClient::AddTextureClient(aTexture);
}

void
ImageClientSingle::OnDetach()
{
  mFrontBuffer = nullptr;
}

void
ImageClientBuffered::OnDetach()
{
  mFrontBuffer = nullptr;
  mBackBuffer = nullptr;
}

ImageClient::ImageClient(CompositableForwarder* aFwd, TextureFlags aFlags,
                         CompositableType aType)
: CompositableClient(aFwd, aFlags)
, mType(aType)
, mLastPaintedImageSerial(0)
{}

void
ImageClient::UpdatePictureRect(nsIntRect aRect)
{
  if (mPictureRect == aRect) {
    return;
  }
  mPictureRect = aRect;
  MOZ_ASSERT(mForwarder);
  GetForwarder()->UpdatePictureRect(this, aRect);
}

ImageClientBridge::ImageClientBridge(CompositableForwarder* aFwd,
                                     TextureFlags aFlags)
: ImageClient(aFwd, aFlags, BUFFER_BRIDGE)
, mAsyncContainerID(0)
, mLayer(nullptr)
{
}

bool
ImageClientBridge::UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags)
{
  if (!GetForwarder() || !mLayer) {
    return false;
  }
  if (mAsyncContainerID == aContainer->GetAsyncContainerID()) {
    return true;
  }
  mAsyncContainerID = aContainer->GetAsyncContainerID();
  static_cast<ShadowLayerForwarder*>(GetForwarder())->AttachAsyncCompositable(mAsyncContainerID, mLayer);
  AutoLockImage autoLock(aContainer);
  aContainer->NotifyPaintedImage(autoLock.GetImage());
  Updated();
  return true;
}

already_AddRefed<Image>
ImageClientSingle::CreateImage(ImageFormat aFormat)
{
  nsRefPtr<Image> img;
  switch (aFormat) {
    case ImageFormat::PLANAR_YCBCR:
      img = new SharedPlanarYCbCrImage(this);
      return img.forget();
    case ImageFormat::SHARED_RGB:
      img = new SharedRGBImage(this);
      return img.forget();
#ifdef MOZ_WIDGET_GONK
    case ImageFormat::GRALLOC_PLANAR_YCBCR:
      img = new GrallocImage();
      return img.forget();
#endif
    default:
      return nullptr;
  }
}

}
}
