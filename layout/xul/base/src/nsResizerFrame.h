




































#ifndef nsResizerFrame_h___
#define nsResizerFrame_h___

#include "nsTitleBarFrame.h"

class nsResizerFrame : public nsTitleBarFrame 
{

protected:
  enum eDirection {
    topleft,
    top,
	 topright,
	 left,	 
	 right,
	 bottomleft,
	 bottom,
	 bottomright
  };
  

public:
  friend nsIFrame* NS_NewResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  nsResizerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus);

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);
  
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual void MouseClicked(nsPresContext* aPresContext, nsGUIEvent *aEvent);

protected:
	PRBool GetInitialDirection(eDirection& aDirection);
	PRBool EvalDirection(nsAutoString& aText,eDirection& aResult);

protected:
	eDirection mDirection;
	nsRect mWidgetRect;
}; 

#endif 
