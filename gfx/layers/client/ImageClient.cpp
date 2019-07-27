




#include "ImageClient.h"
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "GLImages.h"                   
#include "gfx2DGlue.h"                  
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

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

 TemporaryRef<ImageClient>
ImageClient::CreateImageClient(CompositableType aCompositableHostType,
                               CompositableForwarder* aForwarder,
                               TextureFlags aFlags)
{
  RefPtr<ImageClient> result = nullptr;
  switch (aCompositableHostType) {
  case CompositableType::IMAGE:
  case CompositableType::BUFFER_IMAGE_SINGLE:
    result = new ImageClientSingle(aForwarder, aFlags, CompositableType::IMAGE);
    break;
  case CompositableType::BUFFER_IMAGE_BUFFERED:
    result = new ImageClientBuffered(aForwarder, aFlags, CompositableType::IMAGE);
    break;
  case CompositableType::BUFFER_BRIDGE:
    result = new ImageClientBridge(aForwarder, aFlags);
    break;
  case CompositableType::BUFFER_UNKNOWN:
    result = nullptr;
    break;
#ifdef MOZ_WIDGET_GONK
  case CompositableType::IMAGE_OVERLAY:
    result = new ImageClientOverlay(aForwarder, aFlags);
    break;
#endif
  default:
    MOZ_CRASH("unhandled program type");
  }

  NS_ASSERTION(result, "Failed to create ImageClient");

  return result.forget();
}

void
ImageClient::RemoveTexture(TextureClient* aTexture)
{
  RemoveTextureWithTracker(aTexture, nullptr);
}

void
ImageClient::RemoveTextureWithTracker(TextureClient* aTexture,
                                      AsyncTransactionTracker* aAsyncTransactionTracker)
{
#ifdef MOZ_WIDGET_GONK
  if (aAsyncTransactionTracker ||
      GetForwarder()->IsImageBridgeChild()) {
    RefPtr<AsyncTransactionTracker> request = aAsyncTransactionTracker;
    if (!request) {
      
      request = new RemoveTextureFromCompositableTracker();
    }
    
    
    request->SetTextureClient(aTexture);
    GetForwarder()->RemoveTextureFromCompositableAsync(request, this, aTexture);
    return;
  }
#endif

  GetForwarder()->RemoveTextureFromCompositable(this, aTexture);
  if (aAsyncTransactionTracker) {
    
    
    aAsyncTransactionTracker->NotifyComplete();
  }
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
  return TextureInfo(CompositableType::IMAGE);
}

TemporaryRef<AsyncTransactionTracker>
ImageClientSingle::PrepareFlushAllImages()
{
  RefPtr<AsyncTransactionTracker> status = new RemoveTextureFromCompositableTracker();
  return status;
}

void
ImageClientSingle::FlushAllImages(bool aExceptFront,
                                  AsyncTransactionTracker* aAsyncTransactionTracker)
{
  if (!aExceptFront && mFrontBuffer) {
    RemoveTextureWithTracker(mFrontBuffer, aAsyncTransactionTracker);
    mFrontBuffer = nullptr;
  } else if(aAsyncTransactionTracker) {
    
    aAsyncTransactionTracker->NotifyComplete();
  }
}

