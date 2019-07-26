





#include "mozilla/Preferences.h"
#include "mozilla/Likely.h"

#include "nsIHttpChannel.h"

#include "RasterImage.h"
#include "VectorImage.h"

#include "ImageFactory.h"

namespace mozilla {
namespace image {

const char* SVG_MIMETYPE = "image/svg+xml";


static bool gInitializedPrefCaches = false;
static bool gDecodeOnDraw = false;
static bool gDiscardable = false;

static void
InitPrefCaches()
{
  Preferences::AddBoolVarCache(&gDiscardable, "image.mem.discardable");
  Preferences::AddBoolVarCache(&gDecodeOnDraw, "image.mem.decodeondraw");
  gInitializedPrefCaches = true;
}

static uint32_t
ComputeImageFlags(nsIURI* uri, bool isMultiPart)
{
  nsresult rv;

  
  bool isDiscardable = gDiscardable;
  bool doDecodeOnDraw = gDecodeOnDraw;

  
  
  bool isChrome = false;
  rv = uri->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && isChrome)
    isDiscardable = doDecodeOnDraw = false;

  
  
  bool isResource = false;
  rv = uri->SchemeIs("resource", &isResource);
  if (NS_SUCCEEDED(rv) && isResource)
    isDiscardable = doDecodeOnDraw = false;

  
  
  if (isMultiPart)
    isDiscardable = doDecodeOnDraw = false;

  
  uint32_t imageFlags = Image::INIT_FLAG_NONE;
  if (isDiscardable)
    imageFlags |= Image::INIT_FLAG_DISCARDABLE;
  if (doDecodeOnDraw)
    imageFlags |= Image::INIT_FLAG_DECODE_ON_DRAW;
  if (isMultiPart)
    imageFlags |= Image::INIT_FLAG_MULTIPART;

  return imageFlags;
}

 already_AddRefed<Image>
ImageFactory::CreateImage(nsIRequest* aRequest,
                          imgStatusTracker* aStatusTracker,
                          const nsCString& aMimeType,
                          nsIURI* aURI,
                          bool aIsMultiPart,
                          uint32_t aInnerWindowId)
{
  nsresult rv;

  
  if (MOZ_UNLIKELY(!gInitializedPrefCaches))
    InitPrefCaches();

  
  nsAutoCString uriString;
  rv = aURI ? aURI->GetSpec(uriString) : NS_ERROR_FAILURE;
  if (NS_FAILED(rv))
    uriString.Assign("<unknown image URI>");

  
  uint32_t imageFlags = ComputeImageFlags(aURI, aIsMultiPart);

  
  if (aMimeType.Equals(SVG_MIMETYPE)) {
    return CreateVectorImage(aRequest, aStatusTracker, aMimeType,
                             uriString, imageFlags, aInnerWindowId);
  } else {
    return CreateRasterImage(aRequest, aStatusTracker, aMimeType,
                             uriString, imageFlags, aInnerWindowId);
  }
}




template <typename T>
static already_AddRefed<Image>
BadImage(nsRefPtr<T>& image)
{
  image->SetHasError();
  return image.forget();
}

 already_AddRefed<Image>
ImageFactory::CreateRasterImage(nsIRequest* aRequest,
                                imgStatusTracker* aStatusTracker,
                                const nsCString& aMimeType,
                                const nsCString& aURIString,
                                uint32_t aImageFlags,
                                uint32_t aInnerWindowId)
{
  nsresult rv;

  nsRefPtr<RasterImage> newImage = new RasterImage(aStatusTracker);

  rv = newImage->Init(aStatusTracker->GetDecoderObserver(),
                      aMimeType.get(), aURIString.get(), aImageFlags);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  newImage->SetInnerWindowID(aInnerWindowId);

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
  if (httpChannel) {
    nsAutoCString contentLength;
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-length"),
                                        contentLength);
    if (NS_SUCCEEDED(rv)) {
      int32_t len = contentLength.ToInteger(&rv);

      
      
      if (len > 0) {
        uint32_t sizeHint = (uint32_t) len;
        sizeHint = NS_MIN<uint32_t>(sizeHint, 20000000); 
        rv = newImage->SetSourceSizeHint(sizeHint);
        if (NS_FAILED(rv)) {
          
          rv = nsMemory::HeapMinimize(true);
          nsresult rv2 = newImage->SetSourceSizeHint(sizeHint);
          
          if (NS_FAILED(rv) || NS_FAILED(rv2)) {
            NS_WARNING("About to hit OOM in imagelib!");
          }
        }
      }
    }
  }

  return newImage.forget();
}

 already_AddRefed<Image>
ImageFactory::CreateVectorImage(nsIRequest* aRequest,
                                imgStatusTracker* aStatusTracker,
                                const nsCString& aMimeType,
                                const nsCString& aURIString,
                                uint32_t aImageFlags,
                                uint32_t aInnerWindowId)
{
  nsresult rv;

  nsRefPtr<VectorImage> newImage = new VectorImage(aStatusTracker);

  rv = newImage->Init(aStatusTracker->GetDecoderObserver(),
                      aMimeType.get(), aURIString.get(), aImageFlags);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  newImage->SetInnerWindowID(aInnerWindowId);

  rv = newImage->OnStartRequest(aRequest, nullptr);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  return newImage.forget();
}

} 
} 
