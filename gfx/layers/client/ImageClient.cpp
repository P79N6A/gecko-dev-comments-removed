




#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/ImageClient.h"
#include "BasicLayers.h"
#include "mozilla/layers/ShadowLayers.h"
#include "SharedTextureImage.h"
#include "ImageContainer.h" 
#include "mozilla/layers/SharedRGBImage.h"
#include "mozilla/layers/SharedPlanarYCbCrImage.h"
#include "gfxPlatform.h"

#ifdef MOZ_WIDGET_GONK
#include "GrallocImages.h"
#endif

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
    if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
      result = new DeprecatedImageClientSingle(aForwarder, aFlags, BUFFER_IMAGE_SINGLE);
    } else {
      result = new ImageClientSingle(aForwarder, aFlags, COMPOSITABLE_IMAGE);
    }
    break;
  case BUFFER_IMAGE_BUFFERED:
    if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
      result = new DeprecatedImageClientSingle(aForwarder, aFlags, BUFFER_IMAGE_BUFFERED);
    } else {
      
      
      result = new ImageClientSingle(aForwarder, aFlags, COMPOSITABLE_IMAGE);
    }
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
  : ImageClient(aFwd, aType)
  , mTextureFlags(aFlags)
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

bool
ImageClientSingle::UpdateImage(ImageContainer* aContainer,
                               uint32_t aContentFlags)
{
  AutoLockImage autoLock(aContainer);

  Image *image = autoLock.GetImage();
  if (!image) {
    return false;
  }

  if (mLastPaintedImageSerial == image->GetSerial()) {
    return true;
  }

  if (image->AsSharedImage() && image->AsSharedImage()->GetTextureClient()) {
    
    RefPtr<TextureClient> tex = image->AsSharedImage()->GetTextureClient();

    if (mFrontBuffer) {
      RemoveTextureClient(mFrontBuffer);
    }
    mFrontBuffer = tex;
    AddTextureClient(tex);
    GetForwarder()->UpdatedTexture(this, tex, nullptr);
    GetForwarder()->UseTexture(this, tex);
  } else if (image->GetFormat() == PLANAR_YCBCR) {
    PlanarYCbCrImage* ycbcr = static_cast<PlanarYCbCrImage*>(image);
    const PlanarYCbCrImage::Data* data = ycbcr->GetData();
    if (!data) {
      return false;
    }

    if (mFrontBuffer && mFrontBuffer->IsImmutable()) {
      RemoveTextureClient(mFrontBuffer);
      mFrontBuffer = nullptr;
    }

    if (!mFrontBuffer) {
      mFrontBuffer = CreateBufferTextureClient(gfx::FORMAT_YUV);
      gfx::IntSize ySize(data->mYSize.width, data->mYSize.height);
      gfx::IntSize cbCrSize(data->mCbCrSize.width, data->mCbCrSize.height);
      if (!mFrontBuffer->AsTextureClientYCbCr()->AllocateForYCbCr(ySize, cbCrSize)) {
        mFrontBuffer = nullptr;
        return false;
      }
      AddTextureClient(mFrontBuffer);
    }

    if (!mFrontBuffer->Lock(OPEN_READ_WRITE)) {
      return false;
    }
    bool status = mFrontBuffer->AsTextureClientYCbCr()->UpdateYCbCr(*data);
    mFrontBuffer->Unlock();

    if (status) {
      GetForwarder()->UpdatedTexture(this, mFrontBuffer, nullptr);
      GetForwarder()->UseTexture(this, mFrontBuffer);
    } else {
      MOZ_ASSERT(false);
      return false;
    }

  } else {
    nsRefPtr<gfxASurface> surface = image->GetAsSurface();
    MOZ_ASSERT(surface);

    gfx::IntSize size = gfx::IntSize(image->GetSize().width, image->GetSize().height);

    if (mFrontBuffer &&
        (mFrontBuffer->IsImmutable() || mFrontBuffer->GetSize() != size)) {
      RemoveTextureClient(mFrontBuffer);
      mFrontBuffer = nullptr;
    }

    if (!mFrontBuffer) {
      gfxASurface::gfxImageFormat format
        = gfxPlatform::GetPlatform()->OptimalFormatForContent(surface->GetContentType());
      mFrontBuffer = CreateBufferTextureClient(gfx::ImageFormatToSurfaceFormat(format));
      MOZ_ASSERT(mFrontBuffer->AsTextureClientSurface());
      mFrontBuffer->AsTextureClientSurface()->AllocateForSurface(size);

      AddTextureClient(mFrontBuffer);
    }

    if (!mFrontBuffer->Lock(OPEN_READ_WRITE)) {
      return false;
    }
    bool status = mFrontBuffer->AsTextureClientSurface()->UpdateSurface(surface);
    mFrontBuffer->Unlock();
    if (status) {
      GetForwarder()->UpdatedTexture(this, mFrontBuffer, nullptr);
      GetForwarder()->UseTexture(this, mFrontBuffer);
    } else {
      return false;
    }
  }

  UpdatePictureRect(image->GetPictureRect());

  mLastPaintedImageSerial = image->GetSerial();
  aContainer->NotifyPaintedImage(image);
  return true;
}

