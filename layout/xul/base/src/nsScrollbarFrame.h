








































#ifndef nsScrollbarFrame_h__
#define nsScrollbarFrame_h__

#include "nsBoxFrame.h"

class nsIScrollbarMediator;

nsIFrame* NS_NewScrollbarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsScrollbarFrame : public nsBoxFrame
{
public:
    nsScrollbarFrame(nsIPresShell* aShell, nsStyleContext* aContext):
      nsBoxFrame(aShell, aContext), mScrollbarMediator(nsnull) {}

  NS_DECL_QUERYFRAME_TARGET(nsScrollbarFrame)

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("ScrollbarFrame"), aResult);
  }
#endif

  
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 nsGUIEvent *    aEvent,
                                 nsEventStatus*  aEventStatus,
                                 bool aControlHeld);

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual nsIAtom* GetType() const;  

  void SetScrollbarMediatorContent(nsIContent* aMediator);
  nsIScrollbarMediator* GetScrollbarMediator();

  

  






  virtual bool DoesClipChildren() { return true; }

private:
  nsCOMPtr<nsIContent> mScrollbarMediator;
}; 

#endif
