




#ifndef nsMathMLmactionFrame_h___
#define nsMathMLmactionFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"
#include "nsIDOMEventListener.h"
#include "mozilla/Attributes.h"





class nsMathMLmactionFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmactionFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual nsresult
  ChildListChanged(int32_t aModType) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  NS_IMETHOD
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType) MOZ_OVERRIDE;

private:
  void MouseClick();
  void MouseOver();
  void MouseOut();

  class MouseListener MOZ_FINAL : public nsIDOMEventListener
  {
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMEVENTLISTENER

    MouseListener(nsMathMLmactionFrame* aOwner) : mOwner(aOwner) { }

    nsMathMLmactionFrame* mOwner;
  };

protected:
  nsMathMLmactionFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmactionFrame();
  
private:
  int32_t         mActionType;
  int32_t         mChildCount;
  int32_t         mSelection;
  nsIFrame*       mSelectedFrame;
  nsCOMPtr<MouseListener> mListener;

  
  nsIFrame* 
  GetSelectedFrame();
};

#endif 
