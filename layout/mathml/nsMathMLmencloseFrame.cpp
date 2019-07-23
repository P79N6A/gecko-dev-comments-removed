











































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsWhitespaceTokenizer.h"

#include "nsMathMLmencloseFrame.h"
#include "nsDisplayList.h"
#include "gfxContext.h"








static const PRUnichar kLongDivChar = ')';


static const PRUnichar kRadicalChar = 0x221A;

nsIFrame*
NS_NewMathMLmencloseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmencloseFrame(aContext);
}

nsMathMLmencloseFrame::nsMathMLmencloseFrame(nsStyleContext* aContext) :
  nsMathMLContainerFrame(aContext), mNotationsToDraw(0),
  mLongDivCharIndex(-1), mRadicalCharIndex(-1), mContentWidth(0)
{
}

nsMathMLmencloseFrame::~nsMathMLmencloseFrame()
{
}

nsresult nsMathMLmencloseFrame::AllocateMathMLChar(nsMencloseNotation mask)
{
  
  if ((mask == NOTATION_LONGDIV && mLongDivCharIndex >= 0) ||
      (mask == NOTATION_RADICAL && mRadicalCharIndex >= 0))
    return NS_OK;

  
  
  
  PRUint32 i = mMathMLChar.Length();
  nsAutoString Char;

  if (!mMathMLChar.AppendElement())
    return NS_ERROR_OUT_OF_MEMORY;

  if (mask == NOTATION_LONGDIV) {
    Char.Assign(kLongDivChar);
    mLongDivCharIndex = i;
  } else if (mask == NOTATION_RADICAL) {
    Char.Assign(kRadicalChar);
    mRadicalCharIndex = i;
  }

  nsPresContext *presContext = PresContext();
  mMathMLChar[i].SetData(presContext, Char);
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext,
                         &mMathMLChar[i],
                         PR_TRUE);

  return NS_OK;
}





nsresult nsMathMLmencloseFrame::AddNotation(const nsAString& aNotation)
{
  nsresult rv;

  if (aNotation.EqualsLiteral("longdiv")) {
    rv = AllocateMathMLChar(NOTATION_LONGDIV);
    NS_ENSURE_SUCCESS(rv, rv);
    mNotationsToDraw |= NOTATION_LONGDIV;
  } else if (aNotation.EqualsLiteral("actuarial")) {
    mNotationsToDraw |= (NOTATION_RIGHT | NOTATION_TOP);
  } else if (aNotation.EqualsLiteral("radical")) {
    rv = AllocateMathMLChar(NOTATION_RADICAL);
    NS_ENSURE_SUCCESS(rv, rv);
    mNotationsToDraw |= NOTATION_RADICAL;
  } else if (aNotation.EqualsLiteral("box")) {
    mNotationsToDraw |= (NOTATION_LEFT | NOTATION_RIGHT |
                         NOTATION_TOP | NOTATION_BOTTOM);
  } else if (aNotation.EqualsLiteral("roundedbox")) {
    mNotationsToDraw |= NOTATION_ROUNDEDBOX;
  } else if (aNotation.EqualsLiteral("circle")) {
    mNotationsToDraw |= NOTATION_CIRCLE;
  } else if (aNotation.EqualsLiteral("left")) {
    mNotationsToDraw |= NOTATION_LEFT;
  } else if (aNotation.EqualsLiteral("right")) {
    mNotationsToDraw |= NOTATION_RIGHT;
  } else if (aNotation.EqualsLiteral("top")) {
    mNotationsToDraw |= NOTATION_TOP;
  } else if (aNotation.EqualsLiteral("bottom")) {
    mNotationsToDraw |= NOTATION_BOTTOM;
  } else if (aNotation.EqualsLiteral("updiagonalstrike")) {
    mNotationsToDraw |= NOTATION_UPDIAGONALSTRIKE;
  } else if (aNotation.EqualsLiteral("downdiagonalstrike")) {
    mNotationsToDraw |= NOTATION_DOWNDIAGONALSTRIKE;
  } else if (aNotation.EqualsLiteral("verticalstrike")) {
    mNotationsToDraw |= NOTATION_VERTICALSTRIKE;
  } else if (aNotation.EqualsLiteral("horizontalstrike")) {
    mNotationsToDraw |= NOTATION_HORIZONTALSTRIKE;
  }

  return NS_OK;
}




