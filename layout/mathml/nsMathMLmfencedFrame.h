






































#ifndef nsMathMLmfencedFrame_h___
#define nsMathMLmfencedFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmfencedFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmfencedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  SetAdditionalStyleContext(PRInt32          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(PRInt32 aIndex) const;

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsFrameList&    aChildList);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nscoord
  GetIntrinsicWidth(nsIRenderingContext* aRenderingContext);

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  
  virtual nsresult
  ChildListChanged(PRInt32 aModType);

  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

  
  
  static nsresult
  doReflow(nsPresContext*          aPresContext,
           const nsHTMLReflowState& aReflowState,
           nsHTMLReflowMetrics&     aDesiredSize,
           nsReflowStatus&          aStatus,
           nsMathMLContainerFrame*  aForFrame,
           nsMathMLChar*            aOpenChar,
           nsMathMLChar*            aCloseChar,
           nsMathMLChar*            aSeparatorsChar,
           PRInt32                  aSeparatorsCount);

  static nscoord
  doGetIntrinsicWidth(nsIRenderingContext*    aRenderingContext,
                      nsMathMLContainerFrame* aForFrame,
                      nsMathMLChar*           aOpenChar,
                      nsMathMLChar*           aCloseChar,
                      nsMathMLChar*           aSeparatorsChar,
                      PRInt32                 aSeparatorsCount);

  
  static nsresult
  ReflowChar(nsPresContext*      aPresContext,
             nsIRenderingContext& aRenderingContext,
             nsMathMLChar*        aMathMLChar,
             nsOperatorFlags      aForm,
             PRInt32              aScriptLevel,
             nscoord              axisHeight,
             nscoord              leading,
             nscoord              em,
             nsBoundingMetrics&   aContainerSize,
             nscoord&             aAscent,
             nscoord&             aDescent);

  static void
  PlaceChar(nsMathMLChar*      aMathMLChar,
            nscoord            aDesiredSize,
            nsBoundingMetrics& bm,
            nscoord&           dx);

protected:
  nsMathMLmfencedFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmfencedFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

  nsMathMLChar* mOpenChar;
  nsMathMLChar* mCloseChar;
  nsMathMLChar* mSeparatorsChar;
  PRInt32       mSeparatorsCount;

  
  void
  RemoveFencesAndSeparators();

  
  nsresult
  CreateFencesAndSeparators(nsPresContext* aPresContext);
};

#endif 
