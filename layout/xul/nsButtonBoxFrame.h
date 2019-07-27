



#ifndef nsButtonBoxFrame_h___
#define nsButtonBoxFrame_h___

#include "mozilla/Attributes.h"
#include "nsIDOMEventListener.h"
#include "nsBoxFrame.h"

class nsButtonBoxFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewButtonBoxFrame(nsIPresShell* aPresShell);

  explicit nsButtonBoxFrame(nsStyleContext* aContext);

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual nsresult HandleEvent(nsPresContext* aPresContext, 
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetGUIEvent* aEvent)
  { DoMouseClick(aEvent, false); }

  void Blurred();

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override {
    return MakeFrameName(NS_LITERAL_STRING("ButtonBoxFrame"), aResult);
  }
#endif

  



  void DoMouseClick(mozilla::WidgetGUIEvent* aEvent, bool aTrustEvent);
  void UpdateMouseThrough() override { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }

private:
  class nsButtonBoxListener final : public nsIDOMEventListener
  {
  public:
    explicit nsButtonBoxListener(nsButtonBoxFrame* aButtonBoxFrame) :
      mButtonBoxFrame(aButtonBoxFrame)
      { }

    NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;

    NS_DECL_ISUPPORTS

  private:
    friend class nsButtonBoxFrame;
    virtual ~nsButtonBoxListener() { }
    nsButtonBoxFrame* mButtonBoxFrame;
  };

  nsRefPtr<nsButtonBoxListener> mButtonBoxListener;
  bool mIsHandlingKeyEvent;
}; 

#endif 
