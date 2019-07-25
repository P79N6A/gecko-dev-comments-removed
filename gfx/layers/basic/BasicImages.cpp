




































#include "mozilla/Monitor.h"

#include "ImageLayers.h"
#include "BasicLayers.h"
#include "gfxImageSurface.h"

#ifdef XP_MACOSX
#include "gfxQuartzImageSurface.h"
#endif

#include "cairo.h"

#include "yuv_convert.h"

using mozilla::Monitor;

namespace mozilla {
namespace layers {




class BasicImageImplData {
public:
  


  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;

  gfxIntSize GetSize() { return mSize; }

protected:
  gfxIntSize mSize;
};







class BasicCairoImage : public CairoImage, public BasicImageImplData {
public:
  BasicCairoImage() : CairoImage(static_cast<BasicImageImplData*>(this)) {}

  virtual void SetData(const Data& aData)
  {
    mSurface = aData.mSurface;
    mSize = aData.mSize;
  }

  virtual already_AddRefed<gfxASurface> GetAsSurface()
  {
    NS_ASSERTION(NS_IsMainThread(), "Must be main thread");
    nsRefPtr<gfxASurface> surface = mSurface.get();
    return surface.forget();
  }

protected:
  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
};






class BasicPlanarYCbCrImage : public PlanarYCbCrImage, public BasicImageImplData {
public:
  BasicPlanarYCbCrImage() :
    PlanarYCbCrImage(static_cast<BasicImageImplData*>(this))
    {}

  virtual void SetData(const Data& aData);

  virtual already_AddRefed<gfxASurface> GetAsSurface();

protected:
  nsAutoArrayPtr<PRUint8>              mBuffer;
  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
};

void
BasicPlanarYCbCrImage::SetData(const Data& aData)
{
  
  if (aData.mYSize.width > 16384 || aData.mYSize.height > 16384) {
    NS_ERROR("Illegal width or height");
    return;
  }
  size_t size = aData.mPicSize.width*aData.mPicSize.height*4;
  mBuffer = new PRUint8[size];
  if (!mBuffer) {
    
    return;
  }

  gfx::YUVType type = gfx::YV12;
  if (aData.mYSize.width == aData.mCbCrSize.width &&
      aData.mYSize.height == aData.mCbCrSize.height) {
    type = gfx::YV24;
  }
  else if (aData.mYSize.width / 2 == aData.mCbCrSize.width &&
           aData.mYSize.height == aData.mCbCrSize.height) {
    type = gfx::YV16;
  }
  else if (aData.mYSize.width / 2 == aData.mCbCrSize.width &&
           aData.mYSize.height / 2 == aData.mCbCrSize.height ) {
    type = gfx::YV12;
  }
  else {
    NS_ERROR("YCbCr format not supported");
  }
 
  
  gfx::ConvertYCbCrToRGB32(aData.mYChannel,
                           aData.mCbChannel,
                           aData.mCrChannel,
                           mBuffer,
                           aData.mPicX,
                           aData.mPicY,
                           aData.mPicSize.width,
                           aData.mPicSize.height,
                           aData.mYStride,
                           aData.mCbCrStride,
                           aData.mPicSize.width*4,
                           type);                                                          
  mSize = aData.mPicSize;
}

static cairo_user_data_key_t imageSurfaceDataKey;

static void
DestroyBuffer(void* aBuffer)
{
  delete[] static_cast<PRUint8*>(aBuffer);
}

already_AddRefed<gfxASurface>
BasicPlanarYCbCrImage::GetAsSurface()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be main thread");

  if (mSurface) {
    nsRefPtr<gfxASurface> result = mSurface.get();
    return result.forget();
  }

  if (!mBuffer) {
    return nsnull;
  }
  nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface(mBuffer, mSize,
                          mSize.width * gfxASurface::BytePerPixelFromFormat(gfxASurface::ImageFormatRGB24),
                          gfxASurface::ImageFormatRGB24);
  if (!imgSurface) {
    return nsnull;
  }

  
  imgSurface->SetData(&imageSurfaceDataKey, mBuffer.forget(), DestroyBuffer);

  nsRefPtr<gfxASurface> result = imgSurface.get();
#if defined(XP_MACOSX)
  nsRefPtr<gfxQuartzImageSurface> quartzSurface =
    new gfxQuartzImageSurface(imgSurface);
  if (quartzSurface) {
    result = quartzSurface.forget();
  }
#endif
  mSurface = result.get();

  return result.forget();
}






class BasicImageContainer : public ImageContainer {
public:
  BasicImageContainer(BasicLayerManager* aManager) :
    ImageContainer(aManager), mMonitor("BasicImageContainer")
  {}
  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);
  virtual void SetCurrentImage(Image* aImage);
  virtual already_AddRefed<Image> GetCurrentImage();
  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);
  virtual gfxIntSize GetCurrentSize();
  virtual PRBool SetLayerManager(LayerManager *aManager);

protected:
  Monitor mMonitor;
  nsRefPtr<Image> mImage;
};




static PRBool
FormatInList(const Image::Format* aFormats, PRUint32 aNumFormats,
             Image::Format aFormat)
{
  for (PRUint32 i = 0; i < aNumFormats; ++i) {
    if (aFormats[i] == aFormat) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

already_AddRefed<Image>
BasicImageContainer::CreateImage(const Image::Format* aFormats,
                                 PRUint32 aNumFormats)
{
  nsRefPtr<Image> image;
  
  if (FormatInList(aFormats, aNumFormats, Image::CAIRO_SURFACE)) {
    image = new BasicCairoImage();
  } else if (FormatInList(aFormats, aNumFormats, Image::PLANAR_YCBCR)) {
    image = new BasicPlanarYCbCrImage();
  }
  return image.forget();
}

void
BasicImageContainer::SetCurrentImage(Image* aImage)
{
  MonitorAutoEnter mon(mMonitor);
  mImage = aImage;
}

already_AddRefed<Image>
BasicImageContainer::GetCurrentImage()
{
  MonitorAutoEnter mon(mMonitor);
  nsRefPtr<Image> image = mImage;
  return image.forget();
}

static BasicImageImplData*
ToImageData(Image* aImage)
{
  return static_cast<BasicImageImplData*>(aImage->GetImplData());
}

already_AddRefed<gfxASurface>
BasicImageContainer::GetCurrentAsSurface(gfxIntSize* aSizeResult)
{
  NS_PRECONDITION(NS_IsMainThread(), "Must be called on main thread");

  MonitorAutoEnter mon(mMonitor);
  if (!mImage) {
    return nsnull;
  }
  *aSizeResult = ToImageData(mImage)->GetSize();
  return ToImageData(mImage)->GetAsSurface();
}

gfxIntSize
BasicImageContainer::GetCurrentSize()
{
  MonitorAutoEnter mon(mMonitor);
  return !mImage ? gfxIntSize(0,0) : ToImageData(mImage)->GetSize();
}

PRBool
BasicImageContainer::SetLayerManager(LayerManager *aManager)
{
  if (aManager &&
      aManager->GetBackendType() != LayerManager::LAYERS_BASIC)
  {
    return PR_FALSE;
  }

  
  mManager = aManager;
  return PR_TRUE;
}

already_AddRefed<ImageContainer>
BasicLayerManager::CreateImageContainer()
{
  nsRefPtr<ImageContainer> container = new BasicImageContainer(this);
  return container.forget();
}

}
}
