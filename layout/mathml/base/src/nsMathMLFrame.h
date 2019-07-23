




































#ifndef nsMathMLFrame_h___
#define nsMathMLFrame_h___

#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsStyleContext.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsIMathMLFrame.h"
#include "nsFrame.h"
#include "nsCSSValue.h"

class nsMathMLChar;


class nsMathMLFrame : public nsIMathMLFrame {
public:

  

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_IMETHOD_(nsrefcnt) AddRef() {
    
    return 1;
  }

  NS_IMETHOD_(nsrefcnt) Release() {
    
    return 1;
  }

  

  NS_IMETHOD
  GetBoundingMetrics(nsBoundingMetrics& aBoundingMetrics) {
    aBoundingMetrics = mBoundingMetrics;
    return NS_OK;
  }

  NS_IMETHOD
  SetBoundingMetrics(const nsBoundingMetrics& aBoundingMetrics) {
    mBoundingMetrics = aBoundingMetrics;
    return NS_OK;
  }

  NS_IMETHOD
  GetReference(nsPoint& aReference) {
    aReference = mReference;
    return NS_OK;
  }

  NS_IMETHOD
  SetReference(const nsPoint& aReference) {
    mReference = aReference;
    return NS_OK;
  }

  virtual eMathMLFrameType GetMathMLFrameType();

  NS_IMETHOD
  Stretch(nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize)
  {
    return NS_OK;
  }

