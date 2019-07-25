








































#ifndef nsIScrollFrame_h___
#define nsIScrollFrame_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsPresContext.h"
#include "nsIFrame.h" 

class nsBoxLayoutState;
class nsIScrollPositionListener;






class nsIScrollableFrame : public nsQueryFrame {
public:

  NS_DECL_QUERYFRAME_TARGET(nsIScrollableFrame)

  



  virtual nsIFrame* GetScrolledFrame() const = 0;

  typedef nsPresContext::ScrollbarStyles ScrollbarStyles;
  




  virtual ScrollbarStyles GetScrollbarStyles() const = 0;

  enum { HORIZONTAL = 0x01, VERTICAL = 0x02 };
  




  virtual PRUint32 GetScrollbarVisibility() const = 0;
  





  virtual nsMargin GetActualScrollbarSizes() const = 0;
  




  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) = 0;
  




  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
                                            nsRenderingContext* aRC) = 0;

  




  virtual nsRect GetScrollPortRect() const = 0;
  




  virtual nsPoint GetScrollPosition() const = 0;
  







  virtual nsRect GetScrollRange() const = 0;

  



  virtual nsSize GetLineScrollAmount() const = 0;
  



  virtual nsSize GetPageScrollAmount() const = 0;

  







  enum ScrollMode { INSTANT, SMOOTH, NORMAL };
  



  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode) = 0;
  


  enum ScrollUnit { DEVICE_PIXELS, LINES, PAGES, WHOLE };
  








  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow = nsnull) = 0;
  







  virtual void ScrollToRestoredPosition() = 0;

  



  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
  


  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) = 0;

  





  virtual nsIBox* GetScrollbarBox(PRBool aVertical) = 0;

  



  virtual void CurPosAttributeChanged(nsIContent* aChild) = 0;

  



  NS_IMETHOD PostScrolledAreaEventForCurrentArea() = 0;

  




  virtual PRBool IsScrollingActive() = 0;
};

#endif
