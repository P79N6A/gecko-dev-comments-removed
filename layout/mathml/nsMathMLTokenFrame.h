





































#ifndef nsMathMLTokenFrame_h___
#define nsMathMLTokenFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLTokenFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLTokenFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData() {
    
    
    if (mContent->Tag() == nsGkAtoms::mtext_) {
      mPresentationData.flags |= NS_MATHML_SPACE_LIKE;
    }
    return NS_OK;
  }

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  virtual eMathMLFrameType GetMathMLFrameType();

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  virtual void MarkIntrinsicWidthsDirty();

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  virtual nsresult
  ChildListChanged(PRInt32 aModType)
  {
    ProcessTextData();
    return nsMathMLContainerFrame::ChildListChanged(aModType);
  }

protected:
  nsMathMLTokenFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLTokenFrame();

  virtual PRIntn GetSkipSides() const { return 0; }

  
  virtual void ProcessTextData();

  
  
  bool SetTextStyle();

  
  void SetQuotes(bool aNotify);
};

#endif 