void nsMathMLmencloseFrame::InitNotations()
{
  nsAutoString value;

  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::notation_, value)) {
    
    nsWhitespaceTokenizer tokenizer(value);

    while (tokenizer.hasMoreTokens())
      AddNotation(tokenizer.nextToken());
  } else {
    
    if (NS_FAILED(AllocateMathMLChar(NOTATION_LONGDIV)))
      return;
    mNotationsToDraw = NOTATION_LONGDIV;
  }
}

NS_IMETHODIMP
nsMathMLmencloseFrame::Init(nsIContent*      aContent,
                            nsIFrame*        aParent,
                            nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  InitNotations();

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmencloseFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmencloseFrame::TransmitAutomaticData()
{
  if (IsToDraw(NOTATION_RADICAL)) {
    
    UpdatePresentationDataFromChildAt(0, -1,
                                      NS_MATHML_COMPRESSED,
                                      NS_MATHML_COMPRESSED);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmencloseFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  
  
  nsresult rv = nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect,
                                                         aLists);

  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_MATHML_HAS_ERROR(mPresentationData.flags))
    return rv;

  nsRect mencloseRect = nsIFrame::GetRect();
  mencloseRect.x = mencloseRect.y = 0;

  if (IsToDraw(NOTATION_RADICAL)) {
    rv = mMathMLChar[mRadicalCharIndex].Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRect rect;
    mMathMLChar[mRadicalCharIndex].GetRect(rect);
    rect.MoveBy(rect.width, 0);
    rect.SizeTo(mContentWidth, mRuleThickness);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_LONGDIV)) {
    rv = mMathMLChar[mLongDivCharIndex].Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRect rect;
    mMathMLChar[mLongDivCharIndex].GetRect(rect);
    rect.SizeTo(rect.width + mContentWidth, mRuleThickness);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_TOP)) {
    nsRect rect(0, 0, mencloseRect.width, mRuleThickness);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_BOTTOM)) {
    nsRect rect(0, mencloseRect.height - mRuleThickness,
                mencloseRect.width, mRuleThickness);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_LEFT)) {
    nsRect rect(0, 0, mRuleThickness, mencloseRect.height);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_RIGHT)) {
    nsRect rect(mencloseRect.width - mRuleThickness, 0,
                mRuleThickness, mencloseRect.height);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_ROUNDEDBOX)) {
    rv = DisplayNotation(aBuilder, this, mencloseRect, aLists,
                         mRuleThickness, NOTATION_ROUNDEDBOX);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_CIRCLE)) {
    rv = DisplayNotation(aBuilder, this, mencloseRect, aLists,
                         mRuleThickness, NOTATION_CIRCLE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_UPDIAGONALSTRIKE)) {
    rv = DisplayNotation(aBuilder, this, mencloseRect, aLists,
                         mRuleThickness, NOTATION_UPDIAGONALSTRIKE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_DOWNDIAGONALSTRIKE)) {
    rv = DisplayNotation(aBuilder, this, mencloseRect, aLists,
                         mRuleThickness, NOTATION_DOWNDIAGONALSTRIKE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_HORIZONTALSTRIKE)) {
    nsRect rect(0, mencloseRect.height / 2 - mRuleThickness / 2,
                mencloseRect.width, mRuleThickness);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (IsToDraw(NOTATION_VERTICALSTRIKE)) {
    nsRect rect(mencloseRect.width / 2 - mRuleThickness / 2, 0,
                mRuleThickness, mencloseRect.height);
    rv = DisplayBar(aBuilder, this, rect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return rv;
}

 nsresult
nsMathMLmencloseFrame::MeasureForWidth(nsIRenderingContext& aRenderingContext,
                                       nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext, PR_FALSE, aDesiredSize, PR_TRUE);
}

 nsresult
nsMathMLmencloseFrame::Place(nsIRenderingContext& aRenderingContext,
                             PRBool               aPlaceOrigin,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext, aPlaceOrigin, aDesiredSize, PR_FALSE);
}

 nsresult
