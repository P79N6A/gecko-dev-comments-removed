








#ifndef nsScrollbarFrame_h__
#define nsScrollbarFrame_h__

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsIScrollbarMediator;

nsIFrame* NS_NewScrollbarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsScrollbarFrame : public nsBoxFrame
{
public:
    nsScrollbarFrame(nsIPresShell* aShell, nsStyleContext* aContext):
      nsBoxFrame(aShell, aContext), mScrollbarMediator(nullptr) {}

  NS_DECL_QUERYFRAME_TARGET(nsScrollbarFrame)

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("ScrollbarFrame"), aResult);
  }
#endif

  
  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus,
                                 bool aControlHeld) MOZ_OVERRIDE;

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        mozilla::WidgetGUIEvent* aEvent,
                        nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           mozilla::WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;  

  void SetScrollbarMediatorContent(nsIContent* aMediator);
  nsIScrollbarMediator* GetScrollbarMediator();

  

  






  virtual bool DoesClipChildren() MOZ_OVERRIDE { return true; }

  virtual nsresult GetMargin(nsMargin& aMargin) MOZ_OVERRIDE;

  



  void SetIncrementToLine(int32_t aDirection);
  void SetIncrementToPage(int32_t aDirection);
  void SetIncrementToWhole(int32_t aDirection);
  




  int32_t MoveToNewPosition();
  int32_t GetIncrement() { return mIncrement; }

protected:
  int32_t mIncrement; 
  bool mSmoothScroll;

private:
  nsCOMPtr<nsIContent> mScrollbarMediator;
}; 

#endif
