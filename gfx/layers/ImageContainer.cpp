





#include "ImageContainer.h"
#include <string.h>                     
#include "GLImages.h"                   
#include "gfx2DGlue.h"
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/CrossProcessMutex.h"  
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ImageBridgeChild.h"  
#include "mozilla/layers/ImageClient.h"  
#include "nsISupportsUtils.h"           
#include "YCbCrUtils.h"                 
#ifdef MOZ_WIDGET_GONK
#include "GrallocImages.h"
#endif
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_CAMERA) && defined(MOZ_WEBRTC)
#include "GonkCameraImage.h"
#endif
#include "gfx2DGlue.h"
#include "mozilla/gfx/2D.h"

#ifdef XP_MACOSX
#include "mozilla/gfx/QuartzSupport.h"
#include "MacIOSurfaceImage.h"
#endif

#ifdef XP_WIN
#include "gfxD2DSurface.h"
#include "gfxWindowsPlatform.h"
#include <d3d10_1.h>
#include "D3D9SurfaceImage.h"
#include "D3D11ShareHandleImage.h"
#endif

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;
using namespace android;
using namespace mozilla::gfx;

Atomic<int32_t> Image::sSerialCounter(0);

already_AddRefed<Image>
ImageFactory::CreateImage(ImageFormat aFormat,
                          const gfx::IntSize &,
                          BufferRecycleBin *aRecycleBin)
{
  nsRefPtr<Image> img;
#ifdef MOZ_WIDGET_GONK
  if (aFormat == ImageFormat::GRALLOC_PLANAR_YCBCR) {
    img = new GrallocImage();
    return img.forget();
  }
  if (aFormat == ImageFormat::OVERLAY_IMAGE) {
    img = new OverlayImage();
    return img.forget();
  }
#endif
#if defined(MOZ_WIDGET_GONK) && defined(MOZ_B2G_CAMERA) && defined(MOZ_WEBRTC)
  if (aFormat == ImageFormat::GONK_CAMERA_IMAGE) {
    img = new GonkCameraImage();
    return img.forget();
  }
#endif
  if (aFormat == ImageFormat::PLANAR_YCBCR) {
    img = new PlanarYCbCrImage(aRecycleBin);
    return img.forget();
  }
  if (aFormat == ImageFormat::CAIRO_SURFACE) {
    img = new CairoImage();
    return img.forget();
  }
#ifdef MOZ_WIDGET_ANDROID
  if (aFormat == ImageFormat::SURFACE_TEXTURE) {
    img = new SurfaceTextureImage();
    return img.forget();
  }
#endif
  if (aFormat == ImageFormat::EGLIMAGE) {
    img = new EGLImageImage();
    return img.forget();
  }
#ifdef XP_MACOSX
  if (aFormat == ImageFormat::MAC_IOSURFACE) {
    img = new MacIOSurfaceImage();
    return img.forget();
  }
#endif
#ifdef XP_WIN
  if (aFormat == ImageFormat::D3D11_SHARE_HANDLE_TEXTURE) {
    img = new D3D11ShareHandleImage();
    return img.forget();
  }
  if (aFormat == ImageFormat::D3D9_RGB32_TEXTURE) {
    img = new D3D9SurfaceImage();
    return img.forget();
  }
#endif
  return nullptr;
}

BufferRecycleBin::BufferRecycleBin()
  : mLock("mozilla.layers.BufferRecycleBin.mLock")
{
}

void
BufferRecycleBin::RecycleBuffer(uint8_t* aBuffer, uint32_t aSize)
{
  MutexAutoLock lock(mLock);

  if (!mRecycledBuffers.IsEmpty() && aSize != mRecycledBufferSize) {
    mRecycledBuffers.Clear();
  }
  mRecycledBufferSize = aSize;
  mRecycledBuffers.AppendElement(aBuffer);
}

uint8_t*
BufferRecycleBin::GetBuffer(uint32_t aSize)
{
  MutexAutoLock lock(mLock);

  if (mRecycledBuffers.IsEmpty() || mRecycledBufferSize != aSize)
    return new uint8_t[aSize];

  uint32_t last = mRecycledBuffers.Length() - 1;
  uint8_t* result = mRecycledBuffers[last].forget();
  mRecycledBuffers.RemoveElementAt(last);
  return result;
}

ImageContainer::ImageContainer(int flag)
: mReentrantMonitor("ImageContainer.mReentrantMonitor"),
  mPaintCount(0),
  mPreviousImagePainted(false),
  mImageFactory(new ImageFactory()),
  mRecycleBin(new BufferRecycleBin()),
  mCompositionNotifySink(nullptr),
  mImageClient(nullptr)
{
  if (flag == ENABLE_ASYNC && ImageBridgeChild::IsCreated()) {
    
    
    mImageClient = ImageBridgeChild::GetSingleton()->CreateImageClient(CompositableType::IMAGE).take();
    MOZ_ASSERT(mImageClient);
  }
}

ImageContainer::~ImageContainer()
{
  if (IsAsync()) {
    ImageBridgeChild::DispatchReleaseImageClient(mImageClient);
  }
}

