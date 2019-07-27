




#ifndef mozilla_image_Decoder_h
#define mozilla_image_Decoder_h

#include "FrameAnimator.h"
#include "RasterImage.h"
#include "mozilla/RefPtr.h"
#include "DecodePool.h"
#include "ImageMetadata.h"
#include "Orientation.h"
#include "SourceBuffer.h"

namespace mozilla {

namespace Telemetry {
  enum ID : uint32_t;
} 

namespace image {

class Decoder : public IResumable
{
public:

  explicit Decoder(RasterImage* aImage);

  


  void Init();

  






  nsresult Decode();

  



  void Finish();

  



  bool ShouldSyncDecode(size_t aByteLimit);

  





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

  


  bool HasProgress() const
  {
    return mProgress != NoProgress || !mInvalidRect.IsEmpty();
  }

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Decoder, override)

  
  virtual void Resume() override;

  



  
  
  
  bool IsSizeDecode() { return mSizeDecode; }
  void SetSizeDecode(bool aSizeDecode)
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mSizeDecode = aSizeDecode;
  }

  












  virtual nsresult SetTargetSize(const nsIntSize& aSize)
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  











  void SetSendPartialInvalidations(bool aSend)
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mSendPartialInvalidations = aSend;
  }

  








  void SetIterator(SourceBufferIterator&& aIterator)
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mIterator.emplace(Move(aIterator));
  }

  




  void SetImageIsTransient(bool aIsTransient)
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mImageIsTransient = aIsTransient;
  }

  




  void SetImageIsLocked()
  {
    MOZ_ASSERT(!mInitialized, "Shouldn't be initialized yet");
    mImageIsLocked = true;
  }

  bool ImageIsLocked() const { return mImageIsLocked; }

  size_t BytesDecoded() const { return mBytesDecoded; }

  
  TimeDuration DecodeTime() const { return mDecodeTime; }

  
  uint32_t ChunkCount() const { return mChunkCount; }

  
  
  uint32_t GetFrameCount() { return mFrameCount; }

  
  
  uint32_t GetCompleteFrameCount() {
    return mInFrame ? mFrameCount - 1 : mFrameCount;
  }

  
  bool HasError() const { return HasDataError() || HasDecoderError(); }
  bool HasDataError() const { return mDataError; }
  bool HasDecoderError() const { return NS_FAILED(mFailCode); }
  nsresult GetDecoderError() const { return mFailCode; }
  void PostResizeError() { PostDataError(); }

  bool GetDecodeDone() const
  {
    return mDecodeDone || (mSizeDecode && HasSize()) || HasError() || mDataDone;
  }

  








  bool WasAborted() const { return mDecodeAborted; }

  enum DecodeStyle {
      PROGRESSIVE, 
                   
      SEQUENTIAL   
  };

  void SetFlags(uint32_t aFlags) { mFlags = aFlags; }
  uint32_t GetFlags() const { return mFlags; }
  uint32_t GetDecodeFlags() const { return DecodeFlags(mFlags); }

  bool HasSize() const { return mImageMetadata.HasSize(); }
  void SetSizeOnImage();

  nsIntSize GetSize() const
  {
    MOZ_ASSERT(HasSize());
    return mImageMetadata.GetSize();
  }

  virtual Telemetry::ID SpeedHistogram();

  ImageMetadata& GetImageMetadata() { return mImageMetadata; }

  


  RasterImage* GetImage() const { MOZ_ASSERT(mImage); return mImage.get(); }

  
  
  
  RawAccessFrameRef GetCurrentFrameRef()
  {
    return mCurrentFrame ? mCurrentFrame->RawAccessRef()
                         : RawAccessFrameRef();
  }

  








  void Write(const char* aBuffer, uint32_t aCount);


protected:
  friend class nsICODecoder;

  virtual ~Decoder();

  



  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);
  virtual void FinishInternal();
  virtual void FinishWithErrorInternal();

  



  
  
  void PostSize(int32_t aWidth,
                int32_t aHeight,
                Orientation aOrientation = Orientation());

  
  
  
  
  
  
  
  
  
  
  void PostHasTransparency();

  
  
  void PostFrameStart();

  
  
  
  
  
  void PostFrameStop(Opacity aFrameOpacity = Opacity::SOME_TRANSPARENCY,
                     DisposalMethod aDisposalMethod = DisposalMethod::KEEP,
                     int32_t aTimeout = 0,
                     BlendMethod aBlendMethod = BlendMethod::OVER);

  










  void PostInvalidation(const nsIntRect& aRect,
                        const Maybe<nsIntRect>& aRectAtTargetSize = Nothing());

  
  
  
  
  
  
  
  
  void PostDecodeDone(int32_t aLoopCount = 0);

  
  void PostDataError();
  void PostDecoderError(nsresult aFailCode);

  




  void CompleteDecode();

  







  nsresult AllocateFrame(uint32_t aFrameNum,
                         const nsIntSize& aTargetSize,
                         const nsIntRect& aFrameRect,
                         gfx::SurfaceFormat aFormat,
                         uint8_t aPaletteDepth = 0);

  
  nsresult AllocateBasicFrame() {
    nsIntSize size = GetSize();
    return AllocateFrame(0, size, nsIntRect(nsIntPoint(), size),
                         gfx::SurfaceFormat::B8G8R8A8);
  }

  RawAccessFrameRef AllocateFrameInternal(uint32_t aFrameNum,
                                          const nsIntSize& aTargetSize,
                                          const nsIntRect& aFrameRect,
                                          uint32_t aDecodeFlags,
                                          gfx::SurfaceFormat aFormat,
                                          uint8_t aPaletteDepth,
                                          imgFrame* aPreviousFrame);

  



  nsRefPtr<RasterImage> mImage;
  Maybe<SourceBufferIterator> mIterator;
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

  uint32_t mFlags;
  size_t mBytesDecoded;
  bool mSendPartialInvalidations;
  bool mDataDone;
  bool mDecodeDone;
  bool mDataError;
  bool mDecodeAborted;
  bool mShouldReportError;
  bool mImageIsTransient;
  bool mImageIsLocked;

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
