




































#ifndef nsResizerFrame_h___
#define nsResizerFrame_h___

#include "nsTitleBarFrame.h"

class nsResizerFrame : public nsTitleBarFrame 
{
protected:
  struct Direction {
    PRInt8 mHorizontal;
    PRInt8 mVertical;
  };

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  nsResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus);

  virtual void MouseClicked(nsPresContext* aPresContext, nsGUIEvent *aEvent);

protected:
  Direction GetDirection();
  static void AdjustDimensions(PRInt32* aPos, PRInt32* aSize,
                        PRInt32 aMovement, PRInt8 aResizerDirection);

protected:
	nsIntRect mWidgetRect;
}; 

#endif 