already_AddRefed<Image>
ImageContainer::CreateImage(ImageFormat aFormat)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

#ifdef MOZ_WIDGET_GONK
  if (aFormat == ImageFormat::OVERLAY_IMAGE) {
    if (mImageClient && mImageClient->GetTextureInfo().mCompositableType != CompositableType::IMAGE_OVERLAY) {
      
      if (ImageBridgeChild::IsCreated()) {
        ImageBridgeChild::DispatchReleaseImageClient(mImageClient);
        mImageClient = ImageBridgeChild::GetSingleton()->CreateImageClient(CompositableType::IMAGE_OVERLAY).take();
      }
    }
  }
#endif
  if (mImageClient) {
    nsRefPtr<Image> img = mImageClient->CreateImage(aFormat);
    if (img) {
      return img.forget();
    }
  }
  return mImageFactory->CreateImage(aFormat, mScaleHint, mRecycleBin);
}

void
ImageContainer::SetCurrentImageInternal(Image *aImage)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mActiveImage = aImage;
  CurrentImageChanged();
}

void
ImageContainer::ClearCurrentImage()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  SetCurrentImageInternal(nullptr);
}

void
ImageContainer::SetCurrentImage(Image *aImage)
{
  if (!aImage) {
    ClearAllImages();
    return;
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (IsAsync()) {
    ImageBridgeChild::DispatchImageClientUpdate(mImageClient, this);
  }
  SetCurrentImageInternal(aImage);
}

 void
ImageContainer::ClearAllImages()
{
  if (IsAsync()) {
    
    ImageBridgeChild::FlushAllImages(mImageClient, this, false);
    return;
  }

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  SetCurrentImageInternal(nullptr);
}

void
ImageContainer::ClearAllImagesExceptFront()
{
  if (IsAsync()) {
    
    ImageBridgeChild::FlushAllImages(mImageClient, this, true);
  }
}

void
ImageContainer::SetCurrentImageInTransaction(Image *aImage)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(!mImageClient, "Should use async image transfer with ImageBridge.");

  SetCurrentImageInternal(aImage);
}

bool ImageContainer::IsAsync() const {
  return mImageClient != nullptr;
}

uint64_t ImageContainer::GetAsyncContainerID() const
{
  NS_ASSERTION(IsAsync(),"Shared image ID is only relevant to async ImageContainers");
  if (IsAsync()) {
    return mImageClient->GetAsyncID();
  } else {
    return 0; 
  }
}

bool
ImageContainer::HasCurrentImage()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  return !!mActiveImage.get();
}

already_AddRefed<Image>
ImageContainer::LockCurrentImage()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  nsRefPtr<Image> retval = mActiveImage;
  return retval.forget();
}

TemporaryRef<gfx::SourceSurface>
ImageContainer::LockCurrentAsSourceSurface(gfx::IntSize *aSize, Image** aCurrentImage)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (aCurrentImage) {
    nsRefPtr<Image> activeImage(mActiveImage);
    activeImage.forget(aCurrentImage);
  }

  if (!mActiveImage) {
    return nullptr;
  }

  *aSize = mActiveImage->GetSize();
  return mActiveImage->GetAsSourceSurface();
}

void
ImageContainer::UnlockCurrentImage()
{
}

TemporaryRef<gfx::SourceSurface>
ImageContainer::GetCurrentAsSourceSurface(gfx::IntSize *aSize)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mActiveImage)
    return nullptr;
  *aSize = mActiveImage->GetSize();
  return mActiveImage->GetAsSourceSurface();
}

gfx::IntSize
ImageContainer::GetCurrentSize()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mActiveImage) {
    return gfx::IntSize(0, 0);
  }

  return mActiveImage->GetSize();
}

PlanarYCbCrImage::PlanarYCbCrImage(BufferRecycleBin *aRecycleBin)
  : Image(nullptr, ImageFormat::PLANAR_YCBCR)
  , mBufferSize(0)
  , mOffscreenFormat(gfxImageFormat::Unknown)
  , mRecycleBin(aRecycleBin)
{
}

PlanarYCbCrImage::~PlanarYCbCrImage()
{
  if (mBuffer) {
    mRecycleBin->RecycleBuffer(mBuffer.forget(), mBufferSize);
  }
}

size_t
PlanarYCbCrImage::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  
  
  
  
  size_t size = mBuffer.SizeOfExcludingThis(aMallocSizeOf);

  
  

  return size;
}

uint8_t*
PlanarYCbCrImage::AllocateBuffer(uint32_t aSize)
{
  return mRecycleBin->GetBuffer(aSize);
}

static void
CopyPlane(uint8_t *aDst, const uint8_t *aSrc,
          const gfx::IntSize &aSize, int32_t aStride, int32_t aSkip)
{
  if (!aSkip) {
    
    memcpy(aDst, aSrc, aSize.height * aStride);
  } else {
    int32_t height = aSize.height;
    int32_t width = aSize.width;
    for (int y = 0; y < height; ++y) {
      const uint8_t *src = aSrc;
      uint8_t *dst = aDst;
      
      for (int x = 0; x < width; ++x) {
        *dst++ = *src++;
        src += aSkip;
      }
      aSrc += aStride;
      aDst += aStride;
    }
  }
}

