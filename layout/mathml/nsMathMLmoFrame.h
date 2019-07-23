







































#ifndef nsMathMLmoFrame_h___
#define nsMathMLmoFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLTokenFrame.h"





class nsMathMLmoFrame : public nsMathMLTokenFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmoFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual eMathMLFrameType GetMathMLFrameType();

  virtual void
  SetAdditionalStyleContext(PRInt32          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(PRInt32 aIndex) const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  virtual void MarkIntrinsicWidthsDirty();

  virtual nscoord
  GetIntrinsicWidth(nsIRenderingContext *aRenderingContext);

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  
  
  NS_IMETHOD
  Stretch(nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize);

protected:
  nsMathMLmoFrame(nsStyleContext* aContext) : nsMathMLTokenFrame(aContext) {}
  virtual ~nsMathMLmoFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

  nsMathMLChar     mMathMLChar; 
  nsOperatorFlags  mFlags;
  float            mMinSize;
  float            mMaxSize;

  PRBool UseMathMLChar();

  
  virtual void ProcessTextData();

  
  
  
  void
  ProcessOperatorData();

  
  PRBool
  IsFrameInSelection(nsIFrame* aFrame);
};

#endif 