bool
ImageClientBuffered::UpdateImage(ImageContainer* aContainer,
                                 uint32_t aContentFlags)
{
  RefPtr<TextureClient> temp = mFrontBuffer;
  mFrontBuffer = mBackBuffer;
  mBackBuffer = temp;
  return ImageClientSingle::UpdateImage(aContainer, aContentFlags);
}

void
ImageClientSingle::AddTextureClient(TextureClient* aTexture)
{
  aTexture->AddFlags(mTextureFlags);
  CompositableClient::AddTextureClient(aTexture);
}

void
ImageClientSingle::Detach()
{
  mFrontBuffer = nullptr;
}

void
ImageClientBuffered::Detach()
{
  mFrontBuffer = nullptr;
  mBackBuffer = nullptr;
}

ImageClient::ImageClient(CompositableForwarder* aFwd, CompositableType aType)
: CompositableClient(aFwd)
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

DeprecatedImageClientSingle::DeprecatedImageClientSingle(CompositableForwarder* aFwd,
                                                         TextureFlags aFlags,
                                                         CompositableType aType)
  : ImageClient(aFwd, aType)
  , mTextureInfo(aType)
{
  mTextureInfo.mTextureFlags = aFlags;
}

bool
DeprecatedImageClientSingle::EnsureDeprecatedTextureClient(DeprecatedTextureClientType aType)
{
  
  
  if (mDeprecatedTextureClient && mDeprecatedTextureClient->SupportsType(aType)) {
    return true;
  }
  mDeprecatedTextureClient = CreateDeprecatedTextureClient(aType);
  return !!mDeprecatedTextureClient;
}

