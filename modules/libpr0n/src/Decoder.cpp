





































#include "Decoder.h"
#include "nsIServiceManager.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"

namespace mozilla {
namespace imagelib {

Decoder::Decoder()
  : mDecodeFlags(0)
  , mFrameCount(0)
  , mFailCode(NS_OK)
  , mInitialized(false)
  , mSizeDecode(false)
  , mInFrame(false)
  , mDecodeDone(false)
  , mDataError(false)
{
}

Decoder::~Decoder()
{
  NS_WARN_IF_FALSE(!mInFrame, "Shutting down decoder mid-frame!");
  mInitialized = false;
}





void
Decoder::Init(RasterImage* aImage, imgIDecoderObserver* aObserver)
{
  
  NS_ABORT_IF_FALSE(aImage, "Can't initialize decoder without an image!");

  
  NS_ABORT_IF_FALSE(mImage == nsnull, "Can't re-initialize a decoder!");

  
  mImage = aImage;
  mObserver = aObserver;

  
  if (!IsSizeDecode() && mObserver)
      mObserver->OnStartDecode(nsnull);

  
  InitInternal();
  mInitialized = true;
}

void
Decoder::Write(const char* aBuffer, PRUint32 aCount)
{
  
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (HasDataError())
    return;

  
  WriteInternal(aBuffer, aCount);
}

void
Decoder::Finish()
{
  
  if (!HasError())
    FinishInternal();

  
  if (mInFrame && !HasDecoderError())
    PostFrameStop();

  
  
  if (!IsSizeDecode() && !mDecodeDone) {

    
    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    nsCOMPtr<nsIScriptError2> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);

    if (consoleService && errorObject && !HasDecoderError()) {
      nsAutoString msg(NS_LITERAL_STRING("Image corrupt or truncated: ") +
                       NS_ConvertASCIItoUTF16(mImage->GetURIString()));

      errorObject->InitWithWindowID
        (msg.get(),
         NS_ConvertUTF8toUTF16(mImage->GetURIString()).get(),
         nsnull,
         0, 0, nsIScriptError::errorFlag,
         "Image", mImage->WindowID()
         );
  
      nsCOMPtr<nsIScriptError> error = do_QueryInterface(errorObject);
      consoleService->LogMessage(error);
    }

    
    bool salvage = !HasDecoderError() && mImage->GetNumFrames();

    
    if (salvage)
      mImage->DecodingComplete();

    
    if (mObserver) {
      mObserver->OnStopContainer(nsnull, mImage);
      mObserver->OnStopDecode(nsnull, salvage ? NS_OK : NS_ERROR_FAILURE, nsnull);
    }
  }
}

void
Decoder::FlushInvalidations()
{
  NS_ABORT_IF_FALSE(!HasDecoderError(),
                    "Not allowed to make more decoder calls after error!");

  
  if (mInvalidRect.IsEmpty())
    return;

  
  mImage->FrameUpdated(mFrameCount - 1, mInvalidRect);

  
  if (mObserver) {
    PRBool isCurrentFrame = mImage->GetCurrentFrameIndex() == (mFrameCount - 1);
    mObserver->OnDataAvailable(nsnull, isCurrentFrame, &mInvalidRect);
  }

  
  mInvalidRect.SetEmpty();
}





void Decoder::InitInternal() { }
void Decoder::WriteInternal(const char* aBuffer, PRUint32 aCount) { }
void Decoder::FinishInternal() { }





void
Decoder::PostSize(PRInt32 aWidth, PRInt32 aHeight)
{
  
  NS_ABORT_IF_FALSE(aWidth >= 0, "Width can't be negative!");
  NS_ABORT_IF_FALSE(aHeight >= 0, "Height can't be negative!");

  
  mImage->SetSize(aWidth, aHeight);

  
  if (mObserver)
    mObserver->OnStartContainer(nsnull, mImage);
}

void
Decoder::PostFrameStart()
{
  
  NS_ABORT_IF_FALSE(!mInFrame, "Starting new frame but not done with old one!");

  
  
  NS_ABORT_IF_FALSE(mInvalidRect.IsEmpty(),
                    "Start image frame with non-empty invalidation region!");

  
  mFrameCount++;
  mInFrame = true;

  
  
  
  NS_ABORT_IF_FALSE(mFrameCount == mImage->GetNumFrames(),
                    "Decoder frame count doesn't match image's!");

  
  if (mObserver)
    mObserver->OnStartFrame(nsnull, mFrameCount - 1); 
}

void
Decoder::PostFrameStop()
{
  
  NS_ABORT_IF_FALSE(mInFrame, "Stopping frame when we didn't start one!");

  
  mInFrame = false;

  
  FlushInvalidations();

  
  if (mObserver)
    mObserver->OnStopFrame(nsnull, mFrameCount - 1); 
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

  
  mImage->DecodingComplete();
  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
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
