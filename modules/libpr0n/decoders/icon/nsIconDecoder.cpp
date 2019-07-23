






































#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "imgIContainer.h"
#include "imgIContainerObserver.h"
#include "imgILoad.h"
#include "nspr.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsComponentManagerUtils.h"

#include "nsIInterfaceRequestorUtils.h"

NS_IMPL_THREADSAFE_ADDREF(nsIconDecoder)
NS_IMPL_THREADSAFE_RELEASE(nsIconDecoder)

NS_INTERFACE_MAP_BEGIN(nsIconDecoder)
   NS_INTERFACE_MAP_ENTRY(imgIDecoder)
NS_INTERFACE_MAP_END_THREADSAFE

nsIconDecoder::nsIconDecoder()
{
}

nsIconDecoder::~nsIconDecoder()
{ }




NS_IMETHODIMP nsIconDecoder::Init(imgILoad *aLoad)
{
  mObserver = do_QueryInterface(aLoad);  

  mImage = do_CreateInstance("@mozilla.org/image/container;2");
  if (!mImage) return NS_ERROR_OUT_OF_MEMORY;

  aLoad->SetImage(mImage);                                                   

  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Close()
{
  mImage->DecodingComplete();

  if (mObserver) 
  {
    mObserver->OnStopFrame(nsnull, 0);
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Flush()
{
  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  
  PRUint32 readLen;
  PRUint8 header[2];
  nsresult rv = inStr->Read((char*)header, 2, &readLen);
  NS_ENSURE_TRUE(readLen == 2, NS_ERROR_UNEXPECTED); 
  count -= 2;

  PRInt32 w = header[0];
  PRInt32 h = header[1];
  NS_ENSURE_TRUE(w > 0 && h > 0, NS_ERROR_UNEXPECTED);

  if (mObserver)
    mObserver->OnStartDecode(nsnull);
  mImage->Init(w, h, mObserver);
  if (mObserver)
    mObserver->OnStartContainer(nsnull, mImage);

  PRUint32 imageLen;
  PRUint8 *imageData;

  rv = mImage->AppendFrame(0, 0, w, h, gfxASurface::ImageFormatARGB32, &imageData, &imageLen);
  if (NS_FAILED(rv))
    return rv;

  if (mObserver)
    mObserver->OnStartFrame(nsnull, 0);

  
  NS_ENSURE_TRUE(count >= imageLen, NS_ERROR_UNEXPECTED);

  
  rv = inStr->Read((char*)imageData, imageLen, &readLen);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(readLen == imageLen, NS_ERROR_UNEXPECTED);

  
  nsIntRect r(0, 0, w, h);
  rv = mImage->FrameUpdated(0, r);
  if (NS_FAILED(rv))
    return rv;

  mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);

  return NS_OK;
}

