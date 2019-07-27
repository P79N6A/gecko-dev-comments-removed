








#ifndef nsScrollbarFrame_h__
#define nsScrollbarFrame_h__

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"
#include "nsGfxScrollFrame.h"

class nsIScrollbarMediator;

nsIFrame* NS_NewScrollbarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsScrollbarFrame : public nsBoxFrame
{
public:
    explicit nsScrollbarFrame(nsStyleContext* aContext):
      nsBoxFrame(aContext), mScrollbarMediator(nullptr) {}

  NS_DECL_QUERYFRAME_TARGET(nsScrollbarFrame)

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override {
    return MakeFrameName(NS_LITERAL_STRING("ScrollbarFrame"), aResult);
  }
#endif

  
  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) override;

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus,
                                 bool aControlHeld) override;

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        mozilla::WidgetGUIEvent* aEvent,
                        nsEventStatus* aEventStatus) override;

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           mozilla::WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual nsIAtom* GetType() const override;  

  void SetScrollbarMediatorContent(nsIContent* aMediator);
  nsIScrollbarMediator* GetScrollbarMediator();

  

  






  virtual bool DoesClipChildren() override { return true; }

  nsresult GetScrollbarMargin(nsMargin& aMargin,
                              mozilla::ScrollFrameHelper::eScrollbarSide aSide);

  



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
