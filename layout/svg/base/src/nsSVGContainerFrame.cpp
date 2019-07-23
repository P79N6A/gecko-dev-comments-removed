



































#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGOuterSVGFrame.h"




NS_INTERFACE_MAP_BEGIN(nsSVGDisplayContainerFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGChildFrame)
NS_INTERFACE_MAP_END_INHERITING(nsSVGContainerFrame)

nsIFrame*
NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                        nsIContent* aContent,
                        nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGContainerFrame(aContext);
}

NS_IMETHODIMP
nsSVGContainerFrame::AppendFrames(nsIAtom* aListName,
                                  nsIFrame* aFrameList)
{
  return InsertFrames(aListName, mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsSVGContainerFrame::InsertFrames(nsIAtom* aListName,
                                  nsIFrame* aPrevFrame,
                                  nsIFrame* aFrameList)
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
nsSVGContainerFrame::InitSVG()
{
  AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGContainerFrame::Init(nsIContent* aContent,
			  nsIFrame* aParent,
			  nsIFrame* aPrevInFlow)
{
  nsresult rv = nsSVGContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
  InitSVG();
  return rv;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InitSVG()
{
  AddStateBits(mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD);
  return NS_OK;
}

void
nsSVGDisplayContainerFrame::Destroy()
{
  nsSVGUtils::StyleEffects(this);
  nsSVGContainerFrame::Destroy();
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InsertFrames(nsIAtom* aListName,
                                         nsIFrame* aPrevFrame,
                                         nsIFrame* aFrameList)
{
  
  nsIFrame* lastNewFrame = nsnull;
  {
    nsFrameList tmpList(aFrameList);
    lastNewFrame = tmpList.LastChild();
  }
  
  
  nsSVGContainerFrame::InsertFrames(aListName, aPrevFrame, aFrameList);

  
  nsIFrame* end = nsnull;
  if (lastNewFrame)
    end = lastNewFrame->GetNextSibling();
  
  for (nsIFrame* kid = aFrameList; kid != end;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->InitialUpdate(); 
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::RemoveFrame(nsIAtom* aListName,
                                        nsIFrame* aOldFrame)
{
  nsRect dirtyRect;
  
  nsISVGChildFrame* SVGFrame = nsnull;
  CallQueryInterface(aOldFrame, &SVGFrame);

  if (SVGFrame && !(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    dirtyRect = SVGFrame->GetCoveredRegion();

  nsresult rv = nsSVGContainerFrame::RemoveFrame(aListName, aOldFrame);

  nsSVGOuterSVGFrame* outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  NS_ASSERTION(outerSVGFrame, "no outer svg frame");
  if (outerSVGFrame)
    outerSVGFrame->InvalidateRect(dirtyRect);

  return rv;
}




NS_IMETHODIMP
nsSVGDisplayContainerFrame::PaintSVG(nsSVGRenderState* aContext,
                                     nsRect *aDirtyRect)
{
  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintChildWithEffects(aContext, aDirtyRect, kid);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  nsSVGUtils::HitTestChildren(this, x, y, hit);
  
  return NS_OK;
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
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->UpdateCoveredRegion();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::InitialUpdate()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
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

NS_IMETHODIMP
nsSVGDisplayContainerFrame::NotifyCanvasTMChanged(PRBool suppressInvalidation)
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyCanvasTMChanged(suppressInvalidation);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::NotifyRedrawSuspended()
{
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    CallQueryInterface(kid, &SVGFrame);
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
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGDisplayContainerFrame::GetBBox(nsIDOMSVGRect **_retval)
{
  return nsSVGUtils::GetBBox(&mFrames, _retval);
}
