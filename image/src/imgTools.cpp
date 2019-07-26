





#include "imgTools.h"

#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsString.h"
#include "nsError.h"
#include "imgLoader.h"
#include "imgICache.h"
#include "imgIContainer.h"
#include "imgIEncoder.h"
#include "gfxContext.h"
#include "nsStringStream.h"
#include "nsComponentManagerUtils.h"
#include "nsWeakReference.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsStreamUtils.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "ImageFactory.h"
#include "Image.h"
#include "ScriptedNotificationObserver.h"
#include "imgIScriptedNotificationObserver.h"

using namespace mozilla::image;





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
  NS_ABORT_IF_FALSE(*aContainer == nullptr,
                    "Cannot provide an existing image container to DecodeImageData");

  return DecodeImage(aInStr, aMimeType, aContainer);
}

NS_IMETHODIMP imgTools::DecodeImage(nsIInputStream* aInStr,
                                    const nsACString& aMimeType,
                                    imgIContainer **aContainer)
{
  nsresult rv;
  nsRefPtr<Image> image;

  NS_ENSURE_ARG_POINTER(aInStr);

  
  nsAutoCString mimeType(aMimeType);
  image = ImageFactory::CreateAnonymousImage(mimeType);

  if (image->HasError())
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIInputStream> inStream = aInStr;
  if (!NS_InputStreamIsBuffered(aInStr)) {
    nsCOMPtr<nsIInputStream> bufStream;
    rv = NS_NewBufferedInputStream(getter_AddRefs(bufStream), aInStr, 1024);
    if (NS_SUCCEEDED(rv))
      inStream = bufStream;
  }

  
  uint64_t length;
  rv = inStream->Available(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(length <= UINT32_MAX, NS_ERROR_FILE_TOO_BIG);

  
  rv = image->OnImageDataAvailable(nullptr, nullptr, inStream, 0, uint32_t(length));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = image->OnImageDataComplete(nullptr, nullptr, NS_OK, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(*aContainer = image.get());
  return NS_OK;
}


NS_IMETHODIMP imgTools::EncodeImage(imgIContainer *aContainer,
                                    const nsACString& aMimeType,
                                    const nsAString& aOutputOptions,
                                    nsIInputStream **aStream)
{
  nsresult rv;

  
  nsRefPtr<gfxImageSurface> frame;
  rv = GetFirstImageFrame(aContainer, getter_AddRefs(frame));
  NS_ENSURE_SUCCESS(rv, rv);

  return EncodeImageData(frame, aMimeType, aOutputOptions, aStream);
}

NS_IMETHODIMP imgTools::EncodeScaledImage(imgIContainer *aContainer,
                                          const nsACString& aMimeType,
                                          int32_t aScaledWidth,
                                          int32_t aScaledHeight,
                                          const nsAString& aOutputOptions,
                                          nsIInputStream **aStream)
{
  NS_ENSURE_ARG(aScaledWidth >= 0 && aScaledHeight >= 0);

  
  
  if (aScaledWidth == 0 && aScaledHeight == 0) {
    return EncodeImage(aContainer, aMimeType, aOutputOptions, aStream);
  }

  
  nsRefPtr<gfxImageSurface> frame;
  nsresult rv = GetFirstImageFrame(aContainer, getter_AddRefs(frame));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t frameWidth = frame->Width(), frameHeight = frame->Height();

  
  
  if (aScaledWidth == 0) {
    aScaledWidth = frameWidth;
  } else if (aScaledHeight == 0) {
    aScaledHeight = frameHeight;
  }

  
  nsRefPtr<gfxImageSurface> dest = new gfxImageSurface(gfxIntSize(aScaledWidth, aScaledHeight),
                                                       gfxASurface::ImageFormatARGB32);
  gfxContext ctx(dest);

  
  gfxFloat sw = (double) aScaledWidth / frameWidth;
  gfxFloat sh = (double) aScaledHeight / frameHeight;
  ctx.Scale(sw, sh);

  
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetSource(frame);
  ctx.Paint();

  return EncodeImageData(dest, aMimeType, aOutputOptions, aStream);
}

NS_IMETHODIMP imgTools::EncodeCroppedImage(imgIContainer *aContainer,
                                           const nsACString& aMimeType,
                                           int32_t aOffsetX,
                                           int32_t aOffsetY,
                                           int32_t aWidth,
                                           int32_t aHeight,
                                           const nsAString& aOutputOptions,
                                           nsIInputStream **aStream)
{
  NS_ENSURE_ARG(aOffsetX >= 0 && aOffsetY >= 0 && aWidth >= 0 && aHeight >= 0);

  
  
  NS_ENSURE_ARG(aWidth + aHeight > 0 || aOffsetX + aOffsetY == 0);

  
  
  if (aWidth == 0 && aHeight == 0) {
    return EncodeImage(aContainer, aMimeType, aOutputOptions, aStream);
  }

  
  nsRefPtr<gfxImageSurface> frame;
  nsresult rv = GetFirstImageFrame(aContainer, getter_AddRefs(frame));
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t frameWidth = frame->Width(), frameHeight = frame->Height();

  
  
  if (aWidth == 0) {
    aWidth = frameWidth;
  } else if (aHeight == 0) {
    aHeight = frameHeight;
  }

  
  NS_ENSURE_ARG(frameWidth >= aOffsetX + aWidth &&
                frameHeight >= aOffsetY + aHeight);

  
  nsRefPtr<gfxImageSurface> dest = new gfxImageSurface(gfxIntSize(aWidth, aHeight),
                                                       gfxASurface::ImageFormatARGB32);
  gfxContext ctx(dest);

  
  ctx.Translate(gfxPoint(-aOffsetX, -aOffsetY));

  
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetSource(frame);
  ctx.Paint();

  return EncodeImageData(dest, aMimeType, aOutputOptions, aStream);
}

NS_IMETHODIMP imgTools::EncodeImageData(gfxImageSurface *aSurface,
                                        const nsACString& aMimeType,
                                        const nsAString& aOutputOptions,
                                        nsIInputStream **aStream)
{
  uint8_t *bitmapData;
  uint32_t bitmapDataLength, strideSize;

  
  nsAutoCString encoderCID(
    NS_LITERAL_CSTRING("@mozilla.org/image/encoder;2?type=") + aMimeType);

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(encoderCID.get());
  if (!encoder)
    return NS_IMAGELIB_ERROR_NO_ENCODER;

  bitmapData = aSurface->Data();
  if (!bitmapData)
    return NS_ERROR_FAILURE;

  strideSize = aSurface->Stride();

  int32_t width = aSurface->Width(), height = aSurface->Height();
  bitmapDataLength = height * strideSize;

  
  nsresult rv = encoder->InitFromData(bitmapData,
                                      bitmapDataLength,
                                      width,
                                      height,
                                      strideSize,
                                      imgIEncoder::INPUT_FORMAT_HOSTARGB,
                                      aOutputOptions);

  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(encoder, aStream);
}

NS_IMETHODIMP imgTools::GetFirstImageFrame(imgIContainer *aContainer,
                                           gfxImageSurface **aSurface)
{
  nsRefPtr<gfxASurface> surface;
  aContainer->GetFrame(imgIContainer::FRAME_FIRST,
                       imgIContainer::FLAG_SYNC_DECODE,
                       getter_AddRefs(surface));
  NS_ENSURE_TRUE(surface, NS_ERROR_NOT_AVAILABLE);

  nsRefPtr<gfxImageSurface> frame(surface->CopyToARGB32ImageSurface());
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(frame->Width() && frame->Height(), NS_ERROR_FAILURE);

  frame.forget(aSurface);
  return NS_OK;
}

NS_IMETHODIMP imgTools::CreateScriptedObserver(imgIScriptedNotificationObserver* aInner,
                                               imgINotificationObserver** aObserver)
{
  NS_ADDREF(*aObserver = new ScriptedNotificationObserver(aInner));
  return NS_OK;
}

NS_IMETHODIMP
imgTools::GetImgLoaderForDocument(nsIDOMDocument* aDoc, imgILoader** aLoader)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_IF_ADDREF(*aLoader = nsContentUtils::GetImgLoaderForDocument(doc));
  return NS_OK;
}

NS_IMETHODIMP
imgTools::GetImgCacheForDocument(nsIDOMDocument* aDoc, imgICache** aCache)
{
  nsCOMPtr<imgILoader> loader;
  nsresult rv = GetImgLoaderForDocument(aDoc, getter_AddRefs(loader));
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(loader, aCache);
}
