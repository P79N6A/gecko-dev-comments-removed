





#include "Decoder.h"
#include "nsIServiceManager.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "sampler.h"

namespace mozilla {
namespace image {

Decoder::Decoder(RasterImage &aImage)
  : mImage(aImage)
  , mCurrentFrame(nullptr)
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
Decoder::InitSharedDecoder(uint8_t* imageData, uint32_t imageDataLength,
                           uint32_t* colormap, uint32_t colormapSize,
                           imgFrame* currentFrame)
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");
  NS_ABORT_IF_FALSE(mObserver, "Need an observer!");

  mImageData = imageData;
  mImageDataLength = imageDataLength;
  mColormap = colormap;
  mColormapSize = colormapSize;
  mCurrentFrame = currentFrame;

  
  InitInternal();
  mInitialized = true;
}

void
Decoder::Write(const char* aBuffer, uint32_t aCount)
{
  SAMPLE_LABEL("ImageDecoder", "Write");

  
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (HasDataError())
    return;

  nsresult rv = NS_OK;

  
  if (mNeedsNewFrame) {
    rv = AllocateFrame();
    if (NS_FAILED(rv)) {
      PostDataError();
      return;
    }
  }

  
  WriteInternal(aBuffer, aCount);

  
  
  while (mNeedsNewFrame && !HasDataError()) {
    nsresult rv = AllocateFrame();

    if (NS_SUCCEEDED(rv)) {
      
      WriteInternal(nullptr, 0);
    } else {
      PostDataError();
      break;
    }
  }
}

void
Decoder::Finish(RasterImage::eShutdownIntent aShutdownIntent)
{
  
  if (!HasError())
    FinishInternal();

  
  if (mInFrame && !HasDecoderError())
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

    bool usable = true;
    if (aShutdownIntent != RasterImage::eShutdownIntent_NotNeeded && !HasDecoderError()) {
      
      if (GetCompleteFrameCount() == 0) {
        usable = false;
      }
    }

    
    
    if (usable) {
      PostDecodeDone();
    } else {
      if (mObserver) {
        mObserver->OnStopDecode(NS_ERROR_FAILURE);
      }
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
  if (!HasError()) {
    FinishInternal();
  }
}

nsresult
Decoder::AllocateFrame()
{
  MOZ_ASSERT(mNeedsNewFrame);

  nsresult rv;
  if (mNewFrameData.mPaletteDepth) {
    rv = mImage.EnsureFrame(mNewFrameData.mFrameNum, mNewFrameData.mOffsetX,
                            mNewFrameData.mOffsetY, mNewFrameData.mWidth,
                            mNewFrameData.mHeight, mNewFrameData.mFormat,
                            mNewFrameData.mPaletteDepth,
                            &mImageData, &mImageDataLength,
                            &mColormap, &mColormapSize, &mCurrentFrame);
  } else {
    rv = mImage.EnsureFrame(mNewFrameData.mFrameNum, mNewFrameData.mOffsetX,
                            mNewFrameData.mOffsetY, mNewFrameData.mWidth,
                            mNewFrameData.mHeight, mNewFrameData.mFormat,
                            &mImageData, &mImageDataLength, &mCurrentFrame);
  }

  if (NS_SUCCEEDED(rv)) {
    PostFrameStart();
  }

  
  
  mNeedsNewFrame = false;

  return rv;
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
  mImage.SetSize(mImageMetadata.GetWidth(), mImageMetadata.GetHeight());
}





void Decoder::InitInternal() { }
void Decoder::WriteInternal(const char* aBuffer, uint32_t aCount) { }
void Decoder::FinishInternal() { }





void
Decoder::PostSize(int32_t aWidth, int32_t aHeight)
{
  
  NS_ABORT_IF_FALSE(aWidth >= 0, "Width can't be negative!");
  NS_ABORT_IF_FALSE(aHeight >= 0, "Height can't be negative!");

  
  mImageMetadata.SetSize(aWidth, aHeight);

  
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

  
  
  
  NS_ABORT_IF_FALSE(mFrameCount == mImage.GetNumFrames(),
                    "Decoder frame count doesn't match image's!");

  
  if (mObserver) {
    mObserver->OnStartFrame();
  }
}

void
Decoder::PostFrameStop(RasterImage::FrameAlpha aFrameAlpha ,
                       RasterImage::FrameDisposalMethod aDisposalMethod ,
                       int32_t aTimeout ,
                       RasterImage::FrameBlendMethod aBlendMethod )
{
  
  NS_ABORT_IF_FALSE(mInFrame, "Stopping frame when we didn't start one!");
  NS_ABORT_IF_FALSE(mCurrentFrame, "Stopping frame when we don't have one!");

  
  mInFrame = false;

  if (aFrameAlpha == RasterImage::kFrameOpaque) {
    mCurrentFrame->SetHasNoAlpha();
  }

  mCurrentFrame->SetFrameDisposalMethod(aDisposalMethod);
  mCurrentFrame->SetTimeout(aTimeout);
  mCurrentFrame->SetBlendMethod(aBlendMethod);

  
  FlushInvalidations();

  mCurrentFrame = nullptr;

  
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
  mImageMetadata.SetIsNonPremultiplied(GetDecodeFlags() & DECODER_NO_PREMULTIPLY_ALPHA);

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
                      gfxASurface::gfxImageFormat format,
                      uint8_t palette_depth )
{
  
  MOZ_ASSERT(!mNeedsNewFrame);

  
  MOZ_ASSERT(framenum == mFrameCount || framenum == (mFrameCount + 1));

  mNewFrameData = NewFrameData(framenum, x_offset, y_offset, width, height, format, palette_depth);
  mNeedsNewFrame = true;
}

} 
} 