  NS_IMETHOD
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize)
  {
    return NS_OK;
  }

  NS_IMETHOD
  GetEmbellishData(nsEmbellishData& aEmbellishData) {
    aEmbellishData = mEmbellishData;
    return NS_OK;
  }
 
  NS_IMETHOD
  SetEmbellishData(const nsEmbellishData& aEmbellishData) {
    mEmbellishData = aEmbellishData;
    return NS_OK;
  }

  NS_IMETHOD
  GetPresentationData(nsPresentationData& aPresentationData) {
    aPresentationData = mPresentationData;
    return NS_OK;
  }

  NS_IMETHOD
  SetPresentationData(const nsPresentationData& aPresentationData) {
    mPresentationData = aPresentationData;
    return NS_OK;
  }

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData()
  {
    return NS_OK;
  }

  NS_IMETHOD
  UpdatePresentationData(PRInt32         aScriptLevelIncrement,
                         PRUint32        aFlagsValues,
                         PRUint32        aFlagsToUpdate);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    return NS_OK;
  }

  NS_IMETHOD
  ReResolveScriptStyle(PRInt32 aParentScriptLevel)
  {
    return NS_OK;
  }

  
  
  
  static void
  ResolveMathMLCharStyle(nsPresContext*  aPresContext,
                         nsIContent*      aContent,
                         nsStyleContext*  aParenStyleContext,
                         nsMathMLChar*    aMathMLChar,
                         PRBool           aIsMutableChar);

  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  GetEmbellishDataFrom(nsIFrame*        aFrame,
                       nsEmbellishData& aEmbellishData);

  
  
  
  
  static void
  GetPresentationDataFrom(nsIFrame*           aFrame,
                          nsPresentationData& aPresentationData,
                          PRBool              aClimbTree = PR_TRUE);

  
  static void
  FindAttrDisplaystyle(nsIContent*         aContent,
                       nsPresentationData& aPresentationData);

  
  
  
  
  static PRBool
  GetAttribute(nsIContent* aContent,
               nsIFrame*   aMathMLmstyleFrame,          
               nsIAtom*    aAttributeAtom,
               nsString&   aValue);

  
  
  static PRBool
  ParseNumericValue(nsString&   aString,
                    nsCSSValue& aCSSValue);

  static nscoord 
  CalcLength(nsPresContext*   aPresContext,
             nsStyleContext*   aStyleContext,
             const nsCSSValue& aCSSValue);

  static PRBool
  ParseNamedSpaceValue(nsIFrame*   aMathMLmstyleFrame,
                       nsString&   aString,
                       nsCSSValue& aCSSValue);

  static eMathMLFrameType
  GetMathMLFrameTypeFor(nsIFrame* aFrame)
  {
    if (aFrame->IsFrameOfType(nsIFrame::eMathML)) {
      nsIMathMLFrame* mathMLFrame;
      CallQueryInterface(aFrame, &mathMLFrame);
      if (mathMLFrame)
        return mathMLFrame->GetMathMLFrameType();
    }
    return eMathMLFrameType_UNKNOWN;
  }

  
  static void
  GetItalicCorrection(nsBoundingMetrics& aBoundingMetrics,
                      nscoord&           aItalicCorrection)
  {
    aItalicCorrection = aBoundingMetrics.rightBearing - aBoundingMetrics.width;
    if (0 > aItalicCorrection) {
      aItalicCorrection = 0;
    }
  }

  static void
  GetItalicCorrection(nsBoundingMetrics& aBoundingMetrics,
                      nscoord&           aLeftItalicCorrection,
                      nscoord&           aRightItalicCorrection)
  {
    aRightItalicCorrection = aBoundingMetrics.rightBearing - aBoundingMetrics.width;
    if (0 > aRightItalicCorrection) {
      aRightItalicCorrection = 0;
    }
    aLeftItalicCorrection = -aBoundingMetrics.leftBearing;
    if (0 > aLeftItalicCorrection) {
      aLeftItalicCorrection = 0;
    }
  }

  
  static void 
  GetSubDropFromChild(nsIFrame*       aChild,
                      nscoord&        aSubDrop) 
  {
    const nsStyleFont* font = aChild->GetStyleFont();
    nsCOMPtr<nsIFontMetrics> fm = aChild->GetPresContext()->GetMetricsFor(
                                                              font->mFont);
    GetSubDrop(fm, aSubDrop);
  }

  static void 
  GetSupDropFromChild(nsIFrame*       aChild,
                      nscoord&        aSupDrop) 
  {
    const nsStyleFont* font = aChild->GetStyleFont();
    nsCOMPtr<nsIFontMetrics> fm = aChild->GetPresContext()->GetMetricsFor(
                                                              font->mFont);
    GetSupDrop(fm, aSupDrop);
  }

  static void
  GetSkewCorrectionFromChild(nsIFrame*       aChild,
                             nscoord&        aSkewCorrection) 
  {
    
    
    aSkewCorrection = 0;
  }

  
  static void
  GetSubScriptShifts(nsIFontMetrics* fm, 
                     nscoord&        aSubScriptShift1, 
                     nscoord&        aSubScriptShift2)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSubScriptShift1 = NSToCoordRound(150.000f/430.556f * xHeight);
    aSubScriptShift2 = NSToCoordRound(247.217f/430.556f * xHeight);
  }

  
  static void
  GetSupScriptShifts(nsIFontMetrics* fm, 
                     nscoord&        aSupScriptShift1, 
                     nscoord&        aSupScriptShift2, 
                     nscoord&        aSupScriptShift3)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSupScriptShift1 = NSToCoordRound(412.892f/430.556f * xHeight);
    aSupScriptShift2 = NSToCoordRound(362.892f/430.556f * xHeight);
    aSupScriptShift3 = NSToCoordRound(288.889f/430.556f * xHeight);
  }

  

  static void
  GetSubDrop(nsIFontMetrics* fm,
             nscoord&        aSubDrop)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSubDrop = NSToCoordRound(50.000f/430.556f * xHeight);
  }

  static void
  GetSupDrop(nsIFontMetrics* fm,
             nscoord&        aSupDrop)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSupDrop = NSToCoordRound(386.108f/430.556f * xHeight);
  }

  static void
  GetNumeratorShifts(nsIFontMetrics* fm, 
                     nscoord&        numShift1, 
                     nscoord&        numShift2, 
                     nscoord&        numShift3)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    numShift1 = NSToCoordRound(676.508f/430.556f * xHeight);
    numShift2 = NSToCoordRound(393.732f/430.556f * xHeight);
    numShift3 = NSToCoordRound(443.731f/430.556f * xHeight);
  }

  static void
  GetDenominatorShifts(nsIFontMetrics* fm, 
                       nscoord&        denShift1, 
                       nscoord&        denShift2)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    denShift1 = NSToCoordRound(685.951f/430.556f * xHeight);
    denShift2 = NSToCoordRound(344.841f/430.556f * xHeight);
  }

  static void
  GetEmHeight(nsIFontMetrics* fm,
              nscoord&        emHeight)
  {
#if 0 
    
    fm->GetEmHeight(emHeight);
#else
    emHeight = NSToCoordRound(float(fm->Font().size));
#endif
  }

  static void
  GetAxisHeight (nsIFontMetrics* fm,
                 nscoord&        axisHeight)
  {
    fm->GetXHeight (axisHeight);
    axisHeight = NSToCoordRound(250.000f/430.556f * axisHeight);
  }

  static void
  GetBigOpSpacings(nsIFontMetrics* fm, 
                   nscoord&        bigOpSpacing1,
                   nscoord&        bigOpSpacing2,
                   nscoord&        bigOpSpacing3,
                   nscoord&        bigOpSpacing4,
                   nscoord&        bigOpSpacing5)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    bigOpSpacing1 = NSToCoordRound(111.111f/430.556f * xHeight);
    bigOpSpacing2 = NSToCoordRound(166.667f/430.556f * xHeight);
    bigOpSpacing3 = NSToCoordRound(200.000f/430.556f * xHeight);
    bigOpSpacing4 = NSToCoordRound(600.000f/430.556f * xHeight);
    bigOpSpacing5 = NSToCoordRound(100.000f/430.556f * xHeight);
  }

  static void
  GetRuleThickness(nsIFontMetrics* fm,
                   nscoord&        ruleThickness)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    ruleThickness = NSToCoordRound(40.000f/430.556f * xHeight);
  }

  
  
  
  static void
  GetRuleThickness(nsIRenderingContext& aRenderingContext, 
                   nsIFontMetrics*      aFontMetrics,
                   nscoord&             aRuleThickness);

  static void
  GetAxisHeight(nsIRenderingContext& aRenderingContext, 
                nsIFontMetrics*      aFontMetrics,
                nscoord&             aAxisHeight);

  
  
  
  static PRInt32
  MapCommonAttributesIntoCSS(nsPresContext* aPresContext,
                             nsIContent*    aContent);
  static PRInt32
  MapCommonAttributesIntoCSS(nsPresContext* aPresContext,
                             nsIFrame*      aFrame);
 
  
  
  
  static PRBool
  CommonAttributeChangedFor(nsPresContext* aPresContext,
                            nsIContent*    aContent,
                            nsIAtom*       aAttribute);

protected:
#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
  nsresult DisplayBoundingMetrics(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, const nsPoint& aPt,
                                  const nsBoundingMetrics& aMetrics,
                                  const nsDisplayListSet& aLists);
#endif

  



  nsresult DisplayBar(nsDisplayListBuilder* aBuilder,
                      nsIFrame* aFrame, const nsRect& aRect,
                      const nsDisplayListSet& aLists);

  
  nsPresentationData mPresentationData;

  
  nsEmbellishData mEmbellishData;
  
  
  nsBoundingMetrics mBoundingMetrics;
  
  
  nsPoint mReference; 
};

#endif 
