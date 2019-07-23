





































#include "imgTools.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "ImageErrors.h"
#include "imgIContainer.h"
#include "imgILoad.h"
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





class HelperLoader : public imgILoad,
                     public imgIDecoderObserver,
                     public nsSupportsWeakReference
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMGILOAD
    NS_DECL_IMGIDECODEROBSERVER
    NS_DECL_IMGICONTAINEROBSERVER
    HelperLoader(void);

  private:
    nsCOMPtr<imgIContainer> mContainer;
};

NS_IMPL_ISUPPORTS4 (HelperLoader, imgILoad, imgIDecoderObserver, imgIContainerObserver, nsISupportsWeakReference)

HelperLoader::HelperLoader (void)
{
}


NS_IMETHODIMP
HelperLoader::GetImage(imgIContainer **aImage)
{
  *aImage = mContainer;
  NS_IF_ADDREF (*aImage);
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::SetImage(imgIContainer *aImage)
{
  mContainer = aImage;
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::GetIsMultiPartChannel(PRBool *aIsMultiPartChannel)
{
  *aIsMultiPartChannel = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStartRequest(imgIRequest *aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStartContainer(imgIRequest *aRequest, imgIContainer
*aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStartFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnDataAvailable(imgIRequest *aRequest, PRBool aCurrentFrame, const nsIntRect * aRect)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStopFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStopContainer(imgIRequest *aRequest, imgIContainer
*aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStopDecode(imgIRequest *aRequest, nsresult status, const
PRUnichar *statusArg)
{
  return NS_OK;
}


NS_IMETHODIMP
HelperLoader::OnStopRequest(imgIRequest *aRequest, PRBool aIsLastPart)
{
  return NS_OK;
}
  

NS_IMETHODIMP
HelperLoader::FrameChanged(imgIContainer *aContainer, nsIntRect * aDirtyRect)
{
  return NS_OK;
}







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

  
  nsCAutoString decoderCID(
    NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=") + aMimeType);

  nsCOMPtr<imgIDecoder> decoder = do_CreateInstance(decoderCID.get());
  if (!decoder)
    return NS_IMAGELIB_ERROR_NO_DECODER;

  
  nsCOMPtr<imgILoad> loader = new HelperLoader();
  if (!loader)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (*aContainer)
    loader->SetImage(*aContainer);

  rv = decoder->Init(loader);
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

  PRUint32 written;
  rv = decoder->WriteFrom(inStream, length, &written);
  NS_ENSURE_SUCCESS(rv, rv);
  if (written != length)
    NS_WARNING("decoder didn't eat all of its vegetables");
  rv = decoder->Flush();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = decoder->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!*aContainer)
    loader->GetImage(aContainer);

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
  rv = aContainer->CopyCurrentFrame(getter_AddRefs(frame));
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
