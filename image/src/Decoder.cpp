





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
  , mCurrentFrame(nullptr)
  , mProgress(NoProgress)
  , mImageData(nullptr)
  , mColormap(nullptr)
  , mDecodeFlags(0)
  , mBytesDecoded(0)
  , mDecodeDone(false)
  , mDataError(false)
  , mFrameCount(0)
  , mFailCode(NS_OK)
  , mNeedsNewFrame(false)
  , mInitialized(false)
  , mSizeDecode(false)
  , mInFrame(false)
  , mIsAnimated(false)
{ }

Decoder::~Decoder()
{
  mInitialized = false;
}





void
Decoder::Init()
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");

  
  if (!IsSizeDecode()) {
      mProgress |= FLAG_DECODE_STARTED | FLAG_ONLOAD_BLOCKED;
  }

  
  InitInternal();

  mInitialized = true;
}



void
Decoder::InitSharedDecoder(uint8_t* imageData, uint32_t imageDataLength,
                           uint32_t* colormap, uint32_t colormapSize,
                           imgFrame* currentFrame)
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");

  mImageData = imageData;
  mImageDataLength = imageDataLength;
  mColormap = colormap;
  mColormapSize = colormapSize;
  mCurrentFrame = currentFrame;
  
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

  
  MOZ_ASSERT(!HasDecoderError(),
             "Not allowed to make more decoder calls after error!");

  
  mBytesDecoded += aCount;

  
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
      if (!IsSizeDecode()) {
        mProgress |= FLAG_DECODE_COMPLETE | FLAG_ONLOAD_UNBLOCKED;
      }
      mProgress |= FLAG_HAS_ERROR;
    }
  }

  
  mImageMetadata.SetOnImage(&mImage);

  if (mDecodeDone) {
    mImage.DecodingComplete();
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

  nsresult rv;
  nsRefPtr<imgFrame> frame;
  if (mNewFrameData.mPaletteDepth) {
    rv = mImage.EnsureFrame(mNewFrameData.mFrameNum, mNewFrameData.mOffsetX,
                            mNewFrameData.mOffsetY, mNewFrameData.mWidth,
                            mNewFrameData.mHeight, mNewFrameData.mFormat,
                            mNewFrameData.mPaletteDepth,
                            &mImageData, &mImageDataLength,
                            &mColormap, &mColormapSize,
                            getter_AddRefs(frame));
  } else {
    rv = mImage.EnsureFrame(mNewFrameData.mFrameNum, mNewFrameData.mOffsetX,
                            mNewFrameData.mOffsetY, mNewFrameData.mWidth,
                            mNewFrameData.mHeight, mNewFrameData.mFormat,
                            &mImageData, &mImageDataLength,
                            getter_AddRefs(frame));
  }

  if (NS_SUCCEEDED(rv)) {
    mCurrentFrame = frame;
  } else {
    mCurrentFrame = nullptr;
  }

  
  if (NS_SUCCEEDED(rv) && mNewFrameData.mFrameNum == mFrameCount) {
    PostFrameStart();
  } else if (NS_FAILED(rv)) {
    PostDataError();
  }

  
  
  mNeedsNewFrame = false;

  return rv;
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

  
  mProgress |= FLAG_SIZE_AVAILABLE;
}

void
Decoder::PostHasTransparency()
{
  mProgress |= FLAG_HAS_TRANSPARENCY;
}

void
Decoder::PostFrameStart()
{
  
  NS_ABORT_IF_FALSE(!mInFrame, "Starting new frame but not done with old one!");

  
  mFrameCount++;
  mInFrame = true;

  
  if (mFrameCount > 1) {
    mIsAnimated = true;
    mProgress |= FLAG_IS_ANIMATED;
  }

  
  
  
  NS_ABORT_IF_FALSE(mFrameCount == mImage.GetNumFrames(),
                    "Decoder frame count doesn't match image's!");
}

void
Decoder::PostFrameStop(FrameBlender::FrameAlpha aFrameAlpha ,
                       FrameBlender::FrameDisposalMethod aDisposalMethod ,
                       int32_t aTimeout ,
                       FrameBlender::FrameBlendMethod aBlendMethod )
{
  
  MOZ_ASSERT(!IsSizeDecode(), "Stopping frame during a size decode");
  MOZ_ASSERT(mInFrame, "Stopping frame when we didn't start one");
  MOZ_ASSERT(mCurrentFrame, "Stopping frame when we don't have one");

  
  mInFrame = false;

  if (aFrameAlpha == FrameBlender::kFrameOpaque) {
    mCurrentFrame->SetHasNoAlpha();
  }

  mCurrentFrame->SetFrameDisposalMethod(aDisposalMethod);
  mCurrentFrame->SetRawTimeout(aTimeout);
  mCurrentFrame->SetBlendMethod(aBlendMethod);
  mCurrentFrame->ImageUpdated(mCurrentFrame->GetRect());

  mProgress |= FLAG_FRAME_COMPLETE | FLAG_ONLOAD_UNBLOCKED;
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
  mImageMetadata.SetIsNonPremultiplied(GetDecodeFlags() & DECODER_NO_PREMULTIPLY_ALPHA);

  mProgress |= FLAG_DECODE_COMPLETE;
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

  mNewFrameData = NewFrameData(framenum, x_offset, y_offset, width, height, format, palette_depth);
  mNeedsNewFrame = true;
}

} 
} 
