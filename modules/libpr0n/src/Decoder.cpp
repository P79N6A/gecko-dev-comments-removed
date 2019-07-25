





































#include "Decoder.h"

namespace mozilla {
namespace imagelib {

Decoder::Decoder()
  : mFrameCount(0)
  , mInitialized(false)
  , mSizeDecode(false)
  , mInFrame(false)
{
}

Decoder::~Decoder()
{
  NS_WARN_IF_FALSE(!mInFrame, "Shutting down decoder mid-frame!");
  mInitialized = false;
}





nsresult
Decoder::Init(RasterImage* aImage, imgIDecoderObserver* aObserver)
{
  
  NS_ABORT_IF_FALSE(aImage, "Can't initialize decoder without an image!");

  
  NS_ABORT_IF_FALSE(mImage == nsnull, "Can't re-initialize a decoder!");

  
  mImage = aImage;
  mObserver = aObserver;

  
  nsresult rv = InitInternal();
  mInitialized = true;
  return rv;
}

nsresult
Decoder::Write(const char* aBuffer, PRUint32 aCount)
{
  
  return WriteInternal(aBuffer, aCount);
}

nsresult
Decoder::Finish()
{
  
  return FinishInternal();
}

void
Decoder::FlushInvalidations()
{
  
  if (mInvalidRect.IsEmpty())
    return;

  
  mImage->FrameUpdated(mFrameCount - 1, mInvalidRect);

  
  if (mObserver) {
    PRBool isCurrentFrame = mImage->GetCurrentFrameIndex() == (mFrameCount - 1);
    mObserver->OnDataAvailable(nsnull, isCurrentFrame, &mInvalidRect);
  }

  
  mInvalidRect.Empty();
}





nsresult Decoder::InitInternal() {return NS_OK; }
nsresult Decoder::WriteInternal(const char* aBuffer, PRUint32 aCount) {return NS_OK; }
nsresult Decoder::FinishInternal() {return NS_OK; }





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

} 
} 
