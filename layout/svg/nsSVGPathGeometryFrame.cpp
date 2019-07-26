





#include "nsSVGPathGeometryFrame.h"


#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxSVGGlyphs.h"
#include "nsDisplayList.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGMarkerFrame.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGUtils.h"
#include "SVGAnimatedTransformList.h"
#include "SVGGraphicsElement.h"

using namespace mozilla;




nsIFrame*
NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell,
                           nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGPathGeometryFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPathGeometryFrame)




NS_QUERYFRAME_HEAD(nsSVGPathGeometryFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
  NS_QUERYFRAME_ENTRY(nsSVGPathGeometryFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGPathGeometryFrameBase)




class nsDisplaySVGPathGeometry : public nsDisplayItem {
public:
  nsDisplaySVGPathGeometry(nsDisplayListBuilder* aBuilder,
                           nsSVGPathGeometryFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplaySVGPathGeometry);
    NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGPathGeometry() {
    MOZ_COUNT_DTOR(nsDisplaySVGPathGeometry);
  }
#endif
 
  NS_DISPLAY_DECL_NAME("nsDisplaySVGPathGeometry", TYPE_SVG_PATH_GEOMETRY)

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
};

void
nsDisplaySVGPathGeometry::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                                  HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsSVGPathGeometryFrame *frame = static_cast<nsSVGPathGeometryFrame*>(mFrame);
  nsPoint pointRelativeToReferenceFrame = aRect.Center();
  
  nsPoint userSpacePt = pointRelativeToReferenceFrame -
                          (ToReferenceFrame() - frame->GetPosition());
  if (frame->GetFrameForPoint(userSpacePt)) {
    aOutFrames->AppendElement(frame);
  }
}

void
nsDisplaySVGPathGeometry::Paint(nsDisplayListBuilder* aBuilder,
                                nsRenderingContext* aCtx)
{
  
  
  
  nsPoint offset = ToReferenceFrame() - mFrame->GetPosition();

  aCtx->PushState();
  aCtx->Translate(offset);
  static_cast<nsSVGPathGeometryFrame*>(mFrame)->PaintSVG(aCtx, nullptr);
  aCtx->PopState();
}




NS_IMETHODIMP
nsSVGPathGeometryFrame::AttributeChanged(int32_t         aNameSpaceID,
                                         nsIAtom*        aAttribute,
                                         int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (static_cast<nsSVGPathGeometryElement*>
                  (mContent)->AttributeDefinesGeometry(aAttribute) ||
       aAttribute == nsGkAtoms::transform)) {
    nsSVGUtils::InvalidateBounds(this, false);
    nsSVGUtils::ScheduleReflowSVG(this);
  }
  return NS_OK;
}

 void
nsSVGPathGeometryFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGPathGeometryFrameBase::DidSetStyleContext(aOldStyleContext);

  
  
  
  
  

  nsSVGUtils::InvalidateBounds(this, false);
  nsSVGUtils::ScheduleReflowSVG(this);
}

nsIAtom *
nsSVGPathGeometryFrame::GetType() const
{
  return nsGkAtoms::svgPathGeometryFrame;
}

bool
nsSVGPathGeometryFrame::IsSVGTransformed(gfxMatrix *aOwnTransform,
                                         gfxMatrix *aFromParentTransform) const
{
  bool foundTransform = false;

  
  nsIFrame *parent = GetParent();
  if (parent &&
      parent->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer)) {
    foundTransform = static_cast<nsSVGContainerFrame*>(parent)->
                       HasChildrenOnlyTransform(aFromParentTransform);
  }

  nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
  if (content->GetAnimatedTransformList() ||
      content->GetAnimateMotionTransform()) {
    if (aOwnTransform) {
      *aOwnTransform = content->PrependLocalTransformsTo(gfxMatrix(),
                                  nsSVGElement::eUserSpaceToParent);
    }
    foundTransform = true;
  }
  return foundTransform;
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  if (!static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return NS_OK;
  }
  return aLists.Content()->AppendNewToTop(
           new (aBuilder) nsDisplaySVGPathGeometry(aBuilder, this));
}