bool
DeprecatedImageClientSingle::UpdateImage(ImageContainer* aContainer,
                               uint32_t aContentFlags)
{
  AutoLockImage autoLock(aContainer);

  Image *image = autoLock.GetImage();
  if (!image) {
    return false;
  }

  if (mLastPaintedImageSerial == image->GetSerial()) {
    return true;
  }

  if (image->GetFormat() == PLANAR_YCBCR &&
      EnsureDeprecatedTextureClient(TEXTURE_YCBCR)) {
    PlanarYCbCrImage* ycbcr = static_cast<PlanarYCbCrImage*>(image);

    if (ycbcr->AsDeprecatedSharedPlanarYCbCrImage()) {
      AutoLockDeprecatedTextureClient lock(mDeprecatedTextureClient);

      SurfaceDescriptor sd;
      if (!ycbcr->AsDeprecatedSharedPlanarYCbCrImage()->ToSurfaceDescriptor(sd)) {
        return false;
      }

      if (IsSurfaceDescriptorValid(*lock.GetSurfaceDescriptor())) {
        GetForwarder()->DestroySharedSurface(lock.GetSurfaceDescriptor());
      }

      *lock.GetSurfaceDescriptor() = sd;
    } else {
      AutoLockYCbCrClient clientLock(mDeprecatedTextureClient);

      if (!clientLock.Update(ycbcr)) {
        NS_WARNING("failed to update DeprecatedTextureClient (YCbCr)");
        return false;
      }
    }
  } else if (image->GetFormat() == SHARED_TEXTURE &&
             EnsureDeprecatedTextureClient(TEXTURE_SHARED_GL_EXTERNAL)) {
    SharedTextureImage* sharedImage = static_cast<SharedTextureImage*>(image);
    const SharedTextureImage::Data *data = sharedImage->GetData();

    SharedTextureDescriptor texture(data->mShareType,
                                    data->mHandle,
                                    data->mSize,
                                    data->mInverted);
    mDeprecatedTextureClient->SetDescriptor(SurfaceDescriptor(texture));
  } else if (image->GetFormat() == SHARED_RGB &&
             EnsureDeprecatedTextureClient(TEXTURE_SHMEM)) {
    nsIntRect rect(0, 0,
                   image->GetSize().width,
                   image->GetSize().height);
    UpdatePictureRect(rect);

    AutoLockDeprecatedTextureClient lock(mDeprecatedTextureClient);

    SurfaceDescriptor desc;
    if (!static_cast<DeprecatedSharedRGBImage*>(image)->ToSurfaceDescriptor(desc)) {
      return false;
    }
    mDeprecatedTextureClient->SetDescriptor(desc);
#ifdef MOZ_WIDGET_GONK
  } else if (image->GetFormat() == GRALLOC_PLANAR_YCBCR) {
    EnsureDeprecatedTextureClient(TEXTURE_SHARED_GL_EXTERNAL);

    nsIntRect rect(0, 0,
                   image->GetSize().width,
                   image->GetSize().height);
    UpdatePictureRect(rect);

    AutoLockDeprecatedTextureClient lock(mDeprecatedTextureClient);

    SurfaceDescriptor desc = static_cast<GrallocImage*>(image)->GetSurfaceDescriptor();
    if (!IsSurfaceDescriptorValid(desc)) {
      return false;
    }
    mDeprecatedTextureClient->SetDescriptor(desc);
#endif
  } else {
    nsRefPtr<gfxASurface> surface = image->GetAsSurface();
    MOZ_ASSERT(surface);

    EnsureDeprecatedTextureClient(TEXTURE_SHMEM);
    MOZ_ASSERT(mDeprecatedTextureClient, "Failed to create texture client");

    AutoLockShmemClient clientLock(mDeprecatedTextureClient);
    if (!clientLock.Update(image, aContentFlags, surface)) {
      NS_WARNING("failed to update DeprecatedTextureClient");
      return false;
    }
  }

  Updated();

  if (image->GetFormat() == PLANAR_YCBCR) {
    PlanarYCbCrImage* ycbcr = static_cast<PlanarYCbCrImage*>(image);
    UpdatePictureRect(ycbcr->GetData()->GetPictureRect());
  }

  mLastPaintedImageSerial = image->GetSerial();
  aContainer->NotifyPaintedImage(image);
  return true;
}

void
DeprecatedImageClientSingle::Updated()
{
  mForwarder->UpdateTexture(this, 1, mDeprecatedTextureClient->GetDescriptor());
}

ImageClientBridge::ImageClientBridge(CompositableForwarder* aFwd,
                                     TextureFlags aFlags)
: ImageClient(aFwd, BUFFER_BRIDGE)
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
ImageClient::CreateImage(const uint32_t *aFormats,
                         uint32_t aNumFormats)
{
  nsRefPtr<Image> img;
  for (uint32_t i = 0; i < aNumFormats; i++) {
    switch (aFormats[i]) {
      case PLANAR_YCBCR:
        if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
            img = new DeprecatedSharedPlanarYCbCrImage(GetForwarder());
        } else {
            img = new SharedPlanarYCbCrImage(this);
        }
        return img.forget();
      case SHARED_RGB:
        if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
            img = new DeprecatedSharedRGBImage(GetForwarder());
        } else {
            img = new SharedRGBImage(this);
        }
        return img.forget();
#ifdef MOZ_WIDGET_GONK
      case GRALLOC_PLANAR_YCBCR:
        img = new GrallocImage();
        return img.forget();
#endif
    }
  }
  return nullptr;
}

}
}
