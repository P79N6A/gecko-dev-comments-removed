






































#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "imgIContainer.h"
#include "imgIContainerObserver.h"
#include "imgILoad.h"
#include "nspr.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsComponentManagerUtils.h"

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

  mImage = do_CreateInstance("@mozilla.org/image/container;1");
  if (!mImage) return NS_ERROR_OUT_OF_MEMORY;

  aLoad->SetImage(mImage);                                                   

  mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  if (!mFrame) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Close()
{
  if (mObserver) 
  {
    mObserver->OnStopFrame(nsnull, mFrame);
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Flush()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsIconDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
  nsresult rv;

  PRUint8 * const buf = (PRUint8 *)PR_Malloc(count);
  if (!buf) return NS_ERROR_OUT_OF_MEMORY; 
 
  
  PRUint32 readLen;
  rv = inStr->Read((char*)buf, count, &readLen);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(readLen >= 2, NS_ERROR_UNEXPECTED); 

  PRUint8 * const buf_end = buf + readLen;
  PRUint8 *data = buf;

  

  mObserver->OnStartDecode(nsnull);
  
  PRInt32 w = *(data++);
  PRInt32 h = *(data++);
  NS_ENSURE_TRUE(w > 0 && h > 0, NS_ERROR_UNEXPECTED);

  mImage->Init(w, h, mObserver);
  if (mObserver)
    mObserver->OnStartContainer(nsnull, mImage);

  gfx_format format = gfxIFormats::BGRA; 
  rv = mFrame->Init(0, 0, w, h, format, 24);
  if (NS_FAILED(rv))
    return rv;

  mImage->AppendFrame(mFrame);
  if (mObserver)
    mObserver->OnStartFrame(nsnull, mFrame);
  
  PRUint32 bpr, abpr;
  PRInt32 width, height;
  mFrame->GetImageBytesPerRow(&bpr);
  mFrame->GetAlphaBytesPerRow(&abpr);
  mFrame->GetWidth(&width);
  mFrame->GetHeight(&height);

  PRInt32 rownum;
  NS_ENSURE_TRUE(buf_end - data >= PRInt32(bpr) * height,
                 NS_ERROR_UNEXPECTED);

  for (rownum = 0; rownum < height; ++rownum, data += bpr)
    mFrame->SetImageData(data, bpr, rownum * bpr);

  nsIntRect r(0, 0, width, height);
  mObserver->OnDataAvailable(nsnull, mFrame, &r);

  PR_Free(buf);

  return NS_OK;
}