nsMathMLmencloseFrame::PlaceInternal(nsIRenderingContext& aRenderingContext,
                                     PRBool               aPlaceOrigin,
                                     nsHTMLReflowMetrics& aDesiredSize,
                                     PRBool               aWidthOnly)
{
  
  
  
  nsHTMLReflowMetrics baseSize;
  nsresult rv =
    nsMathMLContainerFrame::Place(aRenderingContext, PR_FALSE, baseSize);

  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
      DidReflowChildren(GetFirstChild(nsnull));
      return rv;
    }

  nsBoundingMetrics bmBase = baseSize.mBoundingMetrics;
  nscoord dx_left = 0, dx_right = 0;
  nsBoundingMetrics bmLongdivChar, bmRadicalChar;
  nscoord radicalAscent = 0, radicalDescent = 0;
  nscoord longdivAscent = 0, longdivDescent = 0;
  nscoord psi = 0;

  
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  nsCOMPtr<nsIFontMetrics> fm;
  nscoord mEmHeight;
  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull,
                            PresContext()->GetUserFontSet());
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));
  GetRuleThickness(aRenderingContext, fm, mRuleThickness);
  GetEmHeight(fm, mEmHeight);

  nsBoundingMetrics bmOne;
  aRenderingContext.GetBoundingMetrics(NS_LITERAL_STRING("1").get(), 1, bmOne);

  
  
  

  
  nscoord padding = 3 * mRuleThickness;
  nscoord delta = padding % onePixel;
  if (delta)
    padding += onePixel - delta; 

  if (IsToDraw(NOTATION_LONGDIV) || IsToDraw(NOTATION_RADICAL)) {
      nscoord phi;
      
      
      if (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
        fm->GetXHeight(phi);
      else
        phi = mRuleThickness;
      psi = mRuleThickness + phi / 4;

      delta = psi % onePixel;
      if (delta)
        psi += onePixel - delta; 
    }

  if (mRuleThickness < onePixel)
    mRuleThickness = onePixel;
 
  
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    dx_left = padding;

  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    dx_right = padding;

  
  if (IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_RADICAL) ||
      IsToDraw(NOTATION_LONGDIV)) {
      
      bmBase.ascent = PR_MAX(bmOne.ascent, bmBase.ascent);
      bmBase.descent = PR_MAX(0, bmBase.descent);
  }

  mBoundingMetrics.ascent = bmBase.ascent;
  mBoundingMetrics.descent = bmBase.descent;
    
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_CIRCLE))
    mBoundingMetrics.ascent += padding;
  
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    mBoundingMetrics.descent += padding;

  
  
  
  
  if (IsToDraw(NOTATION_CIRCLE)) {
    double ratio = (sqrt(2.0) - 1.0) / 2.0;
    nscoord padding2;

    
    padding2 = ratio * bmBase.width;

    dx_left = PR_MAX(dx_left, padding2);
    dx_right = PR_MAX(dx_right, padding2);

    
    padding2 = ratio * (bmBase.ascent + bmBase.descent);

    mBoundingMetrics.ascent = PR_MAX(mBoundingMetrics.ascent,
                                     bmBase.ascent + padding2);
    mBoundingMetrics.descent = PR_MAX(mBoundingMetrics.descent,
                                      bmBase.descent + padding2);
  }

  
  
  if (IsToDraw(NOTATION_LONGDIV)) {
    if (aWidthOnly) {
        nscoord longdiv_width = mMathMLChar[mLongDivCharIndex].
          GetMaxWidth(PresContext(), aRenderingContext);

        
        dx_left = PR_MAX(dx_left, longdiv_width);
    } else {
      
      
      nsBoundingMetrics contSize = bmBase;
      contSize.ascent = mRuleThickness;
      contSize.descent = bmBase.ascent + bmBase.descent + psi;

      
      mMathMLChar[mLongDivCharIndex].Stretch(PresContext(), aRenderingContext,
                                             NS_STRETCH_DIRECTION_VERTICAL,
                                             contSize, bmLongdivChar,
                                             NS_STRETCH_LARGER);
      mMathMLChar[mLongDivCharIndex].GetBoundingMetrics(bmLongdivChar);

      
      dx_left = PR_MAX(dx_left, bmLongdivChar.width);

      
      longdivAscent = bmBase.ascent + psi + mRuleThickness;
      longdivDescent = PR_MAX(bmBase.descent,
                              (bmLongdivChar.ascent + bmLongdivChar.descent -
                               longdivAscent));

      mBoundingMetrics.ascent = PR_MAX(mBoundingMetrics.ascent,
                                       longdivAscent);
      mBoundingMetrics.descent = PR_MAX(mBoundingMetrics.descent,
                                        longdivDescent);
    }
  }

  
  
  if (IsToDraw(NOTATION_RADICAL)) {
    if (aWidthOnly) {
      nscoord radical_width = mMathMLChar[mRadicalCharIndex].
        GetMaxWidth(PresContext(), aRenderingContext);
      
      
      dx_left = PR_MAX(dx_left, radical_width);
    } else {
      
      
      nsBoundingMetrics contSize = bmBase;
      contSize.ascent = mRuleThickness;
      contSize.descent = bmBase.ascent + bmBase.descent + psi;

      
      mMathMLChar[mRadicalCharIndex].Stretch(PresContext(), aRenderingContext,
                                             NS_STRETCH_DIRECTION_VERTICAL,
                                             contSize, bmRadicalChar,
                                             NS_STRETCH_LARGER);
      mMathMLChar[mRadicalCharIndex].GetBoundingMetrics(bmRadicalChar);

      
      dx_left = PR_MAX(dx_left, bmRadicalChar.width);

      
      radicalAscent = bmBase.ascent + psi + mRuleThickness;
      radicalDescent = PR_MAX(bmBase.descent,
                              (bmRadicalChar.ascent + bmRadicalChar.descent -
                               radicalAscent));

      mBoundingMetrics.ascent = PR_MAX(mBoundingMetrics.ascent,
                                       radicalAscent);
      mBoundingMetrics.descent = PR_MAX(mBoundingMetrics.descent,
                                        radicalDescent);
    }
  }

  
  
  if (IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      (IsToDraw(NOTATION_LEFT) && IsToDraw(NOTATION_RIGHT))) {
    
    dx_left = dx_right = PR_MAX(dx_left, dx_right);
  }

  
  
  mBoundingMetrics.width = dx_left + bmBase.width + dx_right;

  mBoundingMetrics.leftBearing = PR_MIN(0, dx_left + bmBase.leftBearing);
  mBoundingMetrics.rightBearing =
    PR_MAX(mBoundingMetrics.width, dx_left + bmBase.rightBearing);
  
  aDesiredSize.width = mBoundingMetrics.width;

  aDesiredSize.ascent = PR_MAX(mBoundingMetrics.ascent, baseSize.ascent);
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(mBoundingMetrics.descent, baseSize.height - baseSize.ascent);

  if (IsToDraw(NOTATION_LONGDIV) || IsToDraw(NOTATION_RADICAL)) {
    
    
    
    nscoord leading = nscoord(0.2f * mEmHeight);
    nscoord desiredSizeAscent = aDesiredSize.ascent;
    nscoord desiredSizeDescent = aDesiredSize.height - aDesiredSize.ascent;
    
    if (IsToDraw(NOTATION_LONGDIV)) {
      desiredSizeAscent = PR_MAX(desiredSizeAscent,
                                 longdivAscent + leading);
      desiredSizeDescent = PR_MAX(desiredSizeDescent,
                                  longdivDescent + mRuleThickness);
    }
    
    if (IsToDraw(NOTATION_RADICAL)) {
      desiredSizeAscent = PR_MAX(desiredSizeAscent,
                                 radicalAscent + leading);
      desiredSizeDescent = PR_MAX(desiredSizeDescent,
                                  radicalDescent + mRuleThickness);
    }

    aDesiredSize.ascent = desiredSizeAscent;
    aDesiredSize.height = desiredSizeAscent + desiredSizeDescent;
  }
    
  if (IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      (IsToDraw(NOTATION_TOP) && IsToDraw(NOTATION_BOTTOM))) {
    
    nscoord dy = PR_MAX(aDesiredSize.ascent - bmBase.ascent,
                        aDesiredSize.height - aDesiredSize.ascent -
                        bmBase.descent);

    aDesiredSize.ascent = bmBase.ascent + dy;
    aDesiredSize.height = aDesiredSize.ascent + bmBase.descent + dy;
  }

  
  if (IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX))
    mBoundingMetrics.ascent = aDesiredSize.ascent;
  
  if (IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX))
    mBoundingMetrics.descent = aDesiredSize.height - aDesiredSize.ascent;

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  
  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    
    
    if (IsToDraw(NOTATION_LONGDIV))
      mMathMLChar[mLongDivCharIndex].SetRect(nsRect(dx_left -
                                                    bmLongdivChar.width,
                                                    aDesiredSize.ascent -
                                                    longdivAscent,
                                                    bmLongdivChar.width,
                                                    bmLongdivChar.ascent +
                                                    bmLongdivChar.descent));

    if (IsToDraw(NOTATION_RADICAL))
      mMathMLChar[mRadicalCharIndex].SetRect(nsRect(dx_left -
                                                    bmRadicalChar.width,
                                                    aDesiredSize.ascent -
                                                    radicalAscent,
                                                    bmRadicalChar.width,
                                                    bmRadicalChar.ascent +
                                                    bmRadicalChar.descent));

    mContentWidth = bmBase.width;

    
    
    PositionRowChildFrames(dx_left, aDesiredSize.ascent);
  }

  return NS_OK;
}

