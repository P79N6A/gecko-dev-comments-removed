





































#include "imgTools.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "ImageErrors.h"
#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "imgIEncoder.h"
#include "imgIDecoderObserver.h"
#include "imgIContainerObserver.h"
#include "gfxContext.h"
#include "nsStringStream.h"
#include "nsComponentManagerUtils.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsStreamUtils.h"
#include "nsNetUtil.h"
#include "imgContainer.h"





NS_IMPL_ISUPPORTS1(imgTools, imgITools)

imgTools::imgTools()
{
  
}

imgTools::~imgTools()
{
  
}


NS_IMETHODIMP imgTools::DecodeImageData(nsIInputStream* aInStr,
                                        const nsACString& aMimeType,
                                        imgIContainer **aContainer)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aInStr);
  
  if (!*aContainer) {
    NS_NEWXPCOM(*aContainer, imgContainer);
    if (!*aContainer)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aContainer);
  }

  
  
  nsCString mimeType(aMimeType);
  rv = (*aContainer)->Init(nsnull, mimeType.get(), imgIContainer::INIT_FLAG_NONE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> inStream = aInStr;
  if (!NS_InputStreamIsBuffered(aInStr)) {
    nsCOMPtr<nsIInputStream> bufStream;
    rv = NS_NewBufferedInputStream(getter_AddRefs(bufStream), aInStr, 1024);
    if (NS_SUCCEEDED(rv))
      inStream = bufStream;
  }

  
  PRUint32 length;
  rv = inStream->Available(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRUint32 bytesRead;
  rv = inStream->ReadSegments(imgContainer::WriteToContainer,
                              static_cast<void*>(*aContainer),
                              length, &bytesRead);
  NS_ENSURE_SUCCESS(rv, rv);


  
  rv = (*aContainer)->SourceDataComplete();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NS_OK;
}


NS_IMETHODIMP imgTools::EncodeImage(imgIContainer *aContainer,
                                          const nsACString& aMimeType,
                                          nsIInputStream **aStream)
{
    return EncodeScaledImage(aContainer, aMimeType, 0, 0, aStream);
}


NS_IMETHODIMP imgTools::EncodeScaledImage(imgIContainer *aContainer,
                                          const nsACString& aMimeType,
                                          PRInt32 aScaledWidth,
                                          PRInt32 aScaledHeight,
                                          nsIInputStream **aStream)
{
  nsresult rv;
  PRBool doScaling = PR_TRUE;
  PRUint8 *bitmapData;
  PRUint32 bitmapDataLength, strideSize;

  
  
  if (aScaledWidth == 0 && aScaledHeight == 0) {
    doScaling = PR_FALSE;
  } else {
    NS_ENSURE_ARG(aScaledWidth > 0);
    NS_ENSURE_ARG(aScaledHeight > 0);
  }

  
  nsCAutoString encoderCID(
    NS_LITERAL_CSTRING("@mozilla.org/image/encoder;2?type=") + aMimeType);

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(encoderCID.get());
  if (!encoder)
    return NS_IMAGELIB_ERROR_NO_ENCODER;

  
  nsRefPtr<gfxImageSurface> frame;
  rv = aContainer->CopyFrame(imgIContainer::FRAME_CURRENT, PR_TRUE,
                             getter_AddRefs(frame));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!frame)
    return NS_ERROR_NOT_AVAILABLE;

  PRInt32 w = frame->Width(), h = frame->Height();
  if (!w || !h)
    return NS_ERROR_FAILURE;

  nsRefPtr<gfxImageSurface> dest;

  if (!doScaling) {
    
    aScaledWidth  = w;
    aScaledHeight = h;

    bitmapData = frame->Data();
    if (!bitmapData)
      return NS_ERROR_FAILURE;

    strideSize = frame->Stride();
    bitmapDataLength = aScaledHeight * strideSize;

  } else {
    

    
    dest = new gfxImageSurface(gfxIntSize(aScaledWidth, aScaledHeight),
                               gfxASurface::ImageFormatARGB32);
    if (!dest)
      return NS_ERROR_OUT_OF_MEMORY;

    gfxContext ctx(dest);

    
    gfxFloat sw = (double) aScaledWidth / w;
    gfxFloat sh = (double) aScaledHeight / h;
    ctx.Scale(sw, sh);

    
    ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx.SetSource(frame);
    ctx.Paint();

    bitmapData = dest->Data();
    strideSize = dest->Stride();
    bitmapDataLength = aScaledHeight * strideSize;
  }

  
  rv = encoder->InitFromData(bitmapData, bitmapDataLength,
                             aScaledWidth, aScaledHeight, strideSize,
                             imgIEncoder::INPUT_FORMAT_HOSTARGB, EmptyString());

  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(encoder, aStream);
}
