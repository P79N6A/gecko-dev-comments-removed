




#ifndef nsIScrollbarMediator_h___
#define nsIScrollbarMediator_h___

#include "nsQueryFrame.h"
#include "nsCoord.h"

class nsScrollbarFrame;
class nsIFrame;

class nsIScrollbarMediator : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIScrollbarMediator)

  





  





  enum ScrollSnapMode { DISABLE_SNAP, ENABLE_SNAP };

  




  virtual void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                            ScrollSnapMode aSnap = DISABLE_SNAP) = 0;
  virtual void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                            ScrollSnapMode aSnap = DISABLE_SNAP) = 0;
  virtual void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                            ScrollSnapMode aSnap = DISABLE_SNAP) = 0;
  





  virtual void RepeatButtonScroll(nsScrollbarFrame* aScrollbar) = 0;
  






  virtual void ThumbMoved(nsScrollbarFrame* aScrollbar,
                          nscoord aOldPos,
                          nscoord aNewPos) = 0;
  



  virtual void ScrollbarReleased(nsScrollbarFrame* aScrollbar) = 0;
  virtual void VisibilityChanged(bool aVisible) = 0;

  



  virtual nsIFrame* GetScrollbarBox(bool aVertical) = 0;
  



  virtual void ScrollbarActivityStarted() const = 0;
  virtual void ScrollbarActivityStopped() const = 0;
};

#endif

