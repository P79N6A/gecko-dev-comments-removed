




#include <stdint.h>                     
#include "BasicLayers.h"                
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "cairo.h"                      
#include "gfxASurface.h"                
#include "gfxImageSurface.h"            
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsAutoRef.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsThreadUtils.h"              
#ifdef XP_MACOSX
#include "gfxQuartzImageSurface.h"
#endif
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

class BasicPlanarYCbCrImage : public PlanarYCbCrImage
{
public:
  BasicPlanarYCbCrImage(const gfx::IntSize& aScaleHint, gfxImageFormat aOffscreenFormat, BufferRecycleBin *aRecycleBin)
    : PlanarYCbCrImage(aRecycleBin)
    , mScaleHint(aScaleHint)
    , mDelayedConversion(false)
  {
    SetOffscreenFormat(aOffscreenFormat);
  }

  ~BasicPlanarYCbCrImage()
  {
    if (mDecodedBuffer) {
      
      
      mRecycleBin->RecycleBuffer(mDecodedBuffer.forget(), mSize.height * mStride);
    }
  }

  virtual void SetData(const Data& aData);
  virtual void SetDelayedConversion(bool aDelayed) { mDelayedConversion = aDelayed; }

  already_AddRefed<gfxASurface> GetAsSurface();

private:
  nsAutoArrayPtr<uint8_t> mDecodedBuffer;
  gfx::IntSize mScaleHint;
  int mStride;
  bool mDelayedConversion;
};

class BasicImageFactory : public ImageFactory
{
public:
  BasicImageFactory() {}

  virtual already_AddRefed<Image> CreateImage(const ImageFormat* aFormats,
                                              uint32_t aNumFormats,
                                              const gfx::IntSize &aScaleHint,
                                              BufferRecycleBin *aRecycleBin)
  {
    if (!aNumFormats) {
      return nullptr;
    }

    nsRefPtr<Image> image;
    if (aFormats[0] == PLANAR_YCBCR) {
      image = new BasicPlanarYCbCrImage(aScaleHint, gfxPlatform::GetPlatform()->GetOffscreenFormat(), aRecycleBin);
      return image.forget();
    }

    return ImageFactory::CreateImage(aFormats, aNumFormats, aScaleHint, aRecycleBin);
  }
};

void
BasicPlanarYCbCrImage::SetData(const Data& aData)
{
  PlanarYCbCrImage::SetData(aData);

  if (mDelayedConversion) {
    return;
  }

  
  if (aData.mYSize.width > PlanarYCbCrImage::MAX_DIMENSION ||
      aData.mYSize.height > PlanarYCbCrImage::MAX_DIMENSION) {
    NS_ERROR("Illegal image source width or height");
    return;
  }

  gfxImageFormat format = GetOffscreenFormat();

  LayerIntSize size(mScaleHint.width, mScaleHint.height);
  gfxUtils::GetYCbCrToRGBDestFormatAndSize(aData, format, size);
  if (size.width > PlanarYCbCrImage::MAX_DIMENSION ||
      size.height > PlanarYCbCrImage::MAX_DIMENSION) {
    NS_ERROR("Illegal image dest width or height");
    return;
  }

  mStride = gfxASurface::FormatStrideForWidth(format, size.width);
  mDecodedBuffer = AllocateBuffer(size.height * mStride);
  if (!mDecodedBuffer) {
    
    return;
  }

  gfxUtils::ConvertYCbCrToRGB(aData, format, size, mDecodedBuffer, mStride);
  SetOffscreenFormat(format);
  mSize = size.ToUnknownSize();
}

static cairo_user_data_key_t imageSurfaceDataKey;

static void
DestroyBuffer(void* aBuffer)
{
  delete[] static_cast<uint8_t*>(aBuffer);
}

already_AddRefed<gfxASurface>
BasicPlanarYCbCrImage::GetAsSurface()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be main thread");

  if (mSurface) {
    nsRefPtr<gfxASurface> result = mSurface.get();
    return result.forget();
  }

  if (!mDecodedBuffer) {
    return PlanarYCbCrImage::GetAsSurface();
  }

  gfxImageFormat format = GetOffscreenFormat();

  nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface(mDecodedBuffer, ThebesIntSize(mSize), mStride, format);
  if (!imgSurface || imgSurface->CairoStatus() != 0) {
    return nullptr;
  }

  
  imgSurface->SetData(&imageSurfaceDataKey, mDecodedBuffer.forget(), DestroyBuffer);

  nsRefPtr<gfxASurface> result = imgSurface.get();
#if defined(XP_MACOSX)
  nsRefPtr<gfxQuartzImageSurface> quartzSurface =
    new gfxQuartzImageSurface(imgSurface);
  if (quartzSurface) {
    result = quartzSurface.forget();
  }
#endif

  mSurface = result;

  return result.forget();
}

ImageFactory*
BasicLayerManager::GetImageFactory()
{
  if (!mFactory) {
    mFactory = new BasicImageFactory();
  }

  return mFactory.get();
}

}
}
