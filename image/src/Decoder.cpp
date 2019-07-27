





#include "Decoder.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "GeckoProfiler.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"

namespace mozilla {
namespace image {

Decoder::Decoder(RasterImage &aImage)
  : mImage(aImage)
  , mImageData(nullptr)
  , mColormap(nullptr)
  , mDecodeFlags(0)
  , mDecodeDone(false)
  , mDataError(false)
  , mFrameCount(0)
  , mFailCode(NS_OK)
  , mNeedsNewFrame(false)
  , mInitialized(false)
  , mSizeDecode(false)
  , mInFrame(false)
  , mIsAnimated(false)
{
}

Decoder::~Decoder()
{
  mInitialized = false;
}





void
Decoder::Init()
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");
  NS_ABORT_IF_FALSE(mObserver, "Need an observer!");

  
  if (!IsSizeDecode())
      mObserver->OnStartDecode();

  
  InitInternal();

  mInitialized = true;
}



void
Decoder::InitSharedDecoder(uint8_t* aImageData, uint32_t aImageDataLength,
                           uint32_t* aColormap, uint32_t aColormapSize,
                           RawAccessFrameRef&& aFrameRef)
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");
  NS_ABORT_IF_FALSE(mObserver, "Need an observer!");

  mImageData = aImageData;
  mImageDataLength = aImageDataLength;
  mColormap = aColormap;
  mColormapSize = aColormapSize;
  mCurrentFrame = Move(aFrameRef);

  
  if (!IsSizeDecode()) {
    PostFrameStart();
  }

  
  InitInternal();
  mInitialized = true;
}

void
Decoder::Write(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy)
{
  PROFILER_LABEL("ImageDecoder", "Write",
    js::ProfileEntry::Category::GRAPHICS);

  MOZ_ASSERT(NS_IsMainThread() || aStrategy == DECODE_ASYNC);

  
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (HasDataError())
    return;

  if (IsSizeDecode() && HasSize()) {
    
    return;
  }

  
  WriteInternal(aBuffer, aCount, aStrategy);

  
  
  while (aStrategy == DECODE_SYNC && NeedsNewFrame() && !HasDataError()) {
    nsresult rv = AllocateFrame();

    if (NS_SUCCEEDED(rv)) {
      
      WriteInternal(nullptr, 0, aStrategy);
    }
  }
}

void
Decoder::Finish(RasterImage::eShutdownIntent aShutdownIntent)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (!HasError())
    FinishInternal();

  
  if (mInFrame && !HasError())
    PostFrameStop();

  
  
  if (!IsSizeDecode() && !mDecodeDone) {

    
    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);

    if (consoleService && errorObject && !HasDecoderError()) {
      nsAutoString msg(NS_LITERAL_STRING("Image corrupt or truncated: ") +
                       NS_ConvertUTF8toUTF16(mImage.GetURIString()));

      if (NS_SUCCEEDED(errorObject->InitWithWindowID(
                         msg,
                         NS_ConvertUTF8toUTF16(mImage.GetURIString()),
                         EmptyString(), 0, 0, nsIScriptError::errorFlag,
                         "Image", mImage.InnerWindowID()
                       ))) {
        consoleService->LogMessage(errorObject);
      }
    }

    bool usable = !HasDecoderError();
    if (aShutdownIntent != RasterImage::eShutdownIntent_NotNeeded && !HasDecoderError()) {
      
      if (GetCompleteFrameCount() == 0) {
        usable = false;
      }
    }

    
    
    if (usable) {
      if (mInFrame) {
        PostFrameStop();
      }
      PostDecodeDone();
    } else {
      if (mObserver) {
        mObserver->OnStopDecode(NS_ERROR_FAILURE);
      }
    }
  }

  
  
  mImageMetadata.SetOnImage(&mImage);

  if (mDecodeDone) {
    MOZ_ASSERT(HasError() || mCurrentFrame, "Should have an error or a frame");
    mImage.DecodingComplete(mCurrentFrame.get());
  }
}

void
Decoder::FinishSharedDecoder()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!HasError()) {
    FinishInternal();
  }
}

nsresult
Decoder::AllocateFrame()
{
  MOZ_ASSERT(mNeedsNewFrame);
  MOZ_ASSERT(NS_IsMainThread());

  mCurrentFrame = mImage.EnsureFrame(mNewFrameData.mFrameNum,
                                     mNewFrameData.mFrameRect,
                                     mDecodeFlags,
                                     mNewFrameData.mFormat,
                                     mNewFrameData.mPaletteDepth,
                                     mCurrentFrame.get());

  if (mCurrentFrame) {
    
    mCurrentFrame->GetImageData(&mImageData, &mImageDataLength);
    mCurrentFrame->GetPaletteData(&mColormap, &mColormapSize);

    if (mNewFrameData.mFrameNum == mFrameCount) {
      PostFrameStart();
    }
  } else {
    PostDataError();
  }

  
  
  mNeedsNewFrame = false;

  return mCurrentFrame ? NS_OK : NS_ERROR_FAILURE;
}