NS_IMETHODIMP
nsSVGPathGeometryFrame::PaintSVG(nsRenderingContext *aContext,
                                 const nsIntRect *aDirtyRect)
{
  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;

  uint32_t paintOrder = GetStyleSVG()->mPaintOrder;
  if (paintOrder == NS_STYLE_PAINT_ORDER_NORMAL) {
    Render(aContext, eRenderFill | eRenderStroke);
    PaintMarkers(aContext);
  } else {
    while (paintOrder) {
      uint32_t component =
        paintOrder & ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) - 1);
      switch (component) {
        case NS_STYLE_PAINT_ORDER_FILL:
          Render(aContext, eRenderFill);
          break;
        case NS_STYLE_PAINT_ORDER_STROKE:
          Render(aContext, eRenderStroke);
          break;
        case NS_STYLE_PAINT_ORDER_MARKERS:
          PaintMarkers(aContext);
          break;
      }
      paintOrder >>= NS_STYLE_PAINT_ORDER_BITWIDTH;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGPathGeometryFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  gfxMatrix canvasTM = GetCanvasTM(FOR_HIT_TESTING);
  if (canvasTM.IsSingular()) {
    return nullptr;
  }
  uint16_t fillRule, hitTestFlags;
  if (GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) {
    hitTestFlags = SVG_HIT_TEST_FILL;
    fillRule = GetClipRule();
  } else {
    hitTestFlags = GetHitTestFlags();
    
    
    nsPoint point =
      nsSVGUtils::TransformOuterSVGPointToChildFrame(aPoint, canvasTM, PresContext());
    if (!hitTestFlags || ((hitTestFlags & SVG_HIT_TEST_CHECK_MRECT) &&
                          !mRect.Contains(point)))
      return nullptr;
    fillRule = GetStyleSVG()->mFillRule;
  }

  bool isHit = false;

  nsRefPtr<gfxContext> tmpCtx =
    new gfxContext(gfxPlatform::GetPlatform()->ScreenReferenceSurface());

  GeneratePath(tmpCtx, canvasTM);
  gfxPoint userSpacePoint =
    tmpCtx->DeviceToUser(gfxPoint(PresContext()->AppUnitsToGfxUnits(aPoint.x),
                                  PresContext()->AppUnitsToGfxUnits(aPoint.y)));

  if (fillRule == NS_STYLE_FILL_RULE_EVENODD)
    tmpCtx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
  else
    tmpCtx->SetFillRule(gfxContext::FILL_RULE_WINDING);

  if (hitTestFlags & SVG_HIT_TEST_FILL)
    isHit = tmpCtx->PointInFill(userSpacePoint);
  if (!isHit && (hitTestFlags & SVG_HIT_TEST_STROKE)) {
    nsSVGUtils::SetupCairoStrokeHitGeometry(this, tmpCtx);
    isHit = tmpCtx->PointInStroke(userSpacePoint);
  }

  if (isHit && nsSVGUtils::HitTestClip(this, aPoint))
    return this;

  return nullptr;
}

NS_IMETHODIMP_(nsRect)
nsSVGPathGeometryFrame::GetCoveredRegion()
{
  return nsSVGUtils::TransformFrameRectToOuterSVG(
           mRect, GetCanvasTM(FOR_OUTERSVG_TM), PresContext());
}

void
nsSVGPathGeometryFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  uint32_t flags = nsSVGUtils::eBBoxIncludeFill |
                   nsSVGUtils::eBBoxIncludeStroke |
                   nsSVGUtils::eBBoxIncludeMarkers;
  
  
  
  
  
  uint16_t hitTestFlags = GetHitTestFlags();
  if ((hitTestFlags & SVG_HIT_TEST_FILL)) {
   flags |= nsSVGUtils::eBBoxIncludeFillGeometry;
  }
  if ((hitTestFlags & SVG_HIT_TEST_STROKE)) {
   flags |= nsSVGUtils::eBBoxIncludeStrokeGeometry;
  }
 
  
  
  
  
  
  
  
  
  
  
  gfxSize scaleFactors = GetCanvasTM(FOR_OUTERSVG_TM).ScaleFactors(true);
  bool applyScaling = fabs(scaleFactors.width) >= 1e-6 &&
                      fabs(scaleFactors.height) >= 1e-6;
  gfxMatrix scaling;
  if (applyScaling) {
    scaling.Scale(scaleFactors.width, scaleFactors.height);
  }
  gfxRect extent = GetBBoxContribution(scaling, flags);
  if (applyScaling) {
    extent.Scale(1 / scaleFactors.width, 1 / scaleFactors.height);
  }
  mRect = nsLayoutUtils::RoundGfxRectToAppRect(extent,
            PresContext()->AppUnitsPerCSSPixel());

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    InvalidateFrame();
  }
}

