




#include "nsMimeTypes.h"

#include "Image.h"
#include "nsRefreshDriver.h"
#include "mozilla/TimeStamp.h"

namespace mozilla {
namespace image {


ImageResource::ImageResource(ImageURL* aURI) :
  mURI(aURI),
  mInnerWindowId(0),
  mAnimationConsumers(0),
  mAnimationMode(kNormalAnimMode),
  mInitialized(false),
  mAnimating(false),
  mError(false)
{ }


Image::eDecoderType
Image::GetDecoderType(const char* aMimeType)
{
  
  eDecoderType rv = eDecoderType_unknown;

  
  if (!strcmp(aMimeType, IMAGE_PNG)) {
    rv = eDecoderType_png;

  } else if (!strcmp(aMimeType, IMAGE_X_PNG)) {
    rv = eDecoderType_png;

  
  } else if (!strcmp(aMimeType, IMAGE_GIF)) {
    rv = eDecoderType_gif;

  
  } else if (!strcmp(aMimeType, IMAGE_JPEG)) {
    rv = eDecoderType_jpeg;
  } else if (!strcmp(aMimeType, IMAGE_PJPEG)) {
    rv = eDecoderType_jpeg;
  } else if (!strcmp(aMimeType, IMAGE_JPG)) {
    rv = eDecoderType_jpeg;

  
  } else if (!strcmp(aMimeType, IMAGE_BMP)) {
    rv = eDecoderType_bmp;
  } else if (!strcmp(aMimeType, IMAGE_BMP_MS)) {
    rv = eDecoderType_bmp;

  
  } else if (!strcmp(aMimeType, IMAGE_ICO)) {
    rv = eDecoderType_ico;
  } else if (!strcmp(aMimeType, IMAGE_ICO_MS)) {
    rv = eDecoderType_ico;

  
  } else if (!strcmp(aMimeType, IMAGE_ICON_MS)) {
    rv = eDecoderType_icon;
  }

  return rv;
}

void
ImageResource::IncrementAnimationConsumers()
{
  MOZ_ASSERT(NS_IsMainThread(), "Main thread only to encourage serialization "
                                "with DecrementAnimationConsumers");
  mAnimationConsumers++;
}

void
ImageResource::DecrementAnimationConsumers()
{
  MOZ_ASSERT(NS_IsMainThread(), "Main thread only to encourage serialization "
                                "with IncrementAnimationConsumers");
  MOZ_ASSERT(mAnimationConsumers >= 1,
             "Invalid no. of animation consumers!");
  mAnimationConsumers--;
}

nsresult
ImageResource::GetAnimationModeInternal(uint16_t* aAnimationMode)
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_ARG_POINTER(aAnimationMode);

  *aAnimationMode = mAnimationMode;
  return NS_OK;
}

nsresult
ImageResource::SetAnimationModeInternal(uint16_t aAnimationMode)
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(aAnimationMode == kNormalAnimMode ||
               aAnimationMode == kDontAnimMode ||
               aAnimationMode == kLoopOnceAnimMode,
               "Wrong Animation Mode is being set!");

  mAnimationMode = aAnimationMode;

  return NS_OK;
}

bool
ImageResource::HadRecentRefresh(const TimeStamp& aTime)
{
  
  
  
  static TimeDuration recentThreshold =
      TimeDuration::FromMilliseconds(nsRefreshDriver::DefaultInterval() / 2.0);

  if (!mLastRefreshTime.IsNull() &&
      aTime - mLastRefreshTime < recentThreshold) {
    return true;
  }

  
  
  mLastRefreshTime = aTime;
  return false;
}

void
ImageResource::EvaluateAnimation()
{
  if (!mAnimating && ShouldAnimate()) {
    nsresult rv = StartAnimation();
    mAnimating = NS_SUCCEEDED(rv);
  } else if (mAnimating && !ShouldAnimate()) {
    StopAnimation();
  }
}

} 
} 
