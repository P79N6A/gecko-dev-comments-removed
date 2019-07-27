





#include "Decoder.h"

#include "mozilla/gfx/2D.h"
#include "imgIContainer.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "GeckoProfiler.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"

using mozilla::gfx::IntSize;
using mozilla::gfx::SurfaceFormat;

namespace mozilla {
namespace image {

Decoder::Decoder(RasterImage &aImage)
  : mImage(aImage)
  , mProgress(NoProgress)
  , mImageData(nullptr)
  , mColormap(nullptr)
  , mChunkCount(0)
  , mDecodeFlags(0)
  , mBytesDecoded(0)
  , mDecodeDone(false)
  , mDataError(false)
  , mFrameCount(0)
  , mFailCode(NS_OK)
  , mNeedsNewFrame(false)
  , mNeedsToFlushData(false)
  , mInitialized(false)
  , mSizeDecode(false)
  , mInFrame(false)
  , mIsAnimated(false)
{ }

Decoder::~Decoder()
{
  MOZ_ASSERT(mProgress == NoProgress,
             "Destroying Decoder without taking all its progress changes");
  MOZ_ASSERT(mInvalidRect.IsEmpty(),
             "Destroying Decoder without taking all its invalidations");
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
Decoder::InitSharedDecoder(uint8_t* aImageData, uint32_t aImageDataLength,
                           uint32_t* aColormap, uint32_t aColormapSize,
                           RawAccessFrameRef&& aFrameRef)
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");

  mImageData = aImageData;
  mImageDataLength = aImageDataLength;
  mColormap = aColormap;
  mColormapSize = aColormapSize;
  mCurrentFrame = Move(aFrameRef);

  
  if (!IsSizeDecode()) {
    mFrameCount++;
    PostFrameStart();
  }

  
  InitInternal();
  mInitialized = true;
}

void
Decoder::Write(const char* aBuffer, uint32_t aCount)
{
  PROFILER_LABEL("ImageDecoder", "Write",
    js::ProfileEntry::Category::GRAPHICS);

  
  MOZ_ASSERT(!HasDecoderError(),
             "Not allowed to make more decoder calls after error!");

  
  TimeStamp start = TimeStamp::Now();
  mChunkCount++;

  
  mBytesDecoded += aCount;

  
  if (aBuffer == nullptr && aCount == 0) {
    MOZ_ASSERT(mNeedsToFlushData, "Flushing when we don't need to");
    mNeedsToFlushData = false;
  }

  
  if (HasDataError())
    return;

  if (IsSizeDecode() && HasSize()) {
    
    return;
  }

  MOZ_ASSERT(!NeedsNewFrame() || HasDataError(),
             "Should not need a new frame before writing anything");
  MOZ_ASSERT(!NeedsToFlushData() || HasDataError(),
             "Should not need to flush data before writing anything");

  
  WriteInternal(aBuffer, aCount);

  
  while (NeedsNewFrame() && !HasDataError()) {
    MOZ_ASSERT(!IsSizeDecode(), "Shouldn't need new frame for size decode");

    nsresult rv = AllocateFrame();

    if (NS_SUCCEEDED(rv)) {
      
      WriteInternal(nullptr, 0);
    }

    mNeedsToFlushData = false;
  }

  
  mDecodeTime += (TimeStamp::Now() - start);
}

void
Decoder::Finish(ShutdownReason aReason)
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
    if (aReason != ShutdownReason::NOT_NEEDED && !HasDecoderError()) {
      
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

  mCurrentFrame = EnsureFrame(mNewFrameData.mFrameNum,
                              mNewFrameData.mFrameRect,
                              mDecodeFlags,
                              mNewFrameData.mFormat,
                              mNewFrameData.mPaletteDepth,
                              mCurrentFrame.get());

  if (mCurrentFrame) {
    
    mCurrentFrame->GetImageData(&mImageData, &mImageDataLength);
    mCurrentFrame->GetPaletteData(&mColormap, &mColormapSize);

    if (mNewFrameData.mFrameNum + 1 == mFrameCount) {
      PostFrameStart();
    }
  } else {
    PostDataError();
  }

  
  
  mNeedsNewFrame = false;

  
  
  if (mBytesDecoded > 0) {
    mNeedsToFlushData = true;
  }

  return mCurrentFrame ? NS_OK : NS_ERROR_FAILURE;
}

RawAccessFrameRef
Decoder::EnsureFrame(uint32_t aFrameNum,
                     const nsIntRect& aFrameRect,
                     uint32_t aDecodeFlags,
                     SurfaceFormat aFormat,
                     uint8_t aPaletteDepth,
                     imgFrame* aPreviousFrame)
{
  if (mDataError || NS_FAILED(mFailCode)) {
    return RawAccessFrameRef();
  }

  MOZ_ASSERT(aFrameNum <= mFrameCount, "Invalid frame index!");
  if (aFrameNum > mFrameCount) {
    return RawAccessFrameRef();
  }

  
  if (aFrameNum == mFrameCount) {
    return InternalAddFrame(aFrameNum, aFrameRect, aDecodeFlags, aFormat,
                            aPaletteDepth, aPreviousFrame);
  }

  
  
  
  
  
  MOZ_ASSERT(aFrameNum == 0, "Replacing a frame other than the first?");
  MOZ_ASSERT(mFrameCount == 1, "Should have only one frame");
  MOZ_ASSERT(aPreviousFrame, "Need the previous frame to replace");
  if (aFrameNum != 0 || !aPreviousFrame || mFrameCount != 1) {
    return RawAccessFrameRef();
  }

  MOZ_ASSERT(!aPreviousFrame->GetRect().IsEqualEdges(aFrameRect) ||
             aPreviousFrame->GetFormat() != aFormat ||
             aPreviousFrame->GetPaletteDepth() != aPaletteDepth,
             "Replacing first frame with the same kind of frame?");

  
  IntSize prevFrameSize = aPreviousFrame->GetImageSize();
  SurfaceCache::RemoveSurface(ImageKey(&mImage),
                              RasterSurfaceKey(prevFrameSize, aDecodeFlags, 0));
  mFrameCount = 0;
  mInFrame = false;

  
  return InternalAddFrame(aFrameNum, aFrameRect, aDecodeFlags, aFormat,
                          aPaletteDepth, nullptr);
}

RawAccessFrameRef
Decoder::InternalAddFrame(uint32_t aFrameNum,
                          const nsIntRect& aFrameRect,
                          uint32_t aDecodeFlags,
                          SurfaceFormat aFormat,
                          uint8_t aPaletteDepth,
                          imgFrame* aPreviousFrame)
{
  MOZ_ASSERT(aFrameNum <= mFrameCount, "Invalid frame index!");
  if (aFrameNum > mFrameCount) {
    return RawAccessFrameRef();
  }

  MOZ_ASSERT(mImageMetadata.HasSize());
  nsIntSize imageSize(mImageMetadata.GetWidth(), mImageMetadata.GetHeight());
  if (imageSize.width <= 0 || imageSize.height <= 0 ||
      aFrameRect.width <= 0 || aFrameRect.height <= 0) {
    NS_WARNING("Trying to add frame with zero or negative size");
    return RawAccessFrameRef();
  }

  if (!SurfaceCache::CanHold(imageSize.ToIntSize())) {
    NS_WARNING("Trying to add frame that's too large for the SurfaceCache");
    return RawAccessFrameRef();
  }

  nsRefPtr<imgFrame> frame = new imgFrame();
  bool nonPremult =
    aDecodeFlags & imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
  if (NS_FAILED(frame->InitForDecoder(imageSize, aFrameRect, aFormat,
                                      aPaletteDepth, nonPremult))) {
    NS_WARNING("imgFrame::Init should succeed");
    return RawAccessFrameRef();
  }

  RawAccessFrameRef ref = frame->RawAccessRef();
  if (!ref) {
    return RawAccessFrameRef();
  }

  bool succeeded =
    SurfaceCache::Insert(frame, ImageKey(&mImage),
                         RasterSurfaceKey(imageSize.ToIntSize(),
                                          aDecodeFlags,
                                          aFrameNum),
                         Lifetime::Persistent);
  if (!succeeded) {
    return RawAccessFrameRef();
  }

  nsIntRect refreshArea;

  if (aFrameNum == 1) {
    MOZ_ASSERT(aPreviousFrame, "Must provide a previous frame when animated");
    aPreviousFrame->SetRawAccessOnly();

    
    
    
    AnimationData previousFrameData = aPreviousFrame->GetAnimationData();
    if (previousFrameData.mDisposalMethod == DisposalMethod::CLEAR ||
        previousFrameData.mDisposalMethod == DisposalMethod::CLEAR_ALL ||
        previousFrameData.mDisposalMethod == DisposalMethod::RESTORE_PREVIOUS) {
      refreshArea = previousFrameData.mRect;
    }
  }

  if (aFrameNum > 0) {
    ref->SetRawAccessOnly();

    
    
    refreshArea.UnionRect(refreshArea, frame->GetRect());
  }

  mFrameCount++;
  mImage.OnAddedFrame(mFrameCount, refreshArea);

  return ref;
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
void Decoder::WriteInternal(const char* aBuffer, uint32_t aCount) { }
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

  
  mInFrame = true;

  
  if (mFrameCount > 1) {
    mIsAnimated = true;
    mProgress |= FLAG_IS_ANIMATED;
  }
}

void
Decoder::PostFrameStop(Opacity aFrameOpacity ,
                       DisposalMethod aDisposalMethod ,
                       int32_t aTimeout ,
                       BlendMethod aBlendMethod )
{
  
  MOZ_ASSERT(!IsSizeDecode(), "Stopping frame during a size decode");
  MOZ_ASSERT(mInFrame, "Stopping frame when we didn't start one");
  MOZ_ASSERT(mCurrentFrame, "Stopping frame when we don't have one");

  
  mInFrame = false;

  mCurrentFrame->Finish(aFrameOpacity, aDisposalMethod, aTimeout, aBlendMethod);

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

  mNewFrameData = NewFrameData(framenum,
                               nsIntRect(x_offset, y_offset, width, height),
                               format, palette_depth);
  mNeedsNewFrame = true;
}

} 
} 