nscoord
nsMathMLmencloseFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap)
    return 0;

  
  nsRect rect;
  for (PRUint32 i = 0; i < mMathMLChar.Length(); i++) {
    mMathMLChar[i].GetRect(rect);
    rect.MoveBy(gap, 0);
    mMathMLChar[i].SetRect(rect);
  }

  return gap;
}

NS_IMETHODIMP
nsMathMLmencloseFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                        nsIAtom*        aAttribute,
                                        PRInt32         aModType)
{
  if (aAttribute == nsGkAtoms::notation_) {
    mNotationsToDraw = 0;
    mLongDivCharIndex = mRadicalCharIndex = -1;
    mMathMLChar.Clear();
    
    InitNotations();
  }

  return nsMathMLContainerFrame::
    AttributeChanged(aNameSpaceID, aAttribute, aModType);
}




nsStyleContext*
nsMathMLmencloseFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  PRInt32 len = mMathMLChar.Length();
  if (aIndex >= 0 && aIndex < len)
    return mMathMLChar[aIndex].GetStyleContext();
  else
    return nsnull;
}

void
nsMathMLmencloseFrame::SetAdditionalStyleContext(PRInt32          aIndex, 
                                                 nsStyleContext*  aStyleContext)
{
  PRInt32 len = mMathMLChar.Length();
  if (aIndex >= 0 && aIndex < len)
    mMathMLChar[aIndex].SetStyleContext(aStyleContext);
}

