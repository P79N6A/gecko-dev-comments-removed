





#include "Decoder.h"

#include "mozilla/gfx/2D.h"
#include "DecodePool.h"
#include "GeckoProfiler.h"
#include "imgIContainer.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsProxyRelease.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/Telemetry.h"

using mozilla::gfx::IntSize;
using mozilla::gfx::SurfaceFormat;

namespace mozilla {
namespace image {

Decoder::Decoder(RasterImage* aImage)
  : mImage(aImage)
  , mProgress(NoProgress)
  , mImageData(nullptr)
  , mColormap(nullptr)
  , mChunkCount(0)
  , mFlags(0)
  , mBytesDecoded(0)
  , mSendPartialInvalidations(false)
  , mDataDone(false)
  , mDecodeDone(false)
  , mDataError(false)
  , mDecodeAborted(false)
  , mShouldReportError(false)
  , mImageIsTransient(false)
  , mImageIsLocked(false)
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

  if (!NS_IsMainThread()) {
    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread!");
    if (mainThread) {
      
      RasterImage* rawImg = nullptr;
      mImage.swap(rawImg);
      DebugOnly<nsresult> rv =
        NS_ProxyRelease(mainThread, NS_ISUPPORTS_CAST(ImageResource*, rawImg));
      MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed to proxy release to main thread");
    }
  }
}





void
Decoder::Init()
{
  
  MOZ_ASSERT(!mInitialized, "Can't re-initialize a decoder!");

  
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
  
  MOZ_ASSERT(!mInitialized, "Can't re-initialize a decoder!");

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

nsresult
Decoder::Decode()
{
  MOZ_ASSERT(mInitialized, "Should be initialized here");
  MOZ_ASSERT(mIterator, "Should have a SourceBufferIterator");

  
  
  while (!GetDecodeDone() && !HasError()) {
    auto newState = mIterator->AdvanceOrScheduleResume(this);

    if (newState == SourceBufferIterator::WAITING) {
      
      
      
      
      return NS_OK;
    }

    if (newState == SourceBufferIterator::COMPLETE) {
      mDataDone = true;

      nsresult finalStatus = mIterator->CompletionStatus();
      if (NS_FAILED(finalStatus)) {
        PostDataError();
      }

      CompleteDecode();
      return finalStatus;
    }

    MOZ_ASSERT(newState == SourceBufferIterator::READY);

    Write(mIterator->Data(), mIterator->Length());
  }

  CompleteDecode();
  return HasError() ? NS_ERROR_FAILURE : NS_OK;
}

void
Decoder::Resume()
{
  DecodePool* decodePool = DecodePool::Singleton();
  MOZ_ASSERT(decodePool);

  nsCOMPtr<nsIEventTarget> target = decodePool->GetEventTarget();
  if (MOZ_UNLIKELY(!target)) {
    
    return;
  }

  nsCOMPtr<nsIRunnable> worker = decodePool->CreateDecodeWorker(this);
  target->Dispatch(worker, nsIEventTarget::DISPATCH_NORMAL);
}

bool
Decoder::ShouldSyncDecode(size_t aByteLimit)
{
  MOZ_ASSERT(aByteLimit > 0);
  MOZ_ASSERT(mIterator, "Should have a SourceBufferIterator");

  return mIterator->RemainingBytesIsNoMoreThan(aByteLimit);
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

  
  if (HasDataError()) {
    return;
  }

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
Decoder::CompleteDecode()
{
  
  if (!HasError()) {
    FinishInternal();
  }

  
  if (mInFrame && !HasError()) {
    PostFrameStop();
  }

  
  
  
  
  if (!IsSizeDecode() && !mDecodeDone && !WasAborted()) {
    mShouldReportError = true;

    
    
    if (!HasDecoderError() && GetCompleteFrameCount() > 0) {
      
      

      
      PostHasTransparency();

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
}

void
Decoder::Finish()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(HasError() || !mInFrame, "Finishing while we're still in a frame");

  
  if (mShouldReportError && !WasAborted()) {
    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);

    if (consoleService && errorObject && !HasDecoderError()) {
      nsAutoString msg(NS_LITERAL_STRING("Image corrupt or truncated: ") +
                       NS_ConvertUTF8toUTF16(mImage->GetURIString()));

      if (NS_SUCCEEDED(errorObject->InitWithWindowID(
                         msg,
                         NS_ConvertUTF8toUTF16(mImage->GetURIString()),
                         EmptyString(), 0, 0, nsIScriptError::errorFlag,
                         "Image", mImage->InnerWindowID()
                       ))) {
        consoleService->LogMessage(errorObject);
      }
    }
  }

  
  
  mImageMetadata.SetOnImage(mImage);

  if (HasSize()) {
    SetSizeOnImage();
  }

  if (mDecodeDone && !IsSizeDecode()) {
    MOZ_ASSERT(HasError() || mCurrentFrame, "Should have an error or a frame");

    
    
    
    if (!mIsAnimated && !mImageIsTransient && mCurrentFrame) {
      mCurrentFrame->SetOptimizable();
    }

    mImage->OnDecodingComplete();
  }
}

void
Decoder::FinishSharedDecoder()
{
  if (!HasError()) {
    FinishInternal();
  }
}

nsresult
Decoder::AllocateFrame(const nsIntSize& aTargetSize )
{
  MOZ_ASSERT(mNeedsNewFrame);

  nsIntSize targetSize = aTargetSize;
  if (targetSize == nsIntSize()) {
    MOZ_ASSERT(HasSize());
    targetSize = mImageMetadata.GetSize();
  }

  mCurrentFrame = EnsureFrame(mNewFrameData.mFrameNum,
                              targetSize,
                              mNewFrameData.mFrameRect,
                              GetDecodeFlags(),
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
                     const nsIntSize& aTargetSize,
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
    return InternalAddFrame(aFrameNum, aTargetSize, aFrameRect, aDecodeFlags,
                            aFormat, aPaletteDepth, aPreviousFrame);
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

  
  mInFrame = false;
  RawAccessFrameRef ref = Move(mCurrentFrame);

  MOZ_ASSERT(ref, "No ref to current frame?");

  
  nsIntSize oldSize = aPreviousFrame->GetImageSize();
  bool nonPremult =
    aDecodeFlags & imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
  if (NS_FAILED(aPreviousFrame->ReinitForDecoder(oldSize, aFrameRect, aFormat,
                                                 aPaletteDepth, nonPremult))) {
    NS_WARNING("imgFrame::ReinitForDecoder should succeed");
    mFrameCount = 0;
    aPreviousFrame->Abort();
    return RawAccessFrameRef();
  }

  return ref;
}

RawAccessFrameRef
Decoder::InternalAddFrame(uint32_t aFrameNum,
                          const nsIntSize& aTargetSize,
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

  if (aTargetSize.width <= 0 || aTargetSize.height <= 0 ||
      aFrameRect.width <= 0 || aFrameRect.height <= 0) {
    NS_WARNING("Trying to add frame with zero or negative size");
    return RawAccessFrameRef();
  }

  if (!SurfaceCache::CanHold(aTargetSize)) {
    NS_WARNING("Trying to add frame that's too large for the SurfaceCache");
    return RawAccessFrameRef();
  }

  nsRefPtr<imgFrame> frame = new imgFrame();
  bool nonPremult =
    aDecodeFlags & imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
  if (NS_FAILED(frame->InitForDecoder(aTargetSize, aFrameRect, aFormat,
                                      aPaletteDepth, nonPremult))) {
    NS_WARNING("imgFrame::Init should succeed");
    return RawAccessFrameRef();
  }

  RawAccessFrameRef ref = frame->RawAccessRef();
  if (!ref) {
    frame->Abort();
    return RawAccessFrameRef();
  }

  InsertOutcome outcome =
    SurfaceCache::Insert(frame, ImageKey(mImage.get()),
                         RasterSurfaceKey(aTargetSize,
                                          aDecodeFlags,
                                          aFrameNum),
                         Lifetime::Persistent);
  if (outcome != InsertOutcome::SUCCESS) {
    
    
    
    
    
    mDecodeAborted = true;
    ref->Abort();
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
  mImage->OnAddedFrame(mFrameCount, refreshArea);

  return ref;
}

void
Decoder::SetSizeOnImage()
{
  MOZ_ASSERT(mImageMetadata.HasSize(), "Should have size");
  MOZ_ASSERT(mImageMetadata.HasOrientation(), "Should have orientation");

  nsresult rv = mImage->SetSize(mImageMetadata.GetWidth(),
                                mImageMetadata.GetHeight(),
                                mImageMetadata.GetOrientation());
  if (NS_FAILED(rv)) {
    PostResizeError();
  }
}





void Decoder::InitInternal() { }
void Decoder::WriteInternal(const char* aBuffer, uint32_t aCount) { }
void Decoder::FinishInternal() { }





void
Decoder::PostSize(int32_t aWidth,
                  int32_t aHeight,
                  Orientation aOrientation )
{
  
  MOZ_ASSERT(aWidth >= 0, "Width can't be negative!");
  MOZ_ASSERT(aHeight >= 0, "Height can't be negative!");

  
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
  
  MOZ_ASSERT(!mInFrame, "Starting new frame but not done with old one!");

  
  mInFrame = true;

  
  if (mFrameCount > 1) {
    mIsAnimated = true;
    mProgress |= FLAG_IS_ANIMATED;
  }
}

void
Decoder::PostFrameStop(Opacity aFrameOpacity    ,
                       DisposalMethod aDisposalMethod
                                                ,
                       int32_t aTimeout         ,
                       BlendMethod aBlendMethod )
{
  
  MOZ_ASSERT(!IsSizeDecode(), "Stopping frame during a size decode");
  MOZ_ASSERT(mInFrame, "Stopping frame when we didn't start one");
  MOZ_ASSERT(mCurrentFrame, "Stopping frame when we don't have one");

  
  mInFrame = false;

  mCurrentFrame->Finish(aFrameOpacity, aDisposalMethod, aTimeout, aBlendMethod);

  mProgress |= FLAG_FRAME_COMPLETE | FLAG_ONLOAD_UNBLOCKED;

  
  
  if (!mSendPartialInvalidations && !mIsAnimated) {
    mInvalidRect.UnionRect(mInvalidRect,
                           nsIntRect(nsIntPoint(0, 0), GetSize()));
  }
}

void
Decoder::PostInvalidation(const nsIntRect& aRect,
                          const Maybe<nsIntRect>& aRectAtTargetSize
                            )
{
  
  MOZ_ASSERT(mInFrame, "Can't invalidate when not mid-frame!");
  MOZ_ASSERT(mCurrentFrame, "Can't invalidate when not mid-frame!");

  
  
  if (mSendPartialInvalidations && !mIsAnimated) {
    mInvalidRect.UnionRect(mInvalidRect, aRect);
    mCurrentFrame->ImageUpdated(aRectAtTargetSize.valueOr(aRect));
  }
}

void
Decoder::PostDecodeDone(int32_t aLoopCount )
{
  MOZ_ASSERT(!IsSizeDecode(), "Can't be done with decoding with size decode!");
  MOZ_ASSERT(!mInFrame, "Can't be done decoding if we're mid-frame!");
  MOZ_ASSERT(!mDecodeDone, "Decode already done!");
  mDecodeDone = true;

  mImageMetadata.SetLoopCount(aLoopCount);

  mProgress |= FLAG_DECODE_COMPLETE;
}

void
Decoder::PostDataError()
{
  mDataError = true;

  if (mInFrame && mCurrentFrame) {
    mCurrentFrame->Abort();
  }
}

void
Decoder::PostDecoderError(nsresult aFailureCode)
{
  MOZ_ASSERT(NS_FAILED(aFailureCode), "Not a failure code!");

  mFailCode = aFailureCode;

  
  
  NS_WARNING("Image decoding error - This is probably a bug!");

  if (mInFrame && mCurrentFrame) {
    mCurrentFrame->Abort();
  }
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

Telemetry::ID
Decoder::SpeedHistogram()
{
  
  return Telemetry::HistogramCount;
}

} 
} 
