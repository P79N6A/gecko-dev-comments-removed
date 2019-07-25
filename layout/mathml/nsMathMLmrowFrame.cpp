





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"

#include "nsMathMLmrowFrame.h"





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

  if (mContent->Tag() == nsGkAtoms::mrow_) {
    
    nsMathMLFrame::FindAttrDirectionality(mContent, mPresentationData);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmrowFrame::AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType)
{
  
  
  
  if (mContent->Tag() == nsGkAtoms::mtable_) {
    nsIFrame* frame = mFrames.FirstChild();
    for ( ; frame; frame = frame->GetFirstPrincipalChild()) {
      
      if (frame->GetType() == nsGkAtoms::tableOuterFrame)
        return frame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    }
    NS_NOTREACHED("mtable wrapper without the real table frame");
  }

  return nsMathMLContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}