void
nsSVGPathGeometryFrame::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  
  
  
  
  
  nsSVGUtils::ScheduleReflowSVG(this);
}

SVGBBox
nsSVGPathGeometryFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                            uint32_t aFlags)
{
  SVGBBox bbox;

  if (aToBBoxUserspace.IsSingular()) {
    
    return bbox;
  }

  nsRefPtr<gfxContext> tmpCtx =
    new gfxContext(gfxPlatform::GetPlatform()->ScreenReferenceSurface());

  GeneratePath(tmpCtx, aToBBoxUserspace);
  tmpCtx->IdentityMatrix();

  
  
  
  
  
  
  
  
  
  
  

  gfxRect pathExtents = tmpCtx->GetUserPathExtent();

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeFillGeometry) ||
      ((aFlags & nsSVGUtils::eBBoxIncludeFill) &&
       GetStyleSVG()->mFill.mType != eStyleSVGPaintType_None)) {
    bbox = pathExtents;
  }

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeStrokeGeometry) ||
      ((aFlags & nsSVGUtils::eBBoxIncludeStroke) &&
       nsSVGUtils::HasStroke(this))) {
    
    
    
    if (pathExtents.Width() <= 0 && pathExtents.Height() <= 0) {
      
      
      
      
      
      
      nsSVGUtils::SetupCairoStrokeGeometry(this, tmpCtx);
      pathExtents.MoveTo(tmpCtx->GetUserStrokeExtent().Center());
      pathExtents.SizeTo(0, 0);
    }
    bbox.UnionEdges(nsSVGUtils::PathExtentsToMaxStrokeExtents(pathExtents,
                                                              this,
                                                              aToBBoxUserspace));
  }

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeMarkers) != 0 &&
      static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {

    float strokeWidth = nsSVGUtils::GetStrokeWidth(this);
    MarkerProperties properties = GetMarkerProperties(this);

    if (properties.MarkersExist()) {
      nsTArray<nsSVGMark> marks;
      static_cast<nsSVGPathGeometryElement*>(mContent)->GetMarkPoints(&marks);
      uint32_t num = marks.Length();

      if (num) {
        nsSVGMarkerFrame *frame = properties.GetMarkerStartFrame();
        if (frame) {
          SVGBBox mbbox =
            frame->GetMarkBBoxContribution(aToBBoxUserspace, aFlags, this,
                                           &marks[0], strokeWidth);
          bbox.UnionEdges(mbbox);
        }

        frame = properties.GetMarkerMidFrame();
        if (frame) {
          for (uint32_t i = 1; i < num - 1; i++) {
            SVGBBox mbbox =
              frame->GetMarkBBoxContribution(aToBBoxUserspace, aFlags, this,
                                             &marks[i], strokeWidth);
            bbox.UnionEdges(mbbox);
          }
        }

        frame = properties.GetMarkerEndFrame();
        if (frame) {
          SVGBBox mbbox =
            frame->GetMarkBBoxContribution(aToBBoxUserspace, aFlags, this,
                                           &marks[num-1], strokeWidth);
          bbox.UnionEdges(mbbox);
        }
      }
    }
  }

  return bbox;
}




gfxMatrix
nsSVGPathGeometryFrame::GetCanvasTM(uint32_t aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }

  NS_ASSERTION(mParent, "null parent");

  nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
  dom::SVGGraphicsElement *content = static_cast<dom::SVGGraphicsElement*>(mContent);

  return content->PrependLocalTransformsTo(parent->GetCanvasTM(aFor));
}




nsSVGPathGeometryFrame::MarkerProperties
nsSVGPathGeometryFrame::GetMarkerProperties(nsSVGPathGeometryFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  MarkerProperties result;
  const nsStyleSVG *style = aFrame->GetStyleSVG();
  result.mMarkerStart =
    nsSVGEffects::GetMarkerProperty(style->mMarkerStart, aFrame,
                                    nsSVGEffects::MarkerBeginProperty());
  result.mMarkerMid =
    nsSVGEffects::GetMarkerProperty(style->mMarkerMid, aFrame,
                                    nsSVGEffects::MarkerMiddleProperty());
  result.mMarkerEnd =
    nsSVGEffects::GetMarkerProperty(style->mMarkerEnd, aFrame,
                                    nsSVGEffects::MarkerEndProperty());
  return result;
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerStartFrame()
{
  if (!mMarkerStart)
    return nullptr;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerStart->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nullptr));
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerMidFrame()
{
  if (!mMarkerMid)
    return nullptr;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerMid->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nullptr));
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerEndFrame()
{
  if (!mMarkerEnd)
    return nullptr;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerEnd->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nullptr));
}

