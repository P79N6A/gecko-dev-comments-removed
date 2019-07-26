




#ifndef MOZILLA_IMAGELIB_DECODER_H_
#define MOZILLA_IMAGELIB_DECODER_H_

#include "RasterImage.h"
#include "imgDecoderObserver.h"
#include "mozilla/RefPtr.h"
#include "ImageMetadata.h"

namespace mozilla {
namespace image {

class Decoder
{
public:

  Decoder(RasterImage& aImage);
  virtual ~Decoder();

  




  void Init();

  





  void InitSharedDecoder(uint8_t* imageData, uint32_t imageDataLength,
                         uint32_t* colormap, uint32_t colormapSize,
                         imgFrame* currentFrame);

  









  void Write(const char* aBuffer, uint32_t aCount);

  




  void Finish(RasterImage::eShutdownIntent aShutdownIntent);

  





  void FinishSharedDecoder();

  







  void FlushInvalidations();

  
  NS_INLINE_DECL_REFCOUNTING(Decoder)

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; }
  void SetSizeDecode(bool aSizeDecode)
  {
    NS_ABORT_IF_FALSE(!mInitialized, "Can't set size decode after Init()!");
    mSizeDecode = aSizeDecode;
  }

  void SetObserver(imgDecoderObserver* aObserver)
  {
    MOZ_ASSERT(aObserver);
    mObserver = aObserver;
  }

  
  
  uint32_t GetFrameCount() { return mFrameCount; }

  
  uint32_t GetCompleteFrameCount() { return mInFrame ? mFrameCount - 1 : mFrameCount; }

  
  bool HasError() { return HasDataError() || HasDecoderError(); }
  bool HasDataError() { return mDataError; }
  bool HasDecoderError() { return NS_FAILED(mFailCode); }
  nsresult GetDecoderError() { return mFailCode; }
  void PostResizeError() { PostDataError(); }
  bool GetDecodeDone() const {
    return mDecodeDone;
  }

  
  
  
  enum {
    DECODER_NO_PREMULTIPLY_ALPHA = 0x2,     
    DECODER_NO_COLORSPACE_CONVERSION = 0x4  
  };

  enum DecodeStyle {
      PROGRESSIVE, 
      SEQUENTIAL 
  };

  void SetDecodeFlags(uint32_t aFlags) { mDecodeFlags = aFlags; }
  uint32_t GetDecodeFlags() { return mDecodeFlags; }

  
  virtual Telemetry::ID SpeedHistogram() { return Telemetry::HistogramCount; }

  ImageMetadata& GetImageMetadata() { return mImageMetadata; }

  
  
  
  
  
  
  
  void NeedNewFrame(uint32_t frameNum, uint32_t x_offset, uint32_t y_offset,
                    uint32_t width, uint32_t height,
                    gfxASurface::gfxImageFormat format,
                    uint8_t palette_depth = 0);

protected:

  



  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);
  virtual void FinishInternal();

  



  
  
  void PostSize(int32_t aWidth, int32_t aHeight);

  
  
  void PostFrameStart();

  
  
  
  
  
  void PostFrameStop(RasterImage::FrameAlpha aFrameAlpha = RasterImage::kFrameHasAlpha,
                     RasterImage::FrameDisposalMethod aDisposalMethod = RasterImage::kDisposeKeep,
                     int32_t aTimeout = 0,
                     RasterImage::FrameBlendMethod aBlendMethod = RasterImage::kBlendOver);

  
  
  void PostInvalidation(nsIntRect& aRect);

  
  
  
  
  
  
  
  
  void PostDecodeDone(int32_t aLoopCount = 0);

  
  void PostDataError();
  void PostDecoderError(nsresult aFailCode);

  
  
  nsresult AllocateFrame();

  



  RasterImage &mImage;
  imgFrame* mCurrentFrame;
  RefPtr<imgDecoderObserver> mObserver;
  ImageMetadata mImageMetadata;

  uint8_t* mImageData;       
  uint32_t mImageDataLength;
  uint32_t* mColormap;       
  uint32_t mColormapSize;

  uint32_t mDecodeFlags;
  bool mDecodeDone;
  bool mDataError;

private:
  uint32_t mFrameCount; 

  nsIntRect mInvalidRect; 

  nsresult mFailCode;

  struct NewFrameData
  {
    NewFrameData()
    {}

    NewFrameData(uint32_t num, uint32_t offsetx, uint32_t offsety,
                 uint32_t width, uint32_t height,
                 gfxASurface::gfxImageFormat format, uint8_t paletteDepth)
      : mFrameNum(num)
      , mOffsetX(offsetx)
      , mOffsetY(offsety)
      , mWidth(width)
      , mHeight(height)
      , mFormat(format)
      , mPaletteDepth(paletteDepth)
    {}
    uint32_t mFrameNum;
    uint32_t mOffsetX;
    uint32_t mOffsetY;
    uint32_t mWidth;
    uint32_t mHeight;
    gfxASurface::gfxImageFormat mFormat;
    uint8_t mPaletteDepth;
  };
  NewFrameData mNewFrameData;
  bool mNeedsNewFrame;
  bool mInitialized;
  bool mSizeDecode;
  bool mInFrame;
  bool mIsAnimated;
};

} 
} 

#endif 