void
Decoder::FlushInvalidations()
{
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (mInvalidRect.IsEmpty())
    return;

  if (mObserver) {
#ifdef XP_MACOSX
    
    
    
    if (mImageMetadata.HasSize()) {
      nsIntRect mImageBound(0, 0, mImageMetadata.GetWidth(), mImageMetadata.GetHeight());

      mInvalidRect.Inflate(1);
      mInvalidRect = mInvalidRect.Intersect(mImageBound);
    }
#endif
    mObserver->FrameChanged(&mInvalidRect);
  }

  
  mInvalidRect.SetEmpty();
}

void
Decoder::SetSizeOnImage()
{
  MOZ_ASSERT(mImageMetadata.HasSize(), "Should have size");
  MOZ_ASSERT(mImageMetadata.HasOrientation(), "Should have orientation");

  mImage.SetSize(mImageMetadata.GetWidth(),
                 mImageMetadata.GetHeight(),
                 mImageMetadata.GetOrientation());
}





void Decoder::InitInternal() { }
void Decoder::WriteInternal(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy) { }
void Decoder::FinishInternal() { }





void
Decoder::PostSize(int32_t aWidth,
                  int32_t aHeight,
                  Orientation aOrientation )
{
  
  NS_ABORT_IF_FALSE(aWidth >= 0, "Width can't be negative!");
  NS_ABORT_IF_FALSE(aHeight >= 0, "Height can't be negative!");

  
  mImageMetadata.SetSize(aWidth, aHeight, aOrientation);

  
  if (mObserver)
    mObserver->OnStartContainer();
}

void
Decoder::PostFrameStart()
{
  
  NS_ABORT_IF_FALSE(!mInFrame, "Starting new frame but not done with old one!");

  
  
  NS_ABORT_IF_FALSE(mInvalidRect.IsEmpty(),
                    "Start image frame with non-empty invalidation region!");

  
  mFrameCount++;
  mInFrame = true;

  
  
  
  MOZ_ASSERT(mFrameCount == mImage.GetNumFrames(),
             "Decoder frame count doesn't match image's!");

  
  if (mObserver) {
    mObserver->OnStartFrame();
  }
}

void
Decoder::PostFrameStop(FrameBlender::FrameAlpha aFrameAlpha ,
                       FrameBlender::FrameDisposalMethod aDisposalMethod ,
                       int32_t aTimeout ,
                       FrameBlender::FrameBlendMethod aBlendMethod )
{
  
  NS_ABORT_IF_FALSE(mInFrame, "Stopping frame when we didn't start one!");
  NS_ABORT_IF_FALSE(mCurrentFrame, "Stopping frame when we don't have one!");

  
  mInFrame = false;

  if (aFrameAlpha == FrameBlender::kFrameOpaque) {
    mCurrentFrame->SetHasNoAlpha();
  }

  mCurrentFrame->SetFrameDisposalMethod(aDisposalMethod);
  mCurrentFrame->SetRawTimeout(aTimeout);
  mCurrentFrame->SetBlendMethod(aBlendMethod);
  mCurrentFrame->ImageUpdated(mCurrentFrame->GetRect());

  
  FlushInvalidations();

  
  if (mObserver) {
    mObserver->OnStopFrame();
    if (mFrameCount > 1 && !mIsAnimated) {
      mIsAnimated = true;
      mObserver->OnImageIsAnimated();
    }
  }
}

void
Decoder::PostInvalidation(nsIntRect& aRect)
{
  
  NS_ABORT_IF_FALSE(mInFrame, "Can't invalidate when not mid-frame!");
  NS_ABORT_IF_FALSE(mCurrentFrame, "Can't invalidate when not mid-frame!");

  
  mInvalidRect.UnionRect(mInvalidRect, aRect);
  mCurrentFrame->ImageUpdated(aRect);
}

void
Decoder::PostDecodeDone(int32_t aLoopCount )
{
  NS_ABORT_IF_FALSE(!IsSizeDecode(), "Can't be done with decoding with size decode!");
  NS_ABORT_IF_FALSE(!mInFrame, "Can't be done decoding if we're mid-frame!");
  NS_ABORT_IF_FALSE(!mDecodeDone, "Decode already done!");
  mDecodeDone = true;

  mImageMetadata.SetLoopCount(aLoopCount);

  if (mObserver) {
    mObserver->OnStopDecode(NS_OK);
  }
}

void
Decoder::PostDataError()
{
  mDataError = true;
}

void
Decoder::PostDecoderError(nsresult aFailureCode)
{
  NS_ABORT_IF_FALSE(NS_FAILED(aFailureCode), "Not a failure code!");

  mFailCode = aFailureCode;

  
  
  NS_WARNING("Image decoding error - This is probably a bug!");
}

void
Decoder::NeedNewFrame(uint32_t framenum, uint32_t x_offset, uint32_t y_offset,
                      uint32_t width, uint32_t height,
                      gfx::SurfaceFormat format,
                      uint8_t palette_depth )
{
  
  MOZ_ASSERT(!mNeedsNewFrame);

  
  MOZ_ASSERT(framenum == mFrameCount || framenum == (mFrameCount - 1));

  mNewFrameData = NewFrameData(framenum,
                               nsIntRect(x_offset, y_offset, width, height),
                               format, palette_depth);
  mNeedsNewFrame = true;
}

} 
} 
