








































#ifndef nsSplitterFrame_h__
#define nsSplitterFrame_h__


#include "nsBoxFrame.h"

class nsSplitterFrameInner;

nsIFrame* NS_NewSplitterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsSplitterFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsSplitterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  virtual void Destroy();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("SplitterFrame"), aResult);
  }
#endif

  
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD GetCursor(const nsPoint&    aPoint,
                       nsIFrame::Cursor& aCursor);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 nsGUIEvent *    aEvent,
                                 nsEventStatus*  aEventStatus,
                                 PRBool aControlHeld);

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual void GetInitialOrientation(PRBool& aIsHorizontal); 

  virtual nsIView* GetMouseCapturer() const { return GetView(); }

private:

  friend class nsSplitterFrameInner;
  nsSplitterFrameInner* mInner;

}; 

#endif
