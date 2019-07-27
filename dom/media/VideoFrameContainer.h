





#ifndef VIDEOFRAMECONTAINER_H_
#define VIDEOFRAMECONTAINER_H_

#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
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
  B2G_ACL_EXPORT ~VideoFrameContainer();

public:
  typedef layers::ImageContainer ImageContainer;
  typedef layers::Image Image;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VideoFrameContainer)

  VideoFrameContainer(dom::HTMLMediaElement* aElement,
                      already_AddRefed<ImageContainer> aContainer);

  
  B2G_ACL_EXPORT void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage,
                       TimeStamp aTargetTime);
  void ClearCurrentFrame();
  
  
  
  double GetFrameDelay();
  
  enum {
    INVALIDATE_DEFAULT,
    INVALIDATE_FORCE
  };
  void Invalidate() { InvalidateWithFlags(INVALIDATE_DEFAULT); }
  B2G_ACL_EXPORT void InvalidateWithFlags(uint32_t aFlags);
  B2G_ACL_EXPORT ImageContainer* GetImageContainer();
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
};

}

#endif 
