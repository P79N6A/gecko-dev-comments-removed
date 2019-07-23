








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmsqrtFrame.h"















#define NS_SQR_CHAR_STYLE_CONTEXT_INDEX   0

static const PRUnichar kSqrChar = PRUnichar(0x221A);

nsIFrame*
NS_NewMathMLmsqrtFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmsqrtFrame(aContext);
}

nsMathMLmsqrtFrame::nsMathMLmsqrtFrame(nsStyleContext* aContext) :
  nsMathMLContainerFrame(aContext),
  mSqrChar(),
  mBarRect()
{
}

nsMathMLmsqrtFrame::~nsMathMLmsqrtFrame()
{
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
                                             
  nsPresContext *presContext = PresContext();

  
  
  
  nsAutoString sqrChar; sqrChar.Assign(kSqrChar);
  mSqrChar.SetData(presContext, sqrChar);
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mSqrChar, PR_TRUE);

  return rv;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}


NS_IMETHODIMP
nsMathMLmsqrtFrame::TransmitAutomaticData()
{
  
  
  
  
  UpdatePresentationDataFromChildAt(0, -1, 0,
     NS_MATHML_COMPRESSED,
     NS_MATHML_COMPRESSED);

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  nsresult rv = nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    rv = mSqrChar.Display(aBuilder, this, aLists);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DisplayBar(aBuilder, this, mBarRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    nsRect rect;
    mSqrChar.GetRect(rect);
    nsBoundingMetrics bm;
    mSqrChar.GetBoundingMetrics(bm);
    rv = DisplayBoundingMetrics(aBuilder, this, rect.TopLeft(), bm, aLists);
#endif
  }

  return rv;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  
  
  nsHTMLReflowMetrics baseSize(aDesiredSize);
  nsresult rv = nsMathMLContainerFrame::Reflow(aPresContext, baseSize,
                                               aReflowState, aStatus);
  
  if (NS_FAILED(rv)) return rv;

  nsBoundingMetrics bmSqr, bmBase;
  bmBase = baseSize.mBoundingMetrics;

  
  

  nsIRenderingContext& renderingContext = *aReflowState.rendContext;
  renderingContext.SetFont(GetStyleFont()->mFont, nsnull);
  nsCOMPtr<nsIFontMetrics> fm;
  renderingContext.GetFontMetrics(*getter_AddRefs(fm));

  nscoord ruleThickness, leading, em;
  GetRuleThickness(renderingContext, fm, ruleThickness);

  nsBoundingMetrics bmOne;
  renderingContext.GetBoundingMetrics(NS_LITERAL_STRING("1").get(), 1, bmOne);

  
  
  GetEmHeight(fm, em);
  leading = nscoord(0.2f * em); 

  
  
  nscoord phi = 0, psi = 0;
  if (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
    fm->GetXHeight(phi);
  else
    phi = ruleThickness;
  psi = ruleThickness + phi/4;

  
  if (bmOne.ascent > bmBase.ascent)
    psi += bmOne.ascent - bmBase.ascent;

  
  nsBoundingMetrics contSize = bmBase;
  contSize.ascent = ruleThickness;
  contSize.descent = bmBase.ascent + bmBase.descent + psi;

  
  nsBoundingMetrics radicalSize;
  mSqrChar.Stretch(aPresContext, renderingContext,
                   NS_STRETCH_DIRECTION_VERTICAL, 
                   contSize, radicalSize,
                   NS_STRETCH_LARGER);
  
  
  mSqrChar.GetBoundingMetrics(bmSqr);

  
  
  ruleThickness = bmSqr.ascent;
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  if (ruleThickness < onePixel) {
    ruleThickness = onePixel;
  }

  
  
  nscoord delta = psi % onePixel;
  if (delta)
    psi += onePixel - delta; 

  nscoord dx = 0, dy = 0;
  
  dy = leading; 
  mSqrChar.SetRect(nsRect(dx, dy, bmSqr.width, bmSqr.ascent + bmSqr.descent));
  dx = bmSqr.width;
  mBarRect.SetRect(dx, dy, bmBase.width, ruleThickness);

  
  
  mBoundingMetrics.ascent = bmBase.ascent + psi + ruleThickness;
  mBoundingMetrics.descent = 
    PR_MAX(bmBase.descent, (bmSqr.descent - (bmBase.ascent + psi)));
  mBoundingMetrics.width = bmSqr.width + bmBase.width;
  mBoundingMetrics.leftBearing = bmSqr.leftBearing;
  mBoundingMetrics.rightBearing = bmSqr.width + 
    PR_MAX(bmBase.width, bmBase.rightBearing); 

  aDesiredSize.ascent = mBoundingMetrics.ascent + leading;
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(baseSize.height - baseSize.ascent,
           mBoundingMetrics.descent + ruleThickness);
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  

  dx = radicalSize.width;
  dy = aDesiredSize.ascent - baseSize.ascent;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    childFrame->SetPosition(childFrame->GetPosition() + nsPoint(dx, dy));
    childFrame = childFrame->GetNextSibling();
  }

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

nscoord
nsMathMLmsqrtFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap) return 0;

  nsRect rect;
  mSqrChar.GetRect(rect);
  rect.MoveBy(gap, 0);
  mSqrChar.SetRect(rect);
  mBarRect.MoveBy(gap, 0);
  return gap;
}



nsStyleContext*
nsMathMLmsqrtFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  switch (aIndex) {
  case NS_SQR_CHAR_STYLE_CONTEXT_INDEX:
    return mSqrChar.GetStyleContext();
    break;
  default:
    return nsnull;
  }
}

void
nsMathMLmsqrtFrame::SetAdditionalStyleContext(PRInt32          aIndex, 
                                              nsStyleContext*  aStyleContext)
{
  switch (aIndex) {
  case NS_SQR_CHAR_STYLE_CONTEXT_INDEX:
    mSqrChar.SetStyleContext(aStyleContext);
    break;
  }
}
