




#include "nsMathMLmrowFrame.h"
#include "mozilla/gfx/2D.h"





nsIFrame*
NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmrowFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmrowFrame)

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

nsresult
nsMathMLmrowFrame::AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType)
{
  
  
  
  if (mContent->IsMathMLElement(nsGkAtoms::mtable_)) {
    nsIFrame* frame = mFrames.FirstChild();
    for ( ; frame; frame = frame->GetFirstPrincipalChild()) {
      
      if (frame->GetType() == nsGkAtoms::tableOuterFrame)
        return frame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    }
    NS_NOTREACHED("mtable wrapper without the real table frame");
  }

  return nsMathMLContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

 eMathMLFrameType
nsMathMLmrowFrame::GetMathMLFrameType()
{
  if (!IsMrowLike()) {
    nsIMathMLFrame* child = do_QueryFrame(mFrames.FirstChild());
    if (child) {
      
      
      return child->GetMathMLFrameType();
    }
  }
  return nsMathMLFrame::GetMathMLFrameType();
}