class nsDisplayNotation : public nsDisplayItem
{
public:
  nsDisplayNotation(nsIFrame* aFrame, const nsRect& aRect,
                    nscoord aThickness, nsMencloseNotation aType)
    : nsDisplayItem(aFrame), mRect(aRect), 
      mThickness(aThickness), mType(aType) {
    MOZ_COUNT_CTOR(nsDisplayNotation);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayNotation() {
    MOZ_COUNT_DTOR(nsDisplayNotation);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
                     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLMencloseNotation")

private:
  nsRect             mRect;
  nscoord            mThickness;
  nsMencloseNotation mType;
};

void nsDisplayNotation::Paint(nsDisplayListBuilder* aBuilder,
                              nsIRenderingContext* aCtx,
                              const nsRect& aDirtyRect)
{
  
  nsPresContext* presContext = mFrame->PresContext();
  gfxRect rect = presContext->
    AppUnitsToGfxUnits(mRect + aBuilder->ToReferenceFrame(mFrame));

  
  aCtx->SetColor(mFrame->GetStyleColor()->mColor);

  
  gfxContext *gfxCtx = aCtx->ThebesContext();
  gfxFloat currentLineWidth = gfxCtx->CurrentLineWidth();
  gfxFloat e = presContext->AppUnitsToGfxUnits(mThickness);
  gfxCtx->SetLineWidth(e);

  rect.Inset(e / 2.0);

  gfxCtx->NewPath();

  switch(mType)
    {
    case NOTATION_CIRCLE:
      gfxCtx->Ellipse(rect.pos + rect.size / 2.0, rect.size);
      break;

    case NOTATION_ROUNDEDBOX:
      gfxCtx->RoundedRectangle(rect, gfxCornerSizes(3 * e), PR_TRUE);
      break;

    case NOTATION_UPDIAGONALSTRIKE:
      gfxCtx->Line(rect.BottomLeft(), rect.TopRight());
      break;

    case NOTATION_DOWNDIAGONALSTRIKE:
      gfxCtx->Line(rect.TopLeft(), rect.BottomRight());
      break;

    default:
      NS_NOTREACHED("This notation can not be drawn using nsDisplayNotation");
      break;
    }

  gfxCtx->Stroke();

  
  gfxCtx->SetLineWidth(currentLineWidth);
}

nsresult
nsMathMLmencloseFrame::DisplayNotation(nsDisplayListBuilder* aBuilder,
                                       nsIFrame* aFrame, const nsRect& aRect,
                                       const nsDisplayListSet& aLists,
                                       nscoord aThickness,
                                       nsMencloseNotation aType)
{
  if (!aFrame->GetStyleVisibility()->IsVisible() || aRect.IsEmpty() ||
      aThickness <= 0)
    return NS_OK;

  return aLists.Content()->AppendNewToTop(new (aBuilder)
                                          nsDisplayNotation(aFrame, aRect,
                                                            aThickness,
                                                            aType));
}
