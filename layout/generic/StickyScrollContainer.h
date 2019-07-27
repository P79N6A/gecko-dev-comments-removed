










#ifndef StickyScrollContainer_h
#define StickyScrollContainer_h

#include "nsPoint.h"
#include "nsTArray.h"
#include "nsIScrollPositionListener.h"

struct nsRect;
class nsIFrame;
class nsIScrollableFrame;

namespace mozilla {

class StickyScrollContainer MOZ_FINAL : public nsIScrollPositionListener
{
public:
  



  static StickyScrollContainer* GetStickyScrollContainerForFrame(nsIFrame* aFrame);

  



  static StickyScrollContainer* GetStickyScrollContainerForScrollFrame(nsIFrame* aScrollFrame);

  


  static void NotifyReparentedFrameAcrossScrollFrameBoundary(nsIFrame* aFrame,
                                                             nsIFrame* aOldParent);

  void AddFrame(nsIFrame* aFrame) {
    mFrames.AppendElement(aFrame);
  }
  void RemoveFrame(nsIFrame* aFrame) {
    mFrames.RemoveElement(aFrame);
  }

  nsIScrollableFrame* ScrollFrame() const {
    return mScrollFrame;
  }

  
  static void ComputeStickyOffsets(nsIFrame* aFrame);

  



  nsPoint ComputePosition(nsIFrame* aFrame) const;

  



  void GetScrollRanges(nsIFrame* aFrame, nsRect* aOuter, nsRect* aInner) const;

  


  void PositionContinuations(nsIFrame* aFrame);

  





  void UpdatePositions(nsPoint aScrollPosition, nsIFrame* aSubtreeRoot);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) MOZ_OVERRIDE;
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY) MOZ_OVERRIDE;

private:
  explicit StickyScrollContainer(nsIScrollableFrame* aScrollFrame);
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
