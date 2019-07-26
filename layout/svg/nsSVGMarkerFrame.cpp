





#include "nsSVGMarkerFrame.h"


#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGMarkerElement.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGPathGeometryFrame.h"

using namespace mozilla::dom;
using namespace mozilla::gfx;

nsIFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMarkerFrame)




NS_IMETHODIMP
nsSVGMarkerFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
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
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGMarkerFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}

#ifdef DEBUG
void
nsSVGMarkerFrame::Init(nsIContent* aContent,
                       nsIFrame* aParent,
                       nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::marker), "Content is not an SVG marker");

  nsSVGMarkerFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMarkerFrame::GetType() const
{
  return nsGkAtoms::svgMarkerFrame;
}




gfxMatrix
nsSVGMarkerFrame::GetCanvasTM(uint32_t aFor, nsIFrame* aTransformRoot)
{
  NS_ASSERTION(mMarkedFrame, "null nsSVGPathGeometry frame");

  if (mInUse2) {
    
    return gfxMatrix();
  }

  SVGMarkerElement *content = static_cast<SVGMarkerElement*>(mContent);
  
  mInUse2 = true;
  gfxMatrix markedTM = mMarkedFrame->GetCanvasTM(aFor, aTransformRoot);
  mInUse2 = false;

  Matrix markerTM = content->GetMarkerTransform(mStrokeWidth, mX, mY,
                                                mAutoAngle, mIsStart);
  gfxMatrix viewBoxTM = content->GetViewBoxTransform();

  return viewBoxTM * ThebesMatrix(markerTM) * markedTM;
}

static nsIFrame*
GetAnonymousChildFrame(nsIFrame* aFrame)
{
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
  MOZ_ASSERT(kid && kid->GetType() == nsGkAtoms::svgMarkerAnonChildFrame,
             "expected to find anonymous child of marker frame");
  return kid;
}

nsresult
nsSVGMarkerFrame::PaintMark(nsRenderingContext *aContext,
                            nsSVGPathGeometryFrame *aMarkedFrame,
                            nsSVGMark *aMark, float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return NS_OK;

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  SVGMarkerElement *marker = static_cast<SVGMarkerElement*>(mContent);

  const nsSVGViewBoxRect viewBox = marker->GetViewBoxRect();

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    
    return NS_OK;
  }

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAutoAngle = aMark->angle;
  mIsStart = aMark->type == nsSVGMark::eStart;

  gfxContext *gfx = aContext->ThebesContext();

  if (StyleDisplay()->IsScrollableOverflow()) {
    gfx->Save();
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, viewBox.x, viewBox.y,
                                      viewBox.width, viewBox.height);
    nsSVGUtils::SetClipRect(gfx, GetCanvasTM(nsISVGChildFrame::FOR_PAINTING),
                            clipRect);
  }

  nsIFrame* kid = GetAnonymousChildFrame(this);
  nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
  
  SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);
  nsSVGUtils::PaintFrameWithEffects(aContext, nullptr, kid);

  if (StyleDisplay()->IsScrollableOverflow())
    gfx->Restore();

  return NS_OK;
}

SVGBBox
nsSVGMarkerFrame::GetMarkBBoxContribution(const Matrix &aToBBoxUserspace,
                                          uint32_t aFlags,
                                          nsSVGPathGeometryFrame *aMarkedFrame,
                                          const nsSVGMark *aMark,
                                          float aStrokeWidth)
{
  SVGBBox bbox;

  
  
  
  if (mInUse)
    return bbox;

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  SVGMarkerElement *content = static_cast<SVGMarkerElement*>(mContent);

  const nsSVGViewBoxRect viewBox = content->GetViewBoxRect();

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    return bbox;
  }

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAutoAngle = aMark->angle;
  mIsStart = aMark->type == nsSVGMark::eStart;

  Matrix markerTM =
    content->GetMarkerTransform(mStrokeWidth, mX, mY, mAutoAngle, mIsStart);
  gfxMatrix viewBoxTM = content->GetViewBoxTransform();

  Matrix tm = ToMatrix(viewBoxTM) * markerTM * aToBBoxUserspace;

  nsISVGChildFrame* child = do_QueryFrame(GetAnonymousChildFrame(this));
  
  
  

  
  
  bbox.UnionEdges(child->GetBBoxContribution(tm, aFlags));

  return bbox;
}

void
nsSVGMarkerFrame::SetParentCoordCtxProvider(SVGSVGElement *aContext)
{
  SVGMarkerElement *marker = static_cast<SVGMarkerElement*>(mContent);
  marker->SetParentCoordCtxProvider(aContext);
}




nsSVGMarkerFrame::AutoMarkerReferencer::AutoMarkerReferencer(
    nsSVGMarkerFrame *aFrame,
    nsSVGPathGeometryFrame *aMarkedFrame
    MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
      : mFrame(aFrame)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  mFrame->mInUse = true;
  mFrame->mMarkedFrame = aMarkedFrame;

  SVGSVGElement *ctx =
    static_cast<nsSVGElement*>(aMarkedFrame->GetContent())->GetCtx();
  mFrame->SetParentCoordCtxProvider(ctx);
}

nsSVGMarkerFrame::AutoMarkerReferencer::~AutoMarkerReferencer()
{
  mFrame->SetParentCoordCtxProvider(nullptr);

  mFrame->mMarkedFrame = nullptr;
  mFrame->mInUse = false;
}




nsIFrame*
NS_NewSVGMarkerAnonChildFrame(nsIPresShell* aPresShell,
                              nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerAnonChildFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMarkerAnonChildFrame)

#ifdef DEBUG
void
nsSVGMarkerAnonChildFrame::Init(nsIContent* aContent,
                                nsIFrame* aParent,
                                nsIFrame* aPrevInFlow)
{
  NS_ABORT_IF_FALSE(aParent->GetType() == nsGkAtoms::svgMarkerFrame,
                    "Unexpected parent");
  nsSVGMarkerAnonChildFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif

nsIAtom *
nsSVGMarkerAnonChildFrame::GetType() const
{
  return nsGkAtoms::svgMarkerAnonChildFrame;
}
