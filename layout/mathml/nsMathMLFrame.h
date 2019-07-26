




#ifndef nsMathMLFrame_h___
#define nsMathMLFrame_h___

#include "mozilla/Attributes.h"
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
#include "nsLayoutUtils.h"

class nsMathMLChar;


class nsMathMLFrame : public nsIMathMLFrame {
public:

  

  virtual bool
  IsSpaceLike() {
    return NS_MATHML_IS_SPACE_LIKE(mPresentationData.flags);
  }

  NS_IMETHOD
  GetBoundingMetrics(nsBoundingMetrics& aBoundingMetrics) MOZ_OVERRIDE {
    aBoundingMetrics = mBoundingMetrics;
    return NS_OK;
  }

  NS_IMETHOD
  SetBoundingMetrics(const nsBoundingMetrics& aBoundingMetrics) MOZ_OVERRIDE {
    mBoundingMetrics = aBoundingMetrics;
    return NS_OK;
  }

  NS_IMETHOD
  SetReference(const nsPoint& aReference) MOZ_OVERRIDE {
    mReference = aReference;
    return NS_OK;
  }

  virtual eMathMLFrameType GetMathMLFrameType() MOZ_OVERRIDE;

  NS_IMETHOD
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize) MOZ_OVERRIDE
  {
    return NS_OK;
  }

  NS_IMETHOD
  GetEmbellishData(nsEmbellishData& aEmbellishData) MOZ_OVERRIDE {
    aEmbellishData = mEmbellishData;
    return NS_OK;
  }

  NS_IMETHOD
  GetPresentationData(nsPresentationData& aPresentationData) MOZ_OVERRIDE {
    aPresentationData = mPresentationData;
    return NS_OK;
  }

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE
  {
    return NS_OK;
  }

  NS_IMETHOD
  UpdatePresentationData(uint32_t        aFlagsValues,
                         uint32_t        aFlagsToUpdate) MOZ_OVERRIDE;

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate) MOZ_OVERRIDE
  {
    return NS_OK;
  }

  
  
  
  static void
  ResolveMathMLCharStyle(nsPresContext*  aPresContext,
                         nsIContent*      aContent,
                         nsStyleContext*  aParenStyleContext,
                         nsMathMLChar*    aMathMLChar,
                         bool             aIsMutableChar);

  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  GetEmbellishDataFrom(nsIFrame*        aFrame,
                       nsEmbellishData& aEmbellishData);

  
  
  
  
  static void
  GetPresentationDataFrom(nsIFrame*           aFrame,
                          nsPresentationData& aPresentationData,
                          bool                aClimbTree = true);

  
  static void
  FindAttrDisplaystyle(nsIContent*         aContent,
                       nsPresentationData& aPresentationData);

  
  static void
  FindAttrDirectionality(nsIContent*         aContent,
                         nsPresentationData& aPresentationData);

  
  
  
  
  static bool
  GetAttribute(nsIContent* aContent,
               nsIFrame*   aMathMLmstyleFrame,          
               nsIAtom*    aAttributeAtom,
               nsString&   aValue);

  
  
  
  
  static void ParseNumericValue(const nsString&   aString,
                                nscoord*          aLengthValue,
                                uint32_t          aFlags,
                                nsPresContext*    aPresContext,
                                nsStyleContext*   aStyleContext);

  static nscoord 
  CalcLength(nsPresContext*   aPresContext,
             nsStyleContext*   aStyleContext,
             const nsCSSValue& aCSSValue);

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
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(aChild, getter_AddRefs(fm));
    GetSubDrop(fm, aSubDrop);
  }

  static void 
  GetSupDropFromChild(nsIFrame*       aChild,
                      nscoord&        aSupDrop) 
  {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(aChild, getter_AddRefs(fm));
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
    nscoord xHeight = fm->XHeight();
    aSubScriptShift1 = NSToCoordRound(150.000f/430.556f * xHeight);
    aSubScriptShift2 = NSToCoordRound(247.217f/430.556f * xHeight);
  }

  
  static void
  GetSupScriptShifts(nsFontMetrics* fm, 
                     nscoord&        aSupScriptShift1, 
                     nscoord&        aSupScriptShift2, 
                     nscoord&        aSupScriptShift3)
  {
    nscoord xHeight = fm->XHeight();
    aSupScriptShift1 = NSToCoordRound(412.892f/430.556f * xHeight);
    aSupScriptShift2 = NSToCoordRound(362.892f/430.556f * xHeight);
    aSupScriptShift3 = NSToCoordRound(288.889f/430.556f * xHeight);
  }

  

  static void
  GetSubDrop(nsFontMetrics* fm,
             nscoord&        aSubDrop)
  {
    nscoord xHeight = fm->XHeight();
    aSubDrop = NSToCoordRound(50.000f/430.556f * xHeight);
  }

  static void
  GetSupDrop(nsFontMetrics* fm,
             nscoord&        aSupDrop)
  {
    nscoord xHeight = fm->XHeight();
    aSupDrop = NSToCoordRound(386.108f/430.556f * xHeight);
  }

  static void
  GetNumeratorShifts(nsFontMetrics* fm, 
                     nscoord&        numShift1, 
                     nscoord&        numShift2, 
                     nscoord&        numShift3)
  {
    nscoord xHeight = fm->XHeight();
    numShift1 = NSToCoordRound(676.508f/430.556f * xHeight);
    numShift2 = NSToCoordRound(393.732f/430.556f * xHeight);
    numShift3 = NSToCoordRound(443.731f/430.556f * xHeight);
  }

  static void
  GetDenominatorShifts(nsFontMetrics* fm, 
                       nscoord&        denShift1, 
                       nscoord&        denShift2)
  {
    nscoord xHeight = fm->XHeight();
    denShift1 = NSToCoordRound(685.951f/430.556f * xHeight);
    denShift2 = NSToCoordRound(344.841f/430.556f * xHeight);
  }

  static void
  GetEmHeight(nsFontMetrics* fm,
              nscoord&        emHeight)
  {
#if 0
    
    emHeight = fm->EmHeight();
#else
    emHeight = NSToCoordRound(float(fm->Font().size));
#endif
  }

  static void
  GetAxisHeight (nsFontMetrics* fm,
                 nscoord&        axisHeight)
  {
    axisHeight = NSToCoordRound(250.000f/430.556f * fm->XHeight());
  }

  static void
  GetBigOpSpacings(nsFontMetrics* fm, 
                   nscoord&        bigOpSpacing1,
                   nscoord&        bigOpSpacing2,
                   nscoord&        bigOpSpacing3,
                   nscoord&        bigOpSpacing4,
                   nscoord&        bigOpSpacing5)
  {
    nscoord xHeight = fm->XHeight();
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
    nscoord xHeight = fm->XHeight();
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
#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
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
