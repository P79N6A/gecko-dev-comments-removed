





#include <algorithm>

#include "mozilla/Likely.h"

#include "nsIHttpChannel.h"
#include "nsIFileChannel.h"
#include "nsIFile.h"
#include "nsMimeTypes.h"
#include "nsIRequest.h"

#include "MultipartImage.h"
#include "RasterImage.h"
#include "VectorImage.h"
#include "Image.h"
#include "nsMediaFragmentURIParser.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"

#include "ImageFactory.h"
#include "gfxPrefs.h"

namespace mozilla {
namespace image {

 void
ImageFactory::Initialize()
{ }

static bool
ShouldDownscaleDuringDecode(const nsCString& aMimeType)
{
  return aMimeType.EqualsLiteral(IMAGE_JPEG) ||
         aMimeType.EqualsLiteral(IMAGE_JPG) ||
         aMimeType.EqualsLiteral(IMAGE_PJPEG);
}

static uint32_t
ComputeImageFlags(ImageURL* uri, const nsCString& aMimeType, bool isMultiPart)
{
  nsresult rv;

  
  bool isDiscardable = gfxPrefs::ImageMemDiscardable();
  bool doDecodeOnlyOnDraw = gfxPrefs::ImageDecodeOnlyOnDrawEnabled() &&
                            gfxPrefs::AsyncPanZoomEnabled();
  bool doDecodeImmediately = gfxPrefs::ImageDecodeImmediatelyEnabled();
  bool doDownscaleDuringDecode = gfxPrefs::ImageDownscaleDuringDecodeEnabled();

  
  
  bool isChrome = false;
  rv = uri->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && isChrome) {
    isDiscardable = doDecodeOnlyOnDraw = false;
  }

  
  
  bool isResource = false;
  rv = uri->SchemeIs("resource", &isResource);
  if (NS_SUCCEEDED(rv) && isResource) {
    isDiscardable = doDecodeOnlyOnDraw = false;
  }

  
  
  if ((doDownscaleDuringDecode || doDecodeOnlyOnDraw) &&
      !ShouldDownscaleDuringDecode(aMimeType)) {
    doDownscaleDuringDecode = false;
    doDecodeOnlyOnDraw = false;
  }

  
  if (doDecodeImmediately) {
    doDecodeOnlyOnDraw = false;
  }

  
  
  if (isMultiPart) {
    isDiscardable = doDecodeOnlyOnDraw = doDownscaleDuringDecode = false;
  }

  
  uint32_t imageFlags = Image::INIT_FLAG_NONE;
  if (isDiscardable) {
    imageFlags |= Image::INIT_FLAG_DISCARDABLE;
  }
  if (doDecodeOnlyOnDraw) {
    imageFlags |= Image::INIT_FLAG_DECODE_ONLY_ON_DRAW;
  }
  if (doDecodeImmediately) {
    imageFlags |= Image::INIT_FLAG_DECODE_IMMEDIATELY;
  }
  if (isMultiPart) {
    imageFlags |= Image::INIT_FLAG_TRANSIENT;
  }
  if (doDownscaleDuringDecode) {
    imageFlags |= Image::INIT_FLAG_DOWNSCALE_DURING_DECODE;
  }

  return imageFlags;
}

