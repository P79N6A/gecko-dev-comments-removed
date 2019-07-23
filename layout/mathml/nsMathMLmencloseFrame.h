










































#ifndef nsMathMLmencloseFrame_h___
#define nsMathMLmencloseFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"















enum nsMencloseNotation
  {
    NOTATION_LONGDIV = 0x1,
    NOTATION_RADICAL = 0x2,
    NOTATION_ROUNDEDBOX = 0x4,
    NOTATION_CIRCLE = 0x8,
    NOTATION_LEFT = 0x10,
    NOTATION_RIGHT = 0x20,
    NOTATION_TOP = 0x40,
    NOTATION_BOTTOM = 0x80,
    NOTATION_UPDIAGONALSTRIKE = 0x100,
    NOTATION_DOWNDIAGONALSTRIKE = 0x200,
    NOTATION_VERTICALSTRIKE = 0x400,
    NOTATION_HORIZONTALSTRIKE = 0x800
    
  };

class nsMathMLmencloseFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmencloseFrame(nsIPresShell*   aPresShell,
                                             nsStyleContext* aContext);
  
  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);
  
  virtual nsresult
  MeasureForWidth(nsIRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize);
  
  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);
  
  virtual void
  SetAdditionalStyleContext(PRInt32          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(PRInt32 aIndex) const;

  NS_IMETHOD
  Init(nsIContent* aContent,
       nsIFrame*   aParent,
       nsIFrame*   aPrevInFlow);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData();

  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

protected:
  nsMathMLmencloseFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmencloseFrame();

  nsresult PlaceInternal(nsIRenderingContext& aRenderingContext,
                         PRBool               aPlaceOrigin,
                         nsHTMLReflowMetrics& aDesiredSize,
                         PRBool               aWidthOnly);
  
  
  nsresult AddNotation(const nsAString& aNotation);
  void InitNotations();

  
  PRUint32 mNotationsToDraw;
  PRBool IsToDraw(nsMencloseNotation mask)
  {
    return mask & mNotationsToDraw;
  }

  nscoord mRuleThickness;
  nsTArray<nsMathMLChar> mMathMLChar;
  PRInt8 mLongDivCharIndex, mRadicalCharIndex;
  nscoord mContentWidth;
  nsresult AllocateMathMLChar(nsMencloseNotation mask);

  
  
  nsresult DisplayNotation(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aFrame, const nsRect& aRect,
                           const nsDisplayListSet& aLists,
                           nscoord aThickness, nsMencloseNotation aType);
};

#endif 
