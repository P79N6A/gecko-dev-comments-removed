










#ifndef StickyScrollContainer_h
#define StickyScrollContainer_h

#include "nsPoint.h"
#include "nsTArray.h"
#include "nsIScrollPositionListener.h"

class nsRect;
class nsIFrame;
class nsIScrollableFrame;

namespace mozilla {

class StickyScrollContainer MOZ_FINAL : public nsIScrollPositionListener
{
public:
  



  static StickyScrollContainer* GetStickyScrollContainerForFrame(nsIFrame* aFrame);

  



  static StickyScrollContainer* GetStickyScrollContainerForScrollFrame(nsIFrame* aScrollFrame);

  void AddFrame(nsIFrame* aFrame) {
    mFrames.AppendElement(aFrame);
  }
  void RemoveFrame(nsIFrame* aFrame) {
    mFrames.RemoveElement(aFrame);
  }

  
  static void ComputeStickyOffsets(nsIFrame* aFrame);

  



  nsPoint ComputePosition(nsIFrame* aFrame) const;

  





  void UpdatePositions(nsPoint aScrollPosition, nsIFrame* aSubtreeRoot);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) MOZ_OVERRIDE;
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY) MOZ_OVERRIDE;

private:
  StickyScrollContainer(nsIScrollableFrame* aScrollFrame);
  ~StickyScrollContainer();

  





  void ComputeStickyLimits(nsIFrame* aFrame, nsRect* aStick,
                           nsRect* aContain) const;

  friend void DestroyStickyScrollContainer(void* aPropertyValue);

  nsIScrollableFrame* const mScrollFrame;
  nsTArray<nsIFrame*> mFrames;
  nsPoint mScrollPosition;
};

} 

#endif 