void
ImageClientBuffered::FlushAllImages(bool aExceptFront,
                                    AsyncTransactionTracker* aAsyncTransactionTracker)
{
  if (!aExceptFront && mFrontBuffer) {
    RemoveTexture(mFrontBuffer);
    mFrontBuffer = nullptr;
  }
  if (mBackBuffer) {
    RemoveTextureWithTracker(mBackBuffer, aAsyncTransactionTracker);
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

  AutoRemoveTexture autoRemoveTexture(this);

  if (image->AsSharedImage() && image->AsSharedImage()->GetTextureClient(this)) {
    
    RefPtr<TextureClient> texture = image->AsSharedImage()->GetTextureClient(this);

    autoRemoveTexture.mTexture = mFrontBuffer;
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
      autoRemoveTexture.mTexture = mFrontBuffer;
      mFrontBuffer = nullptr;
    }

    bool bufferCreated = false;
    if (!mFrontBuffer) {
      gfx::IntSize ySize(data->mYSize.width, data->mYSize.height);
      gfx::IntSize cbCrSize(data->mCbCrSize.width, data->mCbCrSize.height);
      mFrontBuffer = TextureClient::CreateForYCbCr(GetForwarder(),
                                                   ySize, cbCrSize, data->mStereoMode,
                                                   TextureFlags::DEFAULT|mTextureFlags);
      if (!mFrontBuffer) {
        return false;
      }
      bufferCreated = true;
    }

    if (!mFrontBuffer->Lock(OpenMode::OPEN_WRITE_ONLY)) {
      mFrontBuffer = nullptr;
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

  } else if (image->GetFormat() == ImageFormat::SURFACE_TEXTURE ||
             image->GetFormat() == ImageFormat::EGLIMAGE)
  {
    gfx::IntSize size = gfx::IntSize(image->GetSize().width, image->GetSize().height);

    if (mFrontBuffer) {
      autoRemoveTexture.mTexture = mFrontBuffer;
      mFrontBuffer = nullptr;
    }

    RefPtr<TextureClient> buffer;

    if (image->GetFormat() == ImageFormat::EGLIMAGE) {
      EGLImageImage* typedImage = static_cast<EGLImageImage*>(image);
      const EGLImageImage::Data* data = typedImage->GetData();

      buffer = new EGLImageTextureClient(mTextureFlags,
                                         data->mImage,
                                         size,
                                         data->mInverted);
#ifdef MOZ_WIDGET_ANDROID
    } else if (image->GetFormat() == ImageFormat::SURFACE_TEXTURE) {
      SurfaceTextureImage* typedImage = static_cast<SurfaceTextureImage*>(image);
      const SurfaceTextureImage::Data* data = typedImage->GetData();

      buffer = new SurfaceTextureClient(mTextureFlags,
                                        data->mSurfTex,
                                        size,
                                        data->mInverted);
#endif
    } else {
      MOZ_ASSERT(false, "Bad ImageFormat.");
    }

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
      autoRemoveTexture.mTexture = mFrontBuffer;
      mFrontBuffer = nullptr;
    }

    bool bufferCreated = false;
    if (!mFrontBuffer) {
      mFrontBuffer = CreateTextureClientForDrawing(surface->GetFormat(), size,
                                                   gfx::BackendType::NONE, mTextureFlags);
      if (!mFrontBuffer) {
        return false;
      }
      MOZ_ASSERT(mFrontBuffer->CanExposeDrawTarget());
      bufferCreated = true;
    }

    if (!mFrontBuffer->Lock(OpenMode::OPEN_WRITE_ONLY)) {
      mFrontBuffer = nullptr;
      return false;
    }

    {
      
      DrawTarget* dt = mFrontBuffer->BorrowDrawTarget();
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
: ImageClient(aFwd, aFlags, CompositableType::BUFFER_BRIDGE)
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

#ifdef MOZ_WIDGET_GONK
ImageClientOverlay::ImageClientOverlay(CompositableForwarder* aFwd,
                                       TextureFlags aFlags)
  : ImageClient(aFwd, aFlags, CompositableType::IMAGE_OVERLAY)
{
}

bool
ImageClientOverlay::UpdateImage(ImageContainer* aContainer, uint32_t aContentFlags)
{
  AutoLockImage autoLock(aContainer);

  Image *image = autoLock.GetImage();
  if (!image) {
    return false;
  }

  if (mLastPaintedImageSerial == image->GetSerial()) {
    return true;
  }

  AutoRemoveTexture autoRemoveTexture(this);
  if (image->GetFormat() == ImageFormat::OVERLAY_IMAGE) {
    OverlayImage* overlayImage = static_cast<OverlayImage*>(image);
    uint32_t overlayId = overlayImage->GetOverlayId();
    gfx::IntSize size = overlayImage->GetSize();

    OverlaySource source;
    source.handle() = OverlayHandle(overlayId);
    source.size() = size;
    GetForwarder()->UseOverlaySource(this, source);
  }
  UpdatePictureRect(image->GetPictureRect());
  return true;
}

already_AddRefed<Image>
ImageClientOverlay::CreateImage(ImageFormat aFormat)
{
  nsRefPtr<Image> img;
  switch (aFormat) {
    case ImageFormat::OVERLAY_IMAGE:
      img = new OverlayImage();
      return img.forget();
    default:
      return nullptr;
  }
}

#endif
}
}
