





































#include "nsSVGGFrame.h"
#include "nsSVGSwitchElement.h"
#include "nsIDOMSVGRect.h"

typedef nsSVGGFrame nsSVGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGSwitchFrameBase(aContext) {}

public:
  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGSwitch"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float aX, float aY, nsIFrame** aResult);  
  NS_IMETHODIMP_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD GetBBox(nsIDOMSVGRect **aRect);

private:
  nsIFrame *GetActiveChildFrame();
};




nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{  
  nsCOMPtr<nsIDOMSVGSwitchElement> svgSwitch = do_QueryInterface(aContent);
  if (!svgSwitch) {
    NS_ERROR("Can't create frame. Content is not an SVG switch\n");
    return nsnull;
  }

  return new (aPresShell) nsSVGSwitchFrame(aContext);
}

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

NS_IMETHODIMP
nsSVGSwitchFrame::PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect)
{
  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsSVGUtils::PaintChildWithEffects(aContext, aDirtyRect, kid);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSwitchFrame::GetFrameForPointSVG(float aX, float aY, nsIFrame** aResult)
{
  *aResult = nsnull;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame;
    CallQueryInterface(kid, &svgFrame);
    if (svgFrame) {
      svgFrame->GetFrameForPointSVG(aX, aY, aResult);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsRect)
nsSVGSwitchFrame::GetCoveredRegion()
{
  nsRect rect;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* child = nsnull;
    CallQueryInterface(kid, &child);
    if (child) {
      rect = child->GetCoveredRegion();
    }
  }
  return rect;
}

NS_IMETHODIMP
nsSVGSwitchFrame::UpdateCoveredRegion()
{
  static_cast<nsSVGSwitchElement*>(mContent)->UpdateActiveChild();

  return nsSVGSwitchFrameBase::UpdateCoveredRegion();
}

NS_IMETHODIMP
nsSVGSwitchFrame::InitialUpdate()
{
  nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::InitialUpdate();
}

NS_IMETHODIMP
nsSVGSwitchFrame::NotifyRedrawUnsuspended()
{
  if (GetStateBits() & NS_STATE_SVG_DIRTY)
    nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::NotifyRedrawUnsuspended();
}

NS_IMETHODIMP
nsSVGSwitchFrame::GetBBox(nsIDOMSVGRect **aRect)
{
  *aRect = nsnull;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame = nsnull;
    CallQueryInterface(kid, &svgFrame);
    if (svgFrame) {
      nsCOMPtr<nsIDOMSVGRect> box;
      svgFrame->GetBBox(getter_AddRefs(box));
      if (box) {
        box.swap(*aRect);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}

nsIFrame *
nsSVGSwitchFrame::GetActiveChildFrame()
{
  nsIContent *activeChild =
    static_cast<nsSVGSwitchElement*>(mContent)->GetActiveChild();

  if (activeChild) {
    for (nsIFrame* kid = mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {

      if (activeChild == kid->GetContent()) {
        return kid;
      }
    }
  }
  return nsnull;
}
