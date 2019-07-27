




#include <stdint.h>                     
#include "BasicLayers.h"                
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "cairo.h"                      
#include "gfxASurface.h"                
#include "gfxPlatform.h"                
#include "gfxUtils.h"                   
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsAutoRef.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsThreadUtils.h"              
#include "mozilla/gfx/Point.h"          
#include "gfx2DGlue.h"
#include "YCbCrUtils.h"                 
#ifdef XP_MACOSX
#include "gfxQuartzImageSurface.h"
#endif

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

  virtual void SetData(const Data& aData) override;
  virtual void SetDelayedConversion(bool aDelayed) override { mDelayedConversion = aDelayed; }

  already_AddRefed<gfx::SourceSurface> GetAsSourceSurface() override;

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t size = PlanarYCbCrImage::SizeOfExcludingThis(aMallocSizeOf);
    size += mDecodedBuffer.SizeOfExcludingThis(aMallocSizeOf);
    return size;
  }

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

  virtual already_AddRefed<Image> CreateImage(ImageFormat aFormat,
                                              const gfx::IntSize &aScaleHint,
                                              BufferRecycleBin *aRecycleBin)
  {
    nsRefPtr<Image> image;
    if (aFormat == ImageFormat::PLANAR_YCBCR) {
      image = new BasicPlanarYCbCrImage(aScaleHint, gfxPlatform::GetPlatform()->GetOffscreenFormat(), aRecycleBin);
      return image.forget();
    }

    return ImageFactory::CreateImage(aFormat, aScaleHint, aRecycleBin);
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

  gfx::SurfaceFormat format = gfx::ImageFormatToSurfaceFormat(GetOffscreenFormat());

  gfx::IntSize size(mScaleHint);
  gfx::GetYCbCrToRGBDestFormatAndSize(aData, format, size);
  if (size.width > PlanarYCbCrImage::MAX_DIMENSION ||
      size.height > PlanarYCbCrImage::MAX_DIMENSION) {
    NS_ERROR("Illegal image dest width or height");
    return;
  }

  gfxImageFormat iFormat = gfx::SurfaceFormatToImageFormat(format);
  mStride = gfxASurface::FormatStrideForWidth(iFormat, size.width);
  mDecodedBuffer = AllocateBuffer(size.height * mStride);
  if (!mDecodedBuffer) {
    
    return;
  }

  gfx::ConvertYCbCrToRGB(aData, format, size, mDecodedBuffer, mStride);
  SetOffscreenFormat(iFormat);
  mSize = size;
}

already_AddRefed<gfx::SourceSurface>
BasicPlanarYCbCrImage::GetAsSourceSurface()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be main thread");

  if (mSourceSurface) {
    RefPtr<gfx::SourceSurface> surface(mSourceSurface);
    return surface.forget();
  }

  if (!mDecodedBuffer) {
    return PlanarYCbCrImage::GetAsSourceSurface();
  }

  gfxImageFormat format = GetOffscreenFormat();

  RefPtr<gfx::SourceSurface> surface;
  {
    
    
    
    RefPtr<gfx::DrawTarget> drawTarget
      = gfxPlatform::GetPlatform()->CreateDrawTargetForData(mDecodedBuffer,
                                                            mSize,
                                                            mStride,
                                                            gfx::ImageFormatToSurfaceFormat(format));
    if (!drawTarget) {
      return nullptr;
    }

    surface = drawTarget->Snapshot();
  }

  mRecycleBin->RecycleBuffer(mDecodedBuffer.forget(), mSize.height * mStride);

  mSourceSurface = surface;
  return surface.forget();
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
