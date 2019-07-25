





































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
  return Shutdown(aFlags);
}

NS_IMETHODIMP Decoder::Flush()
{
  return NS_OK;
}

Decoder::Decoder()
  : mInitialized(false)
  , mSizeDecode(false)
{
}

Decoder::~Decoder()
{
  mInitialized = false;
}





nsresult
Decoder::Init(RasterImage* aImage, imgIDecoderObserver* aObserver)
{
  
  NS_ABORT_IF_FALSE(aImage, "Can't initialize decoder without an image!");

  
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

nsresult
Decoder::Shutdown(PRUint32 aFlags)
{
  
  nsresult rv = ShutdownInternal(aFlags);

  
  mImage = nsnull;
  mObserver = nsnull;

  return rv;
}





nsresult Decoder::InitInternal() {return NS_OK; }
nsresult Decoder::WriteInternal(const char* aBuffer, PRUint32 aCount) {return NS_OK; }
nsresult Decoder::FinishInternal() {return NS_OK; }
nsresult Decoder::ShutdownInternal(PRUint32 aFlags) {return NS_OK; }





void
Decoder::PostSize(PRInt32 aWidth, PRInt32 aHeight)
{
  
  NS_ABORT_IF_FALSE(aWidth >= 0, "Width can't be negative!");
  NS_ABORT_IF_FALSE(aHeight >= 0, "Height can't be negative!");

  
  mImage->SetSize(aWidth, aHeight);

  
  if (mObserver)
    mObserver->OnStartContainer(nsnull, mImage);
}

} 
} 