 already_AddRefed<Image>
ImageFactory::CreateImage(nsIRequest* aRequest,
                          ProgressTracker* aProgressTracker,
                          const nsCString& aMimeType,
                          ImageURL* aURI,
                          bool aIsMultiPart,
                          uint32_t aInnerWindowId)
{
  MOZ_ASSERT(gfxPrefs::SingletonExists(),
             "Pref observers should have been initialized already");

  
  uint32_t imageFlags = ComputeImageFlags(aURI, aMimeType, aIsMultiPart);

  
  if (aMimeType.EqualsLiteral(IMAGE_SVG_XML)) {
    return CreateVectorImage(aRequest, aProgressTracker, aMimeType,
                             aURI, imageFlags, aInnerWindowId);
  } else {
    return CreateRasterImage(aRequest, aProgressTracker, aMimeType,
                             aURI, imageFlags, aInnerWindowId);
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
ImageFactory::CreateAnonymousImage(const nsCString& aMimeType)
{
  nsresult rv;

  nsRefPtr<RasterImage> newImage = new RasterImage();

  nsRefPtr<ProgressTracker> newTracker = new ProgressTracker();
  newTracker->SetImage(newImage);
  newImage->SetProgressTracker(newTracker);

  rv = newImage->Init(aMimeType.get(), Image::INIT_FLAG_NONE);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  return newImage.forget();
}

 already_AddRefed<MultipartImage>
ImageFactory::CreateMultipartImage(Image* aFirstPart,
                                   ProgressTracker* aProgressTracker)
{
  MOZ_ASSERT(aFirstPart);
  MOZ_ASSERT(aProgressTracker);

  nsRefPtr<MultipartImage> newImage = new MultipartImage(aFirstPart);
  aProgressTracker->SetImage(newImage);
  newImage->SetProgressTracker(aProgressTracker);

  newImage->Init();

  return newImage.forget();
}

int32_t
SaturateToInt32(int64_t val)
{
  if (val > INT_MAX) {
    return INT_MAX;
  }
  if (val < INT_MIN) {
    return INT_MIN;
  }

  return static_cast<int32_t>(val);
}

uint32_t
GetContentSize(nsIRequest* aRequest)
{
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    int64_t size;
    nsresult rv = channel->GetContentLength(&size);
    if (NS_SUCCEEDED(rv)) {
      return std::max(SaturateToInt32(size), 0);
    }
  }

  
  nsCOMPtr<nsIFileChannel> fileChannel(do_QueryInterface(aRequest));
  if (fileChannel) {
    nsCOMPtr<nsIFile> file;
    nsresult rv = fileChannel->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
      int64_t filesize;
      rv = file->GetFileSize(&filesize);
      if (NS_SUCCEEDED(rv)) {
        return std::max(SaturateToInt32(filesize), 0);
      }
    }
  }

  
  return 0;
}

 already_AddRefed<Image>
ImageFactory::CreateRasterImage(nsIRequest* aRequest,
                                ProgressTracker* aProgressTracker,
                                const nsCString& aMimeType,
                                ImageURL* aURI,
                                uint32_t aImageFlags,
                                uint32_t aInnerWindowId)
{
  MOZ_ASSERT(aProgressTracker);

  nsresult rv;

  nsRefPtr<RasterImage> newImage = new RasterImage(aURI);
  aProgressTracker->SetImage(newImage);
  newImage->SetProgressTracker(aProgressTracker);

  rv = newImage->Init(aMimeType.get(), aImageFlags);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  newImage->SetInnerWindowID(aInnerWindowId);

  uint32_t len = GetContentSize(aRequest);

  
  
  if (len > 0) {
    
    uint32_t sizeHint = std::min<uint32_t>(len, 20000000);
    rv = newImage->SetSourceSizeHint(sizeHint);
    if (NS_FAILED(rv)) {
      
      rv = nsMemory::HeapMinimize(true);
      nsresult rv2 = newImage->SetSourceSizeHint(sizeHint);
      
      if (NS_FAILED(rv) || NS_FAILED(rv2)) {
        NS_WARNING("About to hit OOM in imagelib!");
      }
    }
  }

  nsAutoCString ref;
  aURI->GetRef(ref);
  net::nsMediaFragmentURIParser parser(ref);
  if (parser.HasResolution()) {
    newImage->SetRequestedResolution(parser.GetResolution());
  }

  if (parser.HasSampleSize()) {
      
      nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
      nsCOMPtr<nsIPrincipal> principal;
      if (chan) {
        nsContentUtils::GetSecurityManager()
          ->GetChannelResultPrincipal(chan, getter_AddRefs(principal));
      }

      if ((principal &&
           principal->GetAppStatus() == nsIPrincipal::APP_STATUS_CERTIFIED) ||
          gfxPrefs::ImageMozSampleSizeEnabled()) {
        newImage->SetRequestedSampleSize(parser.GetSampleSize());
      }
  }

  return newImage.forget();
}

 already_AddRefed<Image>
ImageFactory::CreateVectorImage(nsIRequest* aRequest,
                                ProgressTracker* aProgressTracker,
                                const nsCString& aMimeType,
                                ImageURL* aURI,
                                uint32_t aImageFlags,
                                uint32_t aInnerWindowId)
{
  MOZ_ASSERT(aProgressTracker);

  nsresult rv;

  nsRefPtr<VectorImage> newImage = new VectorImage(aURI);
  aProgressTracker->SetImage(newImage);
  newImage->SetProgressTracker(aProgressTracker);

  rv = newImage->Init(aMimeType.get(), aImageFlags);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  newImage->SetInnerWindowID(aInnerWindowId);

  rv = newImage->OnStartRequest(aRequest, nullptr);
  NS_ENSURE_SUCCESS(rv, BadImage(newImage));

  return newImage.forget();
}

} 
} 
