









































#ifndef nsIScrollFrame_h___
#define nsIScrollFrame_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsIViewManager.h"
#include "nsIScrollableViewProvider.h"
#include "nsPresContext.h"
#include "nsIFrame.h" 

class nsBoxLayoutState;

class nsIScrollableFrame : public nsIScrollableViewProvider {
public:

  NS_DECLARE_FRAME_ACCESSOR(nsIScrollableFrame)

  



  virtual nsIFrame* GetScrolledFrame() const = 0;

  typedef nsPresContext::ScrollbarStyles ScrollbarStyles;

  virtual ScrollbarStyles GetScrollbarStyles() const = 0;

  





  virtual nsMargin GetActualScrollbarSizes() const = 0;

  



  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) = 0;
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
                                            nsIRenderingContext* aRC) = 0;
  
  


  virtual nsPoint GetScrollPosition() const = 0;

  








  virtual void ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags = 0)=0;

  virtual nsIScrollableView* GetScrollableView() = 0;

  



  virtual void SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible) = 0;

  virtual nsIBox* GetScrollbarBox(PRBool aVertical) = 0;

  virtual void CurPosAttributeChanged(nsIContent* aChild, PRInt32 aModType) = 0;

  






  virtual void ScrollToRestoredPosition() = 0;
};

#endif
