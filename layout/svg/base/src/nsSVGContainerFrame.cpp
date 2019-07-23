



































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
nsSVGContainerFrame::AppendFrames(nsIAtom* aListName,
                                  nsFrameList& aFrameList)
{
  return InsertFrames(aListName, mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsSVGContainerFrame::InsertFrames(nsIAtom* aListName,
                                  nsIFrame* aPrevFrame,
                                  nsFrameList& aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  mFrames.InsertFrames(this, aPrevFrame, aFrameList);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGContainerFrame::RemoveFrame(nsIAtom* aListName,
                                 nsIFrame* aOldFrame)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  return mFrames.DestroyFrame(aOldFrame) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSVGContainerFrame::Init(nsIContent* aContent,
                          nsIFrame* aParent,
                          nsIFrame* aPrevInFlow)
{
  AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_PROPAGATE_TRANSFORM);
  nsresult rv = nsSVGContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::Init(nsIContent* aContent,
                                 nsIFrame* aParent,
                                 nsIFrame* aPrevInFlow)
{
  AddStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  if (!(GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
    AddStateBits(aParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD);
  }
  nsresult rv = nsSVGContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InsertFrames(nsIAtom* aListName,
                                         nsIFrame* aPrevFrame,
                                         nsFrameList& aFrameList)
{
  
  
  
  nsIFrame* firstOldFrame = aPrevFrame ?
    aPrevFrame->GetNextSibling() : GetChildList(aListName).FirstChild();
  nsIFrame* firstNewFrame = aFrameList.FirstChild();
  
  
  nsSVGContainerFrame::InsertFrames(aListName, aPrevFrame, aFrameList);

  
  
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
nsSVGDisplayContainerFrame::RemoveFrame(nsIAtom* aListName,
                                        nsIFrame* aOldFrame)
{
  nsSVGUtils::InvalidateCoveredRegion(aOldFrame);

  nsresult rv = nsSVGContainerFrame::RemoveFrame(aListName, aOldFrame);

  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
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

NS_IMETHODIMP
nsSVGDisplayContainerFrame::NotifyRedrawSuspended()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawSuspended();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::NotifyRedrawUnsuspended()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
  }
  return NS_OK;
}

gfxRect
nsSVGDisplayContainerFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace)
{
  gfxRect bboxUnion(0.0, 0.0, 0.0, 0.0);

  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* svgKid = do_QueryFrame(kid);
    if (svgKid) {
      gfxMatrix transform = aToBBoxUserspace;
      
      if (kid->GetType() != nsGkAtoms::svgGlyphFrame) {
        nsIContent *content = kid->GetContent();
        if (content->IsNodeOfType(nsINode::eSVG)) {
          transform = static_cast<nsSVGElement*>(content)->
                        PrependLocalTransformTo(aToBBoxUserspace);
        }
      }
      bboxUnion = bboxUnion.Union(svgKid->GetBBoxContribution(transform));
    }
    kid = kid->GetNextSibling();
  }

  return bboxUnion;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::SetMatrixPropagation(PRBool aPropagate)
{
  if (aPropagate) {
    AddStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  } else {
    RemoveStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  }
  return NS_OK;
}

PRBool
nsSVGDisplayContainerFrame::GetMatrixPropagation()
{
  return (GetStateBits() & NS_STATE_SVG_PROPAGATE_TRANSFORM) != 0;
}
