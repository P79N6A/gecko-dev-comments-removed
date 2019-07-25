





#ifndef VIDEOFRAMECONTAINER_H_
#define VIDEOFRAMECONTAINER_H_

#include "ImageContainer.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "gfxPoint.h"

class nsHTMLMediaElement;

namespace mozilla {










class VideoFrameContainer {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::Image Image;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoFrameContainer)

  VideoFrameContainer(nsHTMLMediaElement* aElement,
                      already_AddRefed<ImageContainer> aContainer)
    : mElement(aElement),
      mImageContainer(aContainer), mMutex("nsVideoFrameContainer"),
      mIntrinsicSizeChanged(false), mImageSizeChanged(false),
      mNeedInvalidation(true)
  {
    NS_ASSERTION(aElement, "aElement must not be null");
    NS_ASSERTION(mImageContainer, "aContainer must not be null");
  }

  
  void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage,
                       TimeStamp aTargetTime);
  
  
  
  double GetFrameDelay();
  
  void Invalidate();
  ImageContainer* GetImageContainer() { return mImageContainer; }
  void ForgetElement() { mElement = nullptr; }

protected:
  
  
  nsHTMLMediaElement* mElement;
  nsRefPtr<ImageContainer> mImageContainer;

  
  Mutex mMutex;
  
  
  
  
  
  gfxIntSize mIntrinsicSize;
  
  
  TimeStamp mPaintTarget;
  
  
  
  TimeDuration mPaintDelay;
  
  
  
  
  
  bool mIntrinsicSizeChanged;
  
  
  
  
  bool mImageSizeChanged;

  bool mNeedInvalidation;
};

}

#endif 
