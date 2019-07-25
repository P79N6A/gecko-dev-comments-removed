




































#include "mozilla/ipc/Shmem.h"
#include "ImageLayers.h"
#include "gfxImageSurface.h"
#include "yuv_convert.h"

#ifdef XP_MACOSX
#include "nsCoreAnimationSupport.h"
#endif

using namespace mozilla::ipc;

namespace mozilla {
namespace layers {

already_AddRefed<Image>
ImageFactory::CreateImage(const Image::Format *aFormats,
                          PRUint32 aNumFormats,
                          const gfxIntSize &,
                          BufferRecycleBin *aRecycleBin)
{
  if (!aNumFormats) {
    return nsnull;
  }
  nsRefPtr<Image> img;
  if (FormatInList(aFormats, aNumFormats, Image::PLANAR_YCBCR)) {
    img = new PlanarYCbCrImage(aRecycleBin);
  } else if (FormatInList(aFormats, aNumFormats, Image::CAIRO_SURFACE)) {
    img = new CairoImage();
#ifdef XP_MACOSX
  } else if (FormatInList(aFormats, aNumFormats, Image::MAC_IO_SURFACE)) {
    img = new MacIOSurfaceImage();
#endif
  }
  return img.forget();
}

BufferRecycleBin::BufferRecycleBin()
  : mLock("mozilla.layers.BufferRecycleBin.mLock")
{
}

void
BufferRecycleBin::RecycleBuffer(PRUint8* aBuffer, PRUint32 aSize)
{
  MutexAutoLock lock(mLock);

  if (!mRecycledBuffers.IsEmpty() && aSize != mRecycledBufferSize) {
    mRecycledBuffers.Clear();
  }
  mRecycledBufferSize = aSize;
  mRecycledBuffers.AppendElement(aBuffer);
}

PRUint8*
BufferRecycleBin::GetBuffer(PRUint32 aSize)
{
  MutexAutoLock lock(mLock);

  if (mRecycledBuffers.IsEmpty() || mRecycledBufferSize != aSize)
    return new PRUint8[aSize];

  PRUint32 last = mRecycledBuffers.Length() - 1;
  PRUint8* result = mRecycledBuffers[last].forget();
  mRecycledBuffers.RemoveElementAt(last);
  return result;
}

ImageContainer::~ImageContainer()
{
}

already_AddRefed<Image>
ImageContainer::CreateImage(const Image::Format *aFormats,
                            PRUint32 aNumFormats)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  return mImageFactory->CreateImage(aFormats, aNumFormats, mScaleHint, mRecycleBin);
}

void
ImageContainer::SetCurrentImage(Image *aImage)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  mActiveImage = aImage;
  CurrentImageChanged();
}

already_AddRefed<gfxASurface>
ImageContainer::GetCurrentAsSurface(gfxIntSize *aSize)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mActiveImage) {
    return nsnull;
  }

  *aSize = mActiveImage->GetSize();
  return mActiveImage->GetAsSurface();
}

gfxIntSize
ImageContainer::GetCurrentSize()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (!mActiveImage) {
    return gfxIntSize(0,0);
  }

  return mActiveImage->GetSize();
}

PlanarYCbCrImage::PlanarYCbCrImage(BufferRecycleBin *aRecycleBin)
  : Image(nsnull, PLANAR_YCBCR)
  , mBufferSize(0)
  , mRecycleBin(aRecycleBin)
{
}

PlanarYCbCrImage::~PlanarYCbCrImage()
{
  if (mBuffer) {
    mRecycleBin->RecycleBuffer(mBuffer.forget(), mBufferSize);
  }
}

PRUint8* 
PlanarYCbCrImage::AllocateBuffer(PRUint32 aSize)
{
  return mRecycleBin->GetBuffer(aSize); 
}

void
PlanarYCbCrImage::CopyData(const Data& aData)
{
  mData = aData;

  mData.mYStride = mData.mYSize.width;
  mData.mCbCrStride = mData.mCbCrSize.width;

  
  mBufferSize = mData.mCbCrStride * mData.mCbCrSize.height * 2 +
                mData.mYStride * mData.mYSize.height;

  
  mBuffer = AllocateBuffer(mBufferSize); 
  if (!mBuffer)
    return;

  mData.mYChannel = mBuffer;
  mData.mCbChannel = mData.mYChannel + mData.mYStride * mData.mYSize.height;
  mData.mCrChannel = mData.mCbChannel + mData.mCbCrStride * mData.mCbCrSize.height;

  for (int i = 0; i < mData.mYSize.height; i++) {
    memcpy(mData.mYChannel + i * mData.mYStride,
           aData.mYChannel + i * aData.mYStride,
           mData.mYStride);
  }
  for (int i = 0; i < mData.mCbCrSize.height; i++) {
    memcpy(mData.mCbChannel + i * mData.mCbCrStride,
           aData.mCbChannel + i * aData.mCbCrStride,
           mData.mCbCrStride);
    memcpy(mData.mCrChannel + i * mData.mCbCrStride,
           aData.mCrChannel + i * aData.mCbCrStride,
           mData.mCbCrStride);
  }

  mSize = aData.mPicSize;
}

void
PlanarYCbCrImage::SetData(const Data &aData)
{
  CopyData(aData);
}

already_AddRefed<gfxASurface>
PlanarYCbCrImage::GetAsSurface()
{
  if (mSurface) {
    nsRefPtr<gfxASurface> result = mSurface.get();
    return result.forget();
  }

  nsRefPtr<gfxImageSurface> imageSurface =
    new gfxImageSurface(mSize, gfxASurface::ImageFormatRGB24);
  
  gfx::YUVType type = 
    gfx::TypeFromSize(mData.mYSize.width,
                      mData.mYSize.height,
                      mData.mCbCrSize.width,
                      mData.mCbCrSize.height);

  
  gfx::ConvertYCbCrToRGB32(mData.mYChannel,
                           mData.mCbChannel,
                           mData.mCrChannel,
                           imageSurface->Data(),
                           mData.mPicX,
                           mData.mPicY,
                           mData.mPicSize.width,
                           mData.mPicSize.height,
                           mData.mYStride,
                           mData.mCbCrStride,
                           imageSurface->Stride(),
                           type);

  mSurface = imageSurface;

  return imageSurface.forget().get();
}

#ifdef XP_MACOSX
void
MacIOSurfaceImage::SetData(const Data& aData)
{
  mIOSurface = nsIOSurface::LookupSurface(aData.mIOSurface->GetIOSurfaceID());
  mSize = gfxIntSize(mIOSurface->GetWidth(), mIOSurface->GetHeight());
}

already_AddRefed<gfxASurface>
MacIOSurfaceImage::GetAsSurface()
{
  return mIOSurface->GetAsSurface();
}

void
MacIOSurfaceImage::Update(ImageContainer* aContainer)
{
  if (mUpdateCallback) {
    mUpdateCallback(aContainer, mPluginInstanceOwner);
  }
}
#endif

}
}
