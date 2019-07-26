








#ifndef nsSplitterFrame_h__
#define nsSplitterFrame_h__


#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsSplitterFrameInner;

nsIFrame* NS_NewSplitterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsSplitterFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsSplitterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("SplitterFrame"), aResult);
  }
#endif

  
  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual nsresult GetCursor(const nsPoint&    aPoint,
                             nsIFrame::Cursor& aCursor) MOZ_OVERRIDE;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

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

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void GetInitialOrientation(bool& aIsHorizontal) MOZ_OVERRIDE; 

private:

  friend class nsSplitterFrameInner;
  nsSplitterFrameInner* mInner;

}; 

#endif
