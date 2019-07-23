









































#ifndef nsIScrollFrame_h___
#define nsIScrollFrame_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsIViewManager.h"
#include "nsIScrollableViewProvider.h"
#include "nsPresContext.h"
#include "nsIFrame.h" 

class nsBoxLayoutState;


#define NS_ISCROLLABLE_FRAME_IID    \
{ 0xf285c180, 0x8492, 0x48d5, \
{ 0xb1, 0xb5, 0x03, 0x28, 0x21, 0xc9, 0x72, 0x02 } }

class nsIScrollableFrame : public nsIScrollableViewProvider {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLABLE_FRAME_IID)

  



  virtual nsIFrame* GetScrolledFrame() const = 0;

  typedef nsPresContext::ScrollbarStyles ScrollbarStyles;

  virtual ScrollbarStyles GetScrollbarStyles() const = 0;

  





  virtual nsMargin GetActualScrollbarSizes() const = 0;

  



  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) = 0;

  


  virtual nsPoint GetScrollPosition() const = 0;

  








  virtual void ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags = NS_VMREFRESH_NO_SYNC)=0;

  virtual nsIScrollableView* GetScrollableView() = 0;

  



  virtual void SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible) = 0;

  virtual nsIBox* GetScrollbarBox(PRBool aVertical) = 0;

  virtual void CurPosAttributeChanged(nsIContent* aChild, PRInt32 aModType) = 0;

  






  virtual void ScrollToRestoredPosition() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollableFrame, NS_ISCROLLABLE_FRAME_IID)

#endif
