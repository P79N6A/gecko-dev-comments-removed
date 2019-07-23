







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmrowFrame.h"





nsIFrame*
NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmrowFrame(aContext);
}

nsMathMLmrowFrame::~nsMathMLmrowFrame()
{
}

NS_IMETHODIMP
nsMathMLmrowFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmrowFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    PRInt32  aModType)
{
  
  
  
  if (mContent->Tag() == nsGkAtoms::mtable_) {
    nsIFrame* frame = mFrames.FirstChild();
    for ( ; frame; frame = frame->GetFirstChild(nsnull)) {
      
      if (frame->GetType() == nsGkAtoms::tableOuterFrame)
        return frame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    }
    NS_NOTREACHED("mtable wrapper without the real table frame");
  }

  return nsMathMLContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

nsIFrame*
nsMathMLmrowFrame::GetContentInsertionFrame()
{
  
  
  
  
  if (mContent->Tag() == nsGkAtoms::mtable_) {
    nsIFrame* frame = mFrames.FirstChild();
    for ( ; frame; frame = frame->GetFirstChild(nsnull)) {
      
      if (frame->GetType() == nsGkAtoms::tableOuterFrame)
        return frame->GetContentInsertionFrame();
    }
    NS_NOTREACHED("mtable wrapper without the real table frame");
  }

  return nsMathMLContainerFrame::GetContentInsertionFrame();
}
