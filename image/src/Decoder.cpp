





#include "Decoder.h"
#include "nsIServiceManager.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "sampler.h"

namespace mozilla {
namespace image {

Decoder::Decoder(RasterImage &aImage, imgDecoderObserver* aObserver)
  : mImage(aImage)
  , mObserver(aObserver)
  , mDecodeFlags(0)
  , mDecodeDone(false)
  , mDataError(false)
  , mFrameCount(0)
  , mFailCode(NS_OK)
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

  
  if (!IsSizeDecode() && mObserver)
      mObserver->OnStartDecode();

  
  InitInternal();
  mInitialized = true;
}



void
Decoder::InitSharedDecoder()
{
  
  NS_ABORT_IF_FALSE(!mInitialized, "Can't re-initialize a decoder!");

  
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

  
  WriteInternal(aBuffer, aCount);
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
    if (aShutdownIntent != RasterImage::eShutdownIntent_Interrupted && !HasDecoderError()) {
      
      if (mImage.GetNumFrames() == 0) {
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
}

void
Decoder::FinishSharedDecoder()
{
  if (!HasError()) {
    FinishInternal();
  }
}

void
Decoder::FlushInvalidations()
{
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (mInvalidRect.IsEmpty())
    return;

  
  mImage.FrameUpdated(mFrameCount - 1, mInvalidRect);

  
  if (mObserver) {
#ifdef XP_MACOSX
    
    
    
    int32_t width;
    int32_t height;

    mImage.GetWidth(&width);
    mImage.GetHeight(&height);
    nsIntRect mImageBound(0, 0, width, height);

    mInvalidRect.Inflate(1);
    mInvalidRect = mInvalidRect.Intersect(mImageBound);
#endif
    mObserver->OnDataAvailable(&mInvalidRect);
  }

  
  mInvalidRect.SetEmpty();
}





void Decoder::InitInternal() { }
void Decoder::WriteInternal(const char* aBuffer, uint32_t aCount) { }
void Decoder::FinishInternal() { }





void
Decoder::PostSize(int32_t aWidth, int32_t aHeight)
{
  
  NS_ABORT_IF_FALSE(aWidth >= 0, "Width can't be negative!");
  NS_ABORT_IF_FALSE(aHeight >= 0, "Height can't be negative!");

  
  mImage.SetSize(aWidth, aHeight);

  
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
}

void
Decoder::PostFrameStop()
{
  
  NS_ABORT_IF_FALSE(mInFrame, "Stopping frame when we didn't start one!");

  
  mInFrame = false;

  
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

  
  mInvalidRect.UnionRect(mInvalidRect, aRect);
}

void
Decoder::PostDecodeDone()
{
  NS_ABORT_IF_FALSE(!IsSizeDecode(), "Can't be done with decoding with size decode!");
  NS_ABORT_IF_FALSE(!mInFrame, "Can't be done decoding if we're mid-frame!");
  NS_ABORT_IF_FALSE(!mDecodeDone, "Decode already done!");
  mDecodeDone = true;

  
  int frames = GetFrameCount();
  bool isNonPremult = GetDecodeFlags() & DECODER_NO_PREMULTIPLY_ALPHA;
  for (int i = 0; i < frames; i++) {
    mImage.SetFrameAsNonPremult(i, isNonPremult);
  }

  
  mImage.DecodingComplete();
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

} 
} 
