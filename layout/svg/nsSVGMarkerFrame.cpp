





#include "nsSVGMarkerFrame.h"


#include "gfxContext.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGMarkerElement.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGPathGeometryFrame.h"

using namespace mozilla::dom;
using namespace mozilla::gfx;

nsContainerFrame*
NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMarkerFrame)




nsresult
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
nsSVGMarkerFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::marker), "Content is not an SVG marker");

  nsSVGMarkerFrameBase::Init(aContent, aParent, aPrevInFlow);
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

  SVGMarkerElement *content = static_cast<SVGMarkerElement*>(mContent);
  
  mInUse2 = true;
  gfxMatrix markedTM = mMarkedFrame->GetCanvasTM();
  mInUse2 = false;

  Matrix markerTM = content->GetMarkerTransform(mStrokeWidth, mX, mY,
                                                mAutoAngle, mIsStart);
  Matrix viewBoxTM = content->GetViewBoxTransform();

  return ThebesMatrix(viewBoxTM * markerTM) * markedTM;
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
nsSVGMarkerFrame::PaintMark(gfxContext& aContext,
                            const gfxMatrix& aToMarkedFrameUserSpace,
                            nsSVGPathGeometryFrame *aMarkedFrame,
                            nsSVGMark *aMark, float aStrokeWidth)
{
  
  
  
  if (mInUse)
    return NS_OK;

  AutoMarkerReferencer markerRef(this, aMarkedFrame);

  SVGMarkerElement *marker = static_cast<SVGMarkerElement*>(mContent);
  if (!marker->HasValidDimensions()) {
    return NS_OK;
  }

  const nsSVGViewBoxRect viewBox = marker->GetViewBoxRect();

  if (viewBox.width <= 0.0f || viewBox.height <= 0.0f) {
    
    return NS_OK;
  }

  mStrokeWidth = aStrokeWidth;
  mX = aMark->x;
  mY = aMark->y;
  mAutoAngle = aMark->angle;
  mIsStart = aMark->type == nsSVGMark::eStart;

  Matrix viewBoxTM = marker->GetViewBoxTransform();

  Matrix markerTM = marker->GetMarkerTransform(mStrokeWidth, mX, mY,
                                               mAutoAngle, mIsStart);

  gfxMatrix markTM = ThebesMatrix(viewBoxTM) * ThebesMatrix(markerTM) *
                     aToMarkedFrameUserSpace;

  if (StyleDisplay()->IsScrollableOverflow()) {
    aContext.Save();
    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, viewBox.x, viewBox.y,
                                      viewBox.width, viewBox.height);
    nsSVGUtils::SetClipRect(&aContext, markTM, clipRect);
  }


  nsIFrame* kid = GetAnonymousChildFrame(this);
  nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
  
  SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);
  nsSVGUtils::PaintFrameWithEffects(kid, aContext, markTM);

  if (StyleDisplay()->IsScrollableOverflow())
    aContext.Restore();

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
  if (!content->HasValidDimensions()) {
    return bbox;
  }

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
  Matrix viewBoxTM = content->GetViewBoxTransform();

  Matrix tm = viewBoxTM * markerTM * aToBBoxUserspace;

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




nsContainerFrame*
NS_NewSVGMarkerAnonChildFrame(nsIPresShell* aPresShell,
                              nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMarkerAnonChildFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMarkerAnonChildFrame)

#ifdef DEBUG
void
nsSVGMarkerAnonChildFrame::Init(nsIContent*       aContent,
                                nsContainerFrame* aParent,
                                nsIFrame*         aPrevInFlow)
{
  MOZ_ASSERT(aParent->GetType() == nsGkAtoms::svgMarkerFrame,
             "Unexpected parent");
  nsSVGMarkerAnonChildFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif

nsIAtom *
nsSVGMarkerAnonChildFrame::GetType() const
{
  return nsGkAtoms::svgMarkerAnonChildFrame;
}
