





































#include "Decoder.h"

namespace mozilla {
namespace imagelib {


NS_IMPL_ISUPPORTS1(Decoder, imgIDecoder)




NS_IMETHODIMP
Decoder::Init(imgIContainer *aImage,
              imgIDecoderObserver *aObserver,
              PRUint32 aFlags)
{
  NS_ABORT_IF_FALSE(aImage->GetType() == imgIContainer::TYPE_RASTER,
                    "wrong type of imgIContainer for decoding into");
  return Init(static_cast<RasterImage*>(aImage), aObserver, aFlags);
}

NS_IMETHODIMP
Decoder::Close(PRUint32 aFlags)
{
  NS_ABORT_IF_FALSE(0, "Not reached!");
  return NS_OK;
}

NS_IMETHODIMP Decoder::Flush()
{
  return NS_OK;
}

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


NS_IMETHODIMP
Decoder::Write(const char* aBuffer, PRUint32 aCount)
{
  
  return WriteInternal(aBuffer, aCount);
}

nsresult
Decoder::Finish()
{
  
  return FinishInternal();
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

  
  if (mObserver)
    mObserver->OnStopFrame(nsnull, mFrameCount - 1); 
}

} 
} 