void
nsSVGPathGeometryFrame::Render(nsRenderingContext *aContext,
                               uint32_t aRenderComponents)
{
  gfxContext *gfx = aContext->ThebesContext();

  uint16_t renderMode = SVGAutoRenderState::GetRenderMode(aContext);

  switch (GetStyleSVG()->mShapeRendering) {
  case NS_STYLE_SHAPE_RENDERING_OPTIMIZESPEED:
  case NS_STYLE_SHAPE_RENDERING_CRISPEDGES:
    gfx->SetAntialiasMode(gfxContext::MODE_ALIASED);
    break;
  default:
    gfx->SetAntialiasMode(gfxContext::MODE_COVERAGE);
    break;
  }

  
  gfx->Save();

  GeneratePath(gfx, GetCanvasTM(FOR_PAINTING));

  if (renderMode != SVGAutoRenderState::NORMAL) {
    NS_ABORT_IF_FALSE(renderMode == SVGAutoRenderState::CLIP ||
                      renderMode == SVGAutoRenderState::CLIP_MASK,
                      "Unknown render mode");
    gfx->Restore();

    if (GetClipRule() == NS_STYLE_FILL_RULE_EVENODD)
      gfx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    else
      gfx->SetFillRule(gfxContext::FILL_RULE_WINDING);

    if (renderMode == SVGAutoRenderState::CLIP_MASK) {
      gfx->SetColor(gfxRGBA(1.0f, 1.0f, 1.0f, 1.0f));
      gfx->Fill();
      gfx->NewPath();
    }

    return;
  }

  gfxTextObjectPaint *objectPaint =
    (gfxTextObjectPaint*)aContext->GetUserData(&gfxTextObjectPaint::sUserDataKey);

  if ((aRenderComponents & eRenderFill) &&
      nsSVGUtils::SetupCairoFillPaint(this, gfx, objectPaint)) {
    gfx->Fill();
  }

  if ((aRenderComponents & eRenderStroke) &&
       nsSVGUtils::SetupCairoStroke(this, gfx, objectPaint)) {
    gfx->Stroke();
  }

  gfx->NewPath();

  gfx->Restore();
}

void
nsSVGPathGeometryFrame::GeneratePath(gfxContext* aContext,
                                     const gfxMatrix &aTransform)
{
  if (aTransform.IsSingular()) {
    aContext->IdentityMatrix();
    aContext->NewPath();
    return;
  }

  aContext->MultiplyAndNudgeToIntegers(aTransform);

  
  const nsStyleSVG* style = GetStyleSVG();
  if (style->mStrokeLinecap == NS_STYLE_STROKE_LINECAP_SQUARE) {
    aContext->SetLineCap(gfxContext::LINE_CAP_SQUARE);
  }

  aContext->NewPath();
  static_cast<nsSVGPathGeometryElement*>(mContent)->ConstructPath(aContext);
}

void
nsSVGPathGeometryFrame::PaintMarkers(nsRenderingContext* aContext)
{
  gfxTextObjectPaint *objectPaint =
    (gfxTextObjectPaint*)aContext->GetUserData(&gfxTextObjectPaint::sUserDataKey);

  if (static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {
    MarkerProperties properties = GetMarkerProperties(this);

    if (properties.MarkersExist()) {
      float strokeWidth = nsSVGUtils::GetStrokeWidth(this, objectPaint);

      nsTArray<nsSVGMark> marks;
      static_cast<nsSVGPathGeometryElement*>
                 (mContent)->GetMarkPoints(&marks);

      uint32_t num = marks.Length();

      if (num) {
        nsSVGMarkerFrame *frame = properties.GetMarkerStartFrame();
        if (frame)
          frame->PaintMark(aContext, this, &marks[0], strokeWidth);

        frame = properties.GetMarkerMidFrame();
        if (frame) {
          for (uint32_t i = 1; i < num - 1; i++)
            frame->PaintMark(aContext, this, &marks[i], strokeWidth);
        }

        frame = properties.GetMarkerEndFrame();
        if (frame)
          frame->PaintMark(aContext, this, &marks[num-1], strokeWidth);
      }
    }
  }
}
