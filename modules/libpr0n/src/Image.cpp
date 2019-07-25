




































#include "Image.h"

namespace mozilla {
namespace imagelib {


Image::Image(imgStatusTracker* aStatusTracker) :
  mInnerWindowId(0),
  mAnimationConsumers(0),
  mAnimationMode(kNormalAnimMode),
  mInitialized(false),
  mAnimating(false),
  mError(false)
{
  if (aStatusTracker) {
    mStatusTracker = aStatusTracker;
    mStatusTracker->SetImage(this);
  } else {
    mStatusTracker = new imgStatusTracker(this);
  }
}

PRUint32
Image::GetDataSize()
{
  if (mError)
    return 0;
  
  return GetSourceHeapSize() + GetDecodedHeapSize() +
         GetDecodedNonheapSize() + GetDecodedOutOfProcessSize();
}


Image::eDecoderType
Image::GetDecoderType(const char *aMimeType)
{
  
  eDecoderType rv = eDecoderType_unknown;

  
  if (!strcmp(aMimeType, "image/png"))
    rv = eDecoderType_png;
  else if (!strcmp(aMimeType, "image/x-png"))
    rv = eDecoderType_png;

  
  else if (!strcmp(aMimeType, "image/gif"))
    rv = eDecoderType_gif;


  
  else if (!strcmp(aMimeType, "image/jpeg"))
    rv = eDecoderType_jpeg;
  else if (!strcmp(aMimeType, "image/pjpeg"))
    rv = eDecoderType_jpeg;
  else if (!strcmp(aMimeType, "image/jpg"))
    rv = eDecoderType_jpeg;

  
  else if (!strcmp(aMimeType, "image/bmp"))
    rv = eDecoderType_bmp;
  else if (!strcmp(aMimeType, "image/x-ms-bmp"))
    rv = eDecoderType_bmp;


  
  else if (!strcmp(aMimeType, "image/x-icon"))
    rv = eDecoderType_ico;
  else if (!strcmp(aMimeType, "image/vnd.microsoft.icon"))
    rv = eDecoderType_ico;

  
  else if (!strcmp(aMimeType, "image/icon"))
    rv = eDecoderType_icon;

  return rv;
}

void
Image::IncrementAnimationConsumers()
{
  mAnimationConsumers++;
  EvaluateAnimation();
}

void
Image::DecrementAnimationConsumers()
{
  NS_ABORT_IF_FALSE(mAnimationConsumers >= 1, "Invalid no. of animation consumers!");
  mAnimationConsumers--;
  EvaluateAnimation();
}



NS_IMETHODIMP
Image::GetAnimationMode(PRUint16* aAnimationMode)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimationMode);
  
  *aAnimationMode = mAnimationMode;
  return NS_OK;
}



NS_IMETHODIMP
Image::SetAnimationMode(PRUint16 aAnimationMode)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ASSERTION(aAnimationMode == kNormalAnimMode ||
               aAnimationMode == kDontAnimMode ||
               aAnimationMode == kLoopOnceAnimMode,
               "Wrong Animation Mode is being set!");
  
  mAnimationMode = aAnimationMode;

  EvaluateAnimation();

  return NS_OK;
}

void
Image::EvaluateAnimation()
{
  if (!mAnimating && ShouldAnimate()) {
    nsresult rv = StartAnimation();
    mAnimating = NS_SUCCEEDED(rv);
  } else if (mAnimating && !ShouldAnimate()) {
    StopAnimation();
    mAnimating = false;
  }
}

} 
} 