void
PlanarYCbCrImage::CopyData(const Data& aData)
{
  mData = aData;

  
  size_t size = mData.mCbCrStride * mData.mCbCrSize.height * 2 +
                mData.mYStride * mData.mYSize.height;

  
  mBuffer = AllocateBuffer(size);
  if (!mBuffer)
    return;

  
  mBufferSize = size;

  mData.mYChannel = mBuffer;
  mData.mCbChannel = mData.mYChannel + mData.mYStride * mData.mYSize.height;
  mData.mCrChannel = mData.mCbChannel + mData.mCbCrStride * mData.mCbCrSize.height;

  CopyPlane(mData.mYChannel, aData.mYChannel,
            mData.mYSize, mData.mYStride, mData.mYSkip);
  CopyPlane(mData.mCbChannel, aData.mCbChannel,
            mData.mCbCrSize, mData.mCbCrStride, mData.mCbSkip);
  CopyPlane(mData.mCrChannel, aData.mCrChannel,
            mData.mCbCrSize, mData.mCbCrStride, mData.mCrSkip);

  mSize = aData.mPicSize;
}

void
PlanarYCbCrImage::SetData(const Data &aData)
{
  CopyData(aData);
}

gfxImageFormat
PlanarYCbCrImage::GetOffscreenFormat()
{
  return mOffscreenFormat == gfxImageFormat::Unknown ?
    gfxPlatform::GetPlatform()->GetOffscreenFormat() :
    mOffscreenFormat;
}

void
PlanarYCbCrImage::SetDataNoCopy(const Data &aData)
{
  mData = aData;
  mSize = aData.mPicSize;
}

uint8_t*
PlanarYCbCrImage::AllocateAndGetNewBuffer(uint32_t aSize)
{
  
  mBuffer = AllocateBuffer(aSize);
  if (mBuffer) {
    
    mBufferSize = aSize;
  }
  return mBuffer;
}

TemporaryRef<gfx::SourceSurface>
PlanarYCbCrImage::GetAsSourceSurface()
{
  if (mSourceSurface) {
    return mSourceSurface.get();
  }

  gfx::IntSize size(mSize);
  gfx::SurfaceFormat format = gfx::ImageFormatToSurfaceFormat(GetOffscreenFormat());
  gfx::GetYCbCrToRGBDestFormatAndSize(mData, format, size);
  if (mSize.width > PlanarYCbCrImage::MAX_DIMENSION ||
      mSize.height > PlanarYCbCrImage::MAX_DIMENSION) {
    NS_ERROR("Illegal image dest width or height");
    return nullptr;
  }

  RefPtr<gfx::DataSourceSurface> surface = gfx::Factory::CreateDataSourceSurface(size, format);
  if (NS_WARN_IF(!surface)) {
    return nullptr;
  }

  gfx::ConvertYCbCrToRGB(mData, format, size, surface->GetData(), surface->Stride());

  mSourceSurface = surface;

  return surface.forget();
}

CairoImage::CairoImage()
  : Image(nullptr, ImageFormat::CAIRO_SURFACE)
{}

CairoImage::~CairoImage()
{
}

TextureClient*
CairoImage::GetTextureClient(CompositableClient *aClient)
{
  if (!aClient) {
    return nullptr;
  }

  CompositableForwarder* forwarder = aClient->GetForwarder();
  RefPtr<TextureClient> textureClient = mTextureClients.Get(forwarder->GetSerial());
  if (textureClient) {
    return textureClient;
  }

  RefPtr<SourceSurface> surface = GetAsSourceSurface();
  MOZ_ASSERT(surface);
  if (!surface) {
    return nullptr;
  }




#ifndef XP_WIN



#ifdef MOZ_WIDGET_GONK
  RefPtr<TextureClientRecycleAllocator> recycler =
    aClient->GetTextureClientRecycler();
  if (recycler) {
    textureClient =
      recycler->CreateOrRecycleForDrawing(surface->GetFormat(),
                                          surface->GetSize(),
                                          gfx::BackendType::NONE,
                                          aClient->GetTextureFlags());
  }
#endif

#endif
  if (!textureClient) {
    
    textureClient = aClient->CreateTextureClientForDrawing(surface->GetFormat(),
                                                           surface->GetSize(),
                                                           gfx::BackendType::NONE,
                                                           TextureFlags::DEFAULT);
  }
  if (!textureClient) {
    return nullptr;
  }
  MOZ_ASSERT(textureClient->CanExposeDrawTarget());
  if (!textureClient->Lock(OpenMode::OPEN_WRITE_ONLY)) {
    return nullptr;
  }

  TextureClientAutoUnlock autoUnolck(textureClient);
  {
    
    DrawTarget* dt = textureClient->BorrowDrawTarget();
    if (!dt) {
      return nullptr;
    }
    dt->CopySurface(surface, IntRect(IntPoint(), surface->GetSize()), IntPoint());
  }

  mTextureClients.Put(forwarder->GetSerial(), textureClient);
  return textureClient;
}

} 
} 
