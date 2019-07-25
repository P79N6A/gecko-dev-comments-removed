




































#include "mozilla/ReentrantMonitor.h"

#include "ImageLayers.h"
#include "BasicLayers.h"
#include "gfxImageSurface.h"

#ifdef XP_MACOSX
#include "gfxQuartzImageSurface.h"
#endif

#include "cairo.h"

#include "yuv_convert.h"
#include "ycbcr_to_rgb565.h"

#include "gfxPlatform.h"

using mozilla::ReentrantMonitor;

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
   typedef gfxASurface::gfxImageFormat gfxImageFormat;
public:
   



  BasicPlanarYCbCrImage(const gfxIntSize& aScaleHint) :
    PlanarYCbCrImage(static_cast<BasicImageImplData*>(this)),
    mScaleHint(aScaleHint),
    mOffscreenFormat(gfxASurface::ImageFormatUnknown),
    mDelayedConversion(PR_FALSE)
    {}

  virtual void SetData(const Data& aData);
  virtual void SetDelayedConversion(PRBool aDelayed) { mDelayedConversion = aDelayed; }

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  const Data* GetData() { return &mData; }

  void SetOffscreenFormat(gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  gfxImageFormat GetOffscreenFormat() { return mOffscreenFormat; }

protected:
  nsAutoArrayPtr<PRUint8>              mBuffer;
  nsCountedRef<nsMainThreadSurfaceRef> mSurface;
  gfxIntSize                           mScaleHint;
  PRInt32                              mStride;
  gfxImageFormat                       mOffscreenFormat;
  Data                                 mData;
  PRUint32                             mBufferSize;
  PRPackedBool                         mDelayedConversion;
};

void
BasicPlanarYCbCrImage::SetData(const Data& aData)
{
  
  if (aData.mYSize.width > 16384 || aData.mYSize.height > 16384) {
    NS_ERROR("Illegal width or height");
    return;
  }
  
  gfx::YUVType type = gfx::YV12;
  int width_shift = 0;
  int height_shift = 0;
  if (aData.mYSize.width == aData.mCbCrSize.width &&
      aData.mYSize.height == aData.mCbCrSize.height) {
    type = gfx::YV24;
    width_shift = 0;
    height_shift = 0;
  }
  else if (aData.mYSize.width / 2 == aData.mCbCrSize.width &&
           aData.mYSize.height == aData.mCbCrSize.height) {
    type = gfx::YV16;
    width_shift = 1;
    height_shift = 0;
  }
  else if (aData.mYSize.width / 2 == aData.mCbCrSize.width &&
           aData.mYSize.height / 2 == aData.mCbCrSize.height ) {
    type = gfx::YV12;
    width_shift = 1;
    height_shift = 1;
  }
  else {
    NS_ERROR("YCbCr format not supported");
  }

  if (mDelayedConversion) {

    mData = aData;
    mData.mCbCrStride = mData.mCbCrSize.width = aData.mPicSize.width >> width_shift;
    
    
    if (width_shift && (aData.mPicSize.width & 1)) {
      mData.mCbCrStride++;
      mData.mCbCrSize.width++;
    }
    mData.mCbCrSize.height = aData.mPicSize.height >> height_shift;
    if (height_shift && (aData.mPicSize.height & 1)) {
        mData.mCbCrSize.height++;
    }
    mData.mYSize = aData.mPicSize;
    mData.mYStride = mData.mYSize.width;
    mBufferSize = mData.mCbCrStride * mData.mCbCrSize.height * 2 +
                  mData.mYStride * mData.mYSize.height;
    mBuffer = new PRUint8[mBufferSize];
    
    mData.mYChannel = mBuffer;
    mData.mCbChannel = mData.mYChannel + mData.mYStride * mData.mYSize.height;
    mData.mCrChannel = mData.mCbChannel + mData.mCbCrStride * mData.mCbCrSize.height;
    int cbcr_x = aData.mPicX >> width_shift;
    int cbcr_y = aData.mPicY >> height_shift;

    for (int i = 0; i < mData.mYSize.height; i++) {
      memcpy(mData.mYChannel + i * mData.mYStride,
             aData.mYChannel + ((aData.mPicY + i) * aData.mYStride) + aData.mPicX,
             mData.mYStride);
    }
    for (int i = 0; i < mData.mCbCrSize.height; i++) {
      memcpy(mData.mCbChannel + i * mData.mCbCrStride,
             aData.mCbChannel + ((cbcr_y + i) * aData.mCbCrStride) + cbcr_x,
             mData.mCbCrStride);
    }
    for (int i = 0; i < mData.mCbCrSize.height; i++) {
      memcpy(mData.mCrChannel + i * mData.mCbCrStride,
             aData.mCrChannel + ((cbcr_y + i) * aData.mCbCrStride) + cbcr_x,
             mData.mCbCrStride);
    }

    
    mData.mPicX = mData.mPicY = 0;
    mSize = aData.mPicSize;
    return;
  }

  gfxASurface::gfxImageFormat format = GetOffscreenFormat();

  
  
  PRBool prescale = mScaleHint.width > 0 && mScaleHint.height > 0 &&
                    mScaleHint != aData.mPicSize;
  if (format == gfxASurface::ImageFormatRGB16_565) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (prescale &&
        !gfx::IsScaleYCbCrToRGB565Fast(aData.mPicX,
                                       aData.mPicY,
                                       aData.mPicSize.width,
                                       aData.mPicSize.height,
                                       mScaleHint.width,
                                       mScaleHint.height,
                                       type,
                                       gfx::FILTER_BILINEAR) &&
        gfx::IsConvertYCbCrToRGB565Fast(aData.mPicX,
                                        aData.mPicY,
                                        aData.mPicSize.width,
                                        aData.mPicSize.height,
                                        type)) {
      prescale = PR_FALSE;
    }
#else
    
    format = gfxASurface::ImageFormatRGB24;
#endif
  }
  else if (format != gfxASurface::ImageFormatRGB24) {
    
    format = gfxASurface::ImageFormatRGB24;
  }
  if (format == gfxASurface::ImageFormatRGB24) {
    

    if (aData.mPicX != 0 || aData.mPicY != 0 || type == gfx::YV24)
      prescale = PR_FALSE;
  }

  gfxIntSize size(prescale ? mScaleHint.width : aData.mPicSize.width,
                  prescale ? mScaleHint.height : aData.mPicSize.height);

  mStride = gfxASurface::FormatStrideForWidth(format, size.width);
  mBuffer = new PRUint8[size.height * mStride];
  if (!mBuffer) {
    
    return;
  }

  
  if (size != aData.mPicSize) {
#if defined(HAVE_YCBCR_TO_RGB565)
    if (format == gfxASurface::ImageFormatRGB16_565) {
      gfx::ScaleYCbCrToRGB565(aData.mYChannel,
                              aData.mCbChannel,
                              aData.mCrChannel,
                              mBuffer,
                              aData.mPicX,
                              aData.mPicY,
                              aData.mPicSize.width,
                              aData.mPicSize.height,
                              size.width,
                              size.height,
                              aData.mYStride,
                              aData.mCbCrStride,
                              mStride,
                              type,
                              gfx::FILTER_BILINEAR);
    } else
#endif
      gfx::ScaleYCbCrToRGB32(aData.mYChannel,
                             aData.mCbChannel,
                             aData.mCrChannel,
                             mBuffer,
                             aData.mPicSize.width,
                             aData.mPicSize.height,
                             size.width,
                             size.height,
                             aData.mYStride,
                             aData.mCbCrStride,
                             mStride,
                             type,
                             gfx::ROTATE_0,
                             gfx::FILTER_BILINEAR);
  } else { 
#if defined(HAVE_YCBCR_TO_RGB565)
    if (format == gfxASurface::ImageFormatRGB16_565) {
      gfx::ConvertYCbCrToRGB565(aData.mYChannel,
                                aData.mCbChannel,
                                aData.mCrChannel,
                                mBuffer,
                                aData.mPicX,
                                aData.mPicY,
                                aData.mPicSize.width,
                                aData.mPicSize.height,
                                aData.mYStride,
                                aData.mCbCrStride,
                                mStride,
                                type);
    } else 
#endif
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
                               mStride,
                               type);
  }
  SetOffscreenFormat(format);
  mSize = size;
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

  
  
  if (!mBuffer || mDelayedConversion) {
    return nsnull;
  }

  gfxASurface::gfxImageFormat format = GetOffscreenFormat();

  nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface(mBuffer, mSize, mStride, format);
  if (!imgSurface || imgSurface->CairoStatus() != 0) {
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
  typedef gfxASurface::gfxImageFormat gfxImageFormat;

  BasicImageContainer() :
    ImageContainer(nsnull),
    mScaleHint(-1, -1),
    mOffscreenFormat(gfxASurface::ImageFormatUnknown),
    mDelayed(PR_FALSE)
  {}
  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);
  virtual void SetDelayedConversion(PRBool aDelayed) { mDelayed = aDelayed; }
  virtual void SetCurrentImage(Image* aImage);
  virtual already_AddRefed<Image> GetCurrentImage();
  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);
  virtual gfxIntSize GetCurrentSize();
  virtual PRBool SetLayerManager(LayerManager *aManager);
  virtual void SetScaleHint(const gfxIntSize& aScaleHint);
  void SetOffscreenFormat(gfxImageFormat aFormat) { mOffscreenFormat = aFormat; }
  virtual LayerManager::LayersBackend GetBackendType() { return LayerManager::LAYERS_BASIC; }

