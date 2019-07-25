



































#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGOuterSVGFrame.h"

NS_QUERYFRAME_HEAD(nsSVGContainerFrame)
  NS_QUERYFRAME_ENTRY(nsSVGContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGContainerFrameBase)

NS_QUERYFRAME_HEAD(nsSVGDisplayContainerFrame)
  NS_QUERYFRAME_ENTRY(nsSVGDisplayContainerFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGContainerFrame)

nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                        nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGContainerFrame)
NS_IMPL_FRAMEARENA_HELPERS(nsSVGDisplayContainerFrame)

NS_IMETHODIMP
nsSVGContainerFrame::AppendFrames(ChildListID  aListID,
                                  nsFrameList& aFrameList)
{
  return InsertFrames(aListID, mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsSVGContainerFrame::InsertFrames(ChildListID aListID,
                                  nsIFrame* aPrevFrame,
                                  nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  mFrames.InsertFrames(this, aPrevFrame, aFrameList);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGContainerFrame::RemoveFrame(ChildListID aListID,
                                 nsIFrame* aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  mFrames.DestroyFrame(aOldFrame);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGContainerFrame::Init(nsIContent* aContent,
                          nsIFrame* aParent,
                          nsIFrame* aPrevInFlow)
{
  AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  nsresult rv = nsSVGContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::Init(nsIContent* aContent,
                                 nsIFrame* aParent,
                                 nsIFrame* aPrevInFlow)
{
  if (!(GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
    AddStateBits(aParent->GetStateBits() &
      (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_CLIPPATH_CHILD |
       NS_STATE_SVG_REDRAW_SUSPENDED));
  }
  nsresult rv = nsSVGContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InsertFrames(ChildListID aListID,
                                         nsIFrame* aPrevFrame,
                                         nsFrameList& aFrameList)
{
  
  
  
  nsIFrame* firstOldFrame = aPrevFrame ?
    aPrevFrame->GetNextSibling() : GetChildList(aListID).FirstChild();
  nsIFrame* firstNewFrame = aFrameList.FirstChild();
  
  
  nsSVGContainerFrame::InsertFrames(aListID, aPrevFrame, aFrameList);

  
  
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    for (nsIFrame* kid = firstNewFrame; kid != firstOldFrame;
         kid = kid->GetNextSibling()) {
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame) {
        SVGFrame->InitialUpdate(); 
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::RemoveFrame(ChildListID aListID,
                                        nsIFrame* aOldFrame)
{
  
  RemoveStateBits(NS_STATE_SVG_REDRAW_SUSPENDED);

  nsSVGUtils::InvalidateCoveredRegion(aOldFrame);

  nsresult rv = nsSVGContainerFrame::RemoveFrame(aListID, aOldFrame);

  if (!(GetStateBits() & (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_IS_OUTER_SVG))) {
    nsSVGUtils::NotifyAncestorsOfFilterRegionChange(this);
  }

  return rv;
}




NS_IMETHODIMP
nsSVGDisplayContainerFrame::PaintSVG(nsSVGRenderState* aContext,
                                     const nsIntRect *aDirtyRect)
{
  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintFrameWithEffects(aContext, aDirtyRect, kid);
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGDisplayContainerFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  return nsSVGUtils::HitTestChildren(this, aPoint);
}

NS_IMETHODIMP_(nsRect)
nsSVGDisplayContainerFrame::GetCoveredRegion()
{
  return nsSVGUtils::GetCoveredRegion(mFrames);
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::UpdateCoveredRegion()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->UpdateCoveredRegion();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InitialUpdate()
{
  NS_ASSERTION(GetStateBits() & NS_FRAME_FIRST_REFLOW,
               "Yikes! We've been called already! Hopefully we weren't called "
               "before our nsSVGOuterSVGFrame's initial Reflow()!!!");

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->InitialUpdate();
    }
  }

  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "We don't actually participate in reflow");
  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
  
  return NS_OK;
}  

void
nsSVGDisplayContainerFrame::NotifySVGChanged(PRUint32 aFlags)
{
  NS_ASSERTION(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
               "Invalidation logic may need adjusting");

  nsSVGUtils::NotifyChildrenOfSVGChange(this, aFlags);
}

void
nsSVGDisplayContainerFrame::NotifyRedrawSuspended()
{
  nsSVGUtils::NotifyRedrawSuspended(this);
}

void
nsSVGDisplayContainerFrame::NotifyRedrawUnsuspended()
{
  nsSVGUtils::NotifyRedrawUnsuspended(this);
}

gfxRect
nsSVGDisplayContainerFrame::GetBBoxContribution(
  const gfxMatrix &aToBBoxUserspace,
  PRUint32 aFlags)
{
  gfxRect bboxUnion;
  bool firstChild = true;

  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* svgKid = do_QueryFrame(kid);
    if (svgKid) {
      gfxMatrix transform = aToBBoxUserspace;
      nsIContent *content = kid->GetContent();
      if (content->IsSVG() && !content->IsNodeOfType(nsINode::eTEXT)) {
        transform = static_cast<nsSVGElement*>(content)->
                      PrependLocalTransformTo(aToBBoxUserspace);
      }
      
      
      
      gfxRect childBBox = svgKid->GetBBoxContribution(transform, aFlags);
      if (firstChild) {
        bboxUnion = childBBox;
        firstChild = false;
        continue;
      }
      bboxUnion = bboxUnion.UnionEdges(childBBox);
    }
    kid = kid->GetNextSibling();
  }

  return bboxUnion;
}
