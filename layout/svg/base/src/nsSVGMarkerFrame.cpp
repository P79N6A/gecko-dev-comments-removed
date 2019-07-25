



































#include "nsSVGMarkerFrame.h"

#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGRect.h"
#include "nsIDocument.h"
#include "nsRenderingContext.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsSVGEffects.h"
#include "nsSVGMarkerElement.h"
#include "nsSVGPathGeometryElement.h"
#include "gfxContext.h"

nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMarkerFrame)




NS_IMETHODIMP
nsSVGMarkerFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::markerUnits ||
       aAttribute == nsGkAtoms::refX ||
       aAttribute == nsGkAtoms::refY ||
       aAttribute == nsGkAtoms::markerWidth ||
       aAttribute == nsGkAtoms::markerHeight ||
       aAttribute == nsGkAtoms::orient ||
       aAttribute == nsGkAtoms::preserveAspectRatio ||
       aAttribute == nsGkAtoms::viewBox)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGMarkerFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGMarkerFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGMarkerElement> marker = do_QueryInterface(aContent);
  NS_ASSERTION(marker, "Content is not an SVG marker");

  return nsSVGMarkerFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMarkerFrame::GetType() const
{
  return nsGkAtoms::svgMarkerFrame;
}




gfxMatrix
nsSVGMarkerFrame::GetCanvasTM()
{
  NS_ASSERTION(mMarkedFrame, "null nsSVGPathGeometry frame");

  if (mInUse2) {
    
    return gfxMatrix();
  }

  nsSVGMarkerElement *content = static_cast<nsSVGMarkerElement*>(mContent);
  
  mInUse2 = true;
  gfxMatrix markedTM = mMarkedFrame->GetCanvasTM();
  mInUse2 = false;

  gfxMatrix markerTM = content->GetMarkerTransform(mStrokeWidth, mX, mY, mAutoAngle);
  gfxMatrix viewBoxTM = content->GetViewBoxTransform();

  return viewBoxTM * markerTM * markedTM;
}


nsresult
nsSVGMarkerFrame::PaintMark(nsRenderingContext *aContext,
                            nsSVGPathGeometryFrame *aMarkedFrame,
                            nsSVGMark *aMark, float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return NS_OK;

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  nsSVGMarkerElement *marker = static_cast<nsSVGMarkerElement*>(mContent);

  const nsSVGViewBoxRect viewBox = marker->GetViewBoxRect();

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    
    return NS_OK;
  }

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAutoAngle = aMark->angle;

  gfxContext *gfx = aContext->ThebesContext();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    gfx->Save();
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, viewBox.x, viewBox.y,
                                      viewBox.width, viewBox.height);
    nsSVGUtils::SetClipRect(gfx, GetCanvasTM(), clipRect);
  }

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      
      SVGFrame->NotifySVGChanged(
                          nsISVGChildFrame::DO_NOT_NOTIFY_RENDERING_OBSERVERS |
                          nsISVGChildFrame::TRANSFORM_CHANGED);
      nsSVGUtils::PaintFrameWithEffects(aContext, nsnull, kid);
    }
  }

  if (GetStyleDisplay()->IsScrollableOverflow())
    gfx->Restore();

  return NS_OK;
}

gfxRect
nsSVGMarkerFrame::GetMarkBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                          PRUint32 aFlags,
                                          nsSVGPathGeometryFrame *aMarkedFrame,
                                          const nsSVGMark *aMark,
                                          float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return gfxRect();

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  nsSVGMarkerElement *content = static_cast<nsSVGMarkerElement*>(mContent);

  const nsSVGViewBoxRect viewBox = content->GetViewBoxRect();

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    return gfxRect();
  }

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAutoAngle = aMark->angle;

  gfxMatrix markerTM =
    content->GetMarkerTransform(mStrokeWidth, mX, mY, mAutoAngle);
  gfxMatrix viewBoxTM = content->GetViewBoxTransform();

  gfxMatrix tm = viewBoxTM * markerTM * aToBBoxUserspace;

  gfxRect bbox;
  bool firstChild = true;

  for (nsIFrame* kid = mFrames.FirstChild();
       kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* child = do_QueryFrame(kid);
    if (child) {
      
      
      

      
      
      
      gfxRect childBBox = child->GetBBoxContribution(tm, aFlags);
      if (firstChild && (childBBox.Width() > 0 || childBBox.Height() > 0)) {
        bbox = childBBox;
        firstChild = false;
        continue;
      }
      bbox = bbox.UnionEdges(childBBox);
    }
  }

  return bbox;
}

void
nsSVGMarkerFrame::SetParentCoordCtxProvider(nsSVGSVGElement *aContext)
{
  nsSVGMarkerElement *marker = static_cast<nsSVGMarkerElement*>(mContent);
  marker->SetParentCoordCtxProvider(aContext);
}




nsSVGMarkerFrame::AutoMarkerReferencer::AutoMarkerReferencer(
    nsSVGMarkerFrame *aFrame,
    nsSVGPathGeometryFrame *aMarkedFrame)
      : mFrame(aFrame)
{
  mFrame->mInUse = true;
  mFrame->mMarkedFrame = aMarkedFrame;

  nsSVGSVGElement *ctx =
    static_cast<nsSVGElement*>(aMarkedFrame->GetContent())->GetCtx();
  mFrame->SetParentCoordCtxProvider(ctx);
}

nsSVGMarkerFrame::AutoMarkerReferencer::~AutoMarkerReferencer()
{
  mFrame->SetParentCoordCtxProvider(nsnull);

  mFrame->mMarkedFrame = nsnull;
  mFrame->mInUse = false;
}