protected:
  nsRefPtr<Image> mImage;
  gfxIntSize mScaleHint;
  gfxImageFormat mOffscreenFormat;
  PRPackedBool mDelayed;
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
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    image = new BasicPlanarYCbCrImage(mScaleHint);
    static_cast<BasicPlanarYCbCrImage*>(image.get())->SetOffscreenFormat(mOffscreenFormat);
    static_cast<BasicPlanarYCbCrImage*>(image.get())->SetDelayedConversion(mDelayed);
  }
  return image.forget();
}

void
BasicImageContainer::SetCurrentImage(Image* aImage)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mImage = aImage;
  CurrentImageChanged();
}

already_AddRefed<Image>
BasicImageContainer::GetCurrentImage()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
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

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  if (!mImage) {
    return nsnull;
  }
  *aSizeResult = ToImageData(mImage)->GetSize();
  return ToImageData(mImage)->GetAsSurface();
}

gfxIntSize
BasicImageContainer::GetCurrentSize()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  return !mImage ? gfxIntSize(0,0) : ToImageData(mImage)->GetSize();
}

void BasicImageContainer::SetScaleHint(const gfxIntSize& aScaleHint)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  mScaleHint = aScaleHint;
}

PRBool
BasicImageContainer::SetLayerManager(LayerManager *aManager)
{
  if (aManager &&
      aManager->GetBackendType() != LayerManager::LAYERS_BASIC)
  {
    return PR_FALSE;
  }

  return PR_TRUE;
}

already_AddRefed<ImageContainer>
BasicLayerManager::CreateImageContainer()
{
  nsRefPtr<ImageContainer> container = new BasicImageContainer();
  static_cast<BasicImageContainer*>(container.get())->
    SetOffscreenFormat(gfxPlatform::GetPlatform()->GetOffscreenFormat());
  return container.forget();
}

}
}
