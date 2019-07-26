





#ifndef VIDEOFRAMECONTAINER_H_
#define VIDEOFRAMECONTAINER_H_

#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "gfxPoint.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

namespace mozilla {

namespace dom {
class HTMLMediaElement;
}

namespace layers {
class Image;
class ImageContainer;
}










class VideoFrameContainer {
public:
  typedef layers::ImageContainer ImageContainer;
  typedef layers::Image Image;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoFrameContainer)

  VideoFrameContainer(dom::HTMLMediaElement* aElement,
                      already_AddRefed<ImageContainer> aContainer);
  ~VideoFrameContainer();

  
  void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage,
                       TimeStamp aTargetTime);
  void ClearCurrentFrame(bool aResetSize = false);
  
  void Reset();
  
  
  
  double GetFrameDelay();
  
  void Invalidate();
  ImageContainer* GetImageContainer();
  void ForgetElement() { mElement = nullptr; }

protected:
  
  
  dom::HTMLMediaElement* mElement;
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
