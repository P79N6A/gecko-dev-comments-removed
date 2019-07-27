




#ifndef MOZILLA_IMAGELIB_DECODER_H_
#define MOZILLA_IMAGELIB_DECODER_H_

#include "FrameAnimator.h"
#include "RasterImage.h"
#include "mozilla/RefPtr.h"
#include "DecodePool.h"
#include "ImageMetadata.h"
#include "Orientation.h"
#include "mozilla/Telemetry.h"

namespace mozilla {

namespace image {

class Decoder
{
public:

  explicit Decoder(RasterImage* aImage);

  




  void Init();

  









  void Write(const char* aBuffer, uint32_t aCount);

  




  void Finish(ShutdownReason aReason);

  





  nsIntRect TakeInvalidRect()
  {
    nsIntRect invalidRect = mInvalidRect;
    mInvalidRect.SetEmpty();
    return invalidRect;
  }

  




  Progress TakeProgress()
  {
    Progress progress = mProgress;
    mProgress = NoProgress;
    return progress;
  }

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Decoder)

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; }
  void SetSizeDecode(bool aSizeDecode)
  {
    NS_ABORT_IF_FALSE(!mInitialized, "Can't set size decode after Init()!");
    mSizeDecode = aSizeDecode;
  }

  











  void SetSendPartialInvalidations(bool aSend)
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mSendPartialInvalidations = aSend;
  }

  size_t BytesDecoded() const { return mBytesDecoded; }

  
  TimeDuration DecodeTime() const { return mDecodeTime; }

  
  uint32_t ChunkCount() const { return mChunkCount; }

  
  
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

  nsIntSize GetSize() const { return mImageMetadata.GetSize(); }
  bool HasSize() const { return mImageMetadata.HasSize(); }
  void SetSizeOnImage();

  
  virtual Telemetry::ID SpeedHistogram() { return Telemetry::HistogramCount; }

  ImageMetadata& GetImageMetadata() { return mImageMetadata; }

  


  RasterImage* GetImage() const { MOZ_ASSERT(mImage); return mImage.get(); }

  already_AddRefed<imgFrame> GetCurrentFrame()
  {
    nsRefPtr<imgFrame> frame = mCurrentFrame.get();
    return frame.forget();
  }

  RawAccessFrameRef GetCurrentFrameRef()
  {
    return mCurrentFrame ? mCurrentFrame->RawAccessRef()
                         : RawAccessFrameRef();
  }

protected:
  friend class nsICODecoder;

  virtual ~Decoder();

  



  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);
  virtual void FinishInternal();

  



  
  
  void PostSize(int32_t aWidth,
                int32_t aHeight,
                Orientation aOrientation = Orientation());

  
  
  
  
  
  
  
  
  
  
  void PostHasTransparency();

  
  
  void PostFrameStart();

  
  
  
  
  
  void PostFrameStop(Opacity aFrameOpacity = Opacity::SOME_TRANSPARENCY,
                     DisposalMethod aDisposalMethod = DisposalMethod::KEEP,
                     int32_t aTimeout = 0,
                     BlendMethod aBlendMethod = BlendMethod::OVER);

  
  
  void PostInvalidation(nsIntRect& aRect);

  
  
  
  
  
  
  
  
  void PostDecodeDone(int32_t aLoopCount = 0);

  
  void PostDataError();
  void PostDecoderError(nsresult aFailCode);

  







  nsresult AllocateFrame(uint32_t aFrameNum,
                         const nsIntRect& aFrameRect,
                         gfx::SurfaceFormat aFormat,
                         uint8_t aPaletteDepth = 0);

  RawAccessFrameRef AllocateFrameInternal(uint32_t aFrameNum,
                                          const nsIntRect& aFrameRect,
                                          uint32_t aDecodeFlags,
                                          gfx::SurfaceFormat aFormat,
                                          uint8_t aPaletteDepth,
                                          imgFrame* aPreviousFrame);

  



  nsRefPtr<RasterImage> mImage;
  RawAccessFrameRef mCurrentFrame;
  ImageMetadata mImageMetadata;
  nsIntRect mInvalidRect; 
  Progress mProgress;

  uint8_t* mImageData;       
  uint32_t mImageDataLength;
  uint32_t* mColormap;       
  uint32_t mColormapSize;

  
  TimeDuration mDecodeTime;
  uint32_t mChunkCount;

  uint32_t mDecodeFlags;
  size_t mBytesDecoded;
  bool mSendPartialInvalidations;
  bool mDecodeDone;
  bool mDataError;

private:
  uint32_t mFrameCount; 

  nsresult mFailCode;

  bool mInitialized;
  bool mSizeDecode;
  bool mInFrame;
  bool mIsAnimated;
};

} 
} 

#endif 
