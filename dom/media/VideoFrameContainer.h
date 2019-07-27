





#ifndef VIDEOFRAMECONTAINER_H_
#define VIDEOFRAMECONTAINER_H_

#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "gfxPoint.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "ImageContainer.h"

namespace mozilla {

namespace dom {
class HTMLMediaElement;
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
                       const TimeStamp& aTargetTime);
  void SetCurrentFrames(const gfxIntSize& aIntrinsicSize,
                        const nsTArray<ImageContainer::NonOwningImage>& aImages);
  void ClearCurrentFrame(const gfxIntSize& aIntrinsicSize)
  {
    SetCurrentFrames(aIntrinsicSize, nsTArray<ImageContainer::NonOwningImage>());
  }

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
  void SetCurrentFramesLocked(const gfxIntSize& aIntrinsicSize,
                              const nsTArray<ImageContainer::NonOwningImage>& aImages);

  
  
  dom::HTMLMediaElement* mElement;
  nsRefPtr<ImageContainer> mImageContainer;

  
  Mutex mMutex;
  
  
  
  
  
  gfxIntSize mIntrinsicSize;
  
  
  ImageContainer::FrameID mFrameID;
  
  
  
  
  
  bool mIntrinsicSizeChanged;
  
  
  
  
  bool mImageSizeChanged;
};

} 

#endif 
