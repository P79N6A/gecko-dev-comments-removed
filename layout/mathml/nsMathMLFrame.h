





































#ifndef nsMathMLFrame_h___
#define nsMathMLFrame_h___

#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsFontMetrics.h"
#include "nsStyleContext.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsIMathMLFrame.h"
#include "nsFrame.h"
#include "nsCSSValue.h"
#include "nsMathMLElement.h"

class nsMathMLChar;


class nsMathMLFrame : public nsIMathMLFrame {
public:

  

  virtual PRBool
  IsSpaceLike() {
    return NS_MATHML_IS_SPACE_LIKE(mPresentationData.flags);
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
  SetReference(const nsPoint& aReference) {
    mReference = aReference;
    return NS_OK;
  }

  virtual eMathMLFrameType GetMathMLFrameType();

  NS_IMETHOD
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize)
  {
    return NS_OK;
  }

  NS_IMETHOD
  GetEmbellishData(nsEmbellishData& aEmbellishData) {
    aEmbellishData = mEmbellishData;
    return NS_OK;
  }

  NS_IMETHOD
  GetPresentationData(nsPresentationData& aPresentationData) {
    aPresentationData = mPresentationData;
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
  UpdatePresentationData(PRUint32        aFlagsValues,
                         PRUint32        aFlagsToUpdate);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
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
  ParseNumericValue(const nsString& aString,
                    nsCSSValue&     aCSSValue) {
    return nsMathMLElement::ParseNumericValue(aString, aCSSValue,
            nsMathMLElement::PARSE_ALLOW_NEGATIVE |
            nsMathMLElement::PARSE_ALLOW_UNITLESS);
  }

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
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(aFrame);
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
    nsRefPtr<nsFontMetrics> fm = aChild->PresContext()->GetMetricsFor(
                                                              font->mFont);
    GetSubDrop(fm, aSubDrop);
  }

  static void 
  GetSupDropFromChild(nsIFrame*       aChild,
                      nscoord&        aSupDrop) 
  {
    const nsStyleFont* font = aChild->GetStyleFont();
    nsRefPtr<nsFontMetrics> fm = aChild->PresContext()->GetMetricsFor(
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
  GetSubScriptShifts(nsFontMetrics* fm, 
                     nscoord&        aSubScriptShift1, 
                     nscoord&        aSubScriptShift2)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSubScriptShift1 = NSToCoordRound(150.000f/430.556f * xHeight);
    aSubScriptShift2 = NSToCoordRound(247.217f/430.556f * xHeight);
  }

  
  static void
  GetSupScriptShifts(nsFontMetrics* fm, 
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
  GetSubDrop(nsFontMetrics* fm,
             nscoord&        aSubDrop)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSubDrop = NSToCoordRound(50.000f/430.556f * xHeight);
  }

  static void
  GetSupDrop(nsFontMetrics* fm,
             nscoord&        aSupDrop)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    aSupDrop = NSToCoordRound(386.108f/430.556f * xHeight);
  }

  static void
  GetNumeratorShifts(nsFontMetrics* fm, 
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
  GetDenominatorShifts(nsFontMetrics* fm, 
                       nscoord&        denShift1, 
                       nscoord&        denShift2)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    denShift1 = NSToCoordRound(685.951f/430.556f * xHeight);
    denShift2 = NSToCoordRound(344.841f/430.556f * xHeight);
  }

  static void
  GetEmHeight(nsFontMetrics* fm,
              nscoord&        emHeight)
  {
#if 0 
    
    fm->GetEmHeight(emHeight);
#else
    emHeight = NSToCoordRound(float(fm->Font().size));
#endif
  }

  static void
  GetAxisHeight (nsFontMetrics* fm,
                 nscoord&        axisHeight)
  {
    fm->GetXHeight (axisHeight);
    axisHeight = NSToCoordRound(250.000f/430.556f * axisHeight);
  }

  static void
  GetBigOpSpacings(nsFontMetrics* fm, 
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
  GetRuleThickness(nsFontMetrics* fm,
                   nscoord&        ruleThickness)
  {
    nscoord xHeight;
    fm->GetXHeight(xHeight);
    ruleThickness = NSToCoordRound(40.000f/430.556f * xHeight);
  }

  
  
  
  static void
  GetRuleThickness(nsRenderingContext& aRenderingContext, 
                   nsFontMetrics*      aFontMetrics,
                   nscoord&             aRuleThickness);

  static void
  GetAxisHeight(nsRenderingContext& aRenderingContext, 
                nsFontMetrics*      aFontMetrics,
                nscoord&             aAxisHeight);

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
