





#include "nsSVGPathGeometryFrame.h"


#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxSVGGlyphs.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsDisplayList.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGMarkerFrame.h"
#include "nsSVGPathGeometryElement.h"
#include "nsSVGUtils.h"
#include "mozilla/ArrayUtils.h"
#include "SVGAnimatedTransformList.h"
#include "SVGContentUtils.h"
#include "SVGGraphicsElement.h"

using namespace mozilla;
using namespace mozilla::gfx;




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
  
  nsPoint userSpacePtInAppUnits = pointRelativeToReferenceFrame -
                                   (ToReferenceFrame() - frame->GetPosition());
  gfxPoint userSpacePt =
    gfxPoint(userSpacePtInAppUnits.x, userSpacePtInAppUnits.y) /
      frame->PresContext()->AppUnitsPerCSSPixel();
  if (frame->GetFrameForPoint(userSpacePt)) {
    aOutFrames->AppendElement(frame);
  }
}

void
nsDisplaySVGPathGeometry::Paint(nsDisplayListBuilder* aBuilder,
                                nsRenderingContext* aCtx)
{
  uint32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  
  nsPoint offset = ToReferenceFrame() - mFrame->GetPosition();

  gfxPoint devPixelOffset =
    nsLayoutUtils::PointToGfxPoint(offset, appUnitsPerDevPixel);

  aCtx->ThebesContext()->Save();
  gfxMatrix tm = nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(mFrame) *
                   gfxMatrix::Translation(devPixelOffset);
  static_cast<nsSVGPathGeometryFrame*>(mFrame)->PaintSVG(aCtx, tm);
  aCtx->ThebesContext()->Restore();
}




void
nsSVGPathGeometryFrame::Init(nsIContent*       aContent,
                             nsContainerFrame* aParent,
                             nsIFrame*         aPrevInFlow)
{
  AddStateBits(aParent->GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD);
  nsSVGPathGeometryFrameBase::Init(aContent, aParent, aPrevInFlow);
}

nsresult
nsSVGPathGeometryFrame::AttributeChanged(int32_t         aNameSpaceID,
                                         nsIAtom*        aAttribute,
                                         int32_t         aModType)
{
  
  
  
  

  if (aNameSpaceID == kNameSpaceID_None &&
      (static_cast<nsSVGPathGeometryElement*>
                  (mContent)->AttributeDefinesGeometry(aAttribute))) {
    nsSVGEffects::InvalidateRenderingObservers(this);
    nsSVGUtils::ScheduleReflowSVG(this);
  }
  return NS_OK;
}

 void
nsSVGPathGeometryFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGPathGeometryFrameBase::DidSetStyleContext(aOldStyleContext);

  if (aOldStyleContext) {
    float oldOpacity = aOldStyleContext->PeekStyleDisplay()->mOpacity;
    float newOpacity = StyleDisplay()->mOpacity;
    if (newOpacity != oldOpacity &&
        nsSVGUtils::CanOptimizeOpacity(this)) {
      
      
      InvalidateFrame();
    }
  }
}

nsIAtom *
nsSVGPathGeometryFrame::GetType() const
{
  return nsGkAtoms::svgPathGeometryFrame;
}

bool
nsSVGPathGeometryFrame::IsSVGTransformed(gfx::Matrix *aOwnTransform,
                                         gfx::Matrix *aFromParentTransform) const
{
  bool foundTransform = false;

  
  nsIFrame *parent = GetParent();
  if (parent &&
      parent->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer)) {
    foundTransform = static_cast<nsSVGContainerFrame*>(parent)->
                       HasChildrenOnlyTransform(aFromParentTransform);
  }

  nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
  nsSVGAnimatedTransformList* transformList =
    content->GetAnimatedTransformList();
  if ((transformList && transformList->HasTransform()) ||
      content->GetAnimateMotionTransform()) {
    if (aOwnTransform) {
      *aOwnTransform = gfx::ToMatrix(content->PrependLocalTransformsTo(gfxMatrix(),
                                  nsSVGElement::eUserSpaceToParent));
    }
    foundTransform = true;
  }
  return foundTransform;
}

void
nsSVGPathGeometryFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  if (!static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return;
  }
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplaySVGPathGeometry(aBuilder, this));
}




nsresult
nsSVGPathGeometryFrame::PaintSVG(nsRenderingContext *aContext,
                                 const gfxMatrix& aTransform,
                                 const nsIntRect* aDirtyRect)
{
  if (!StyleVisibility()->IsVisible())
    return NS_OK;

  uint32_t paintOrder = StyleSVG()->mPaintOrder;
  if (paintOrder == NS_STYLE_PAINT_ORDER_NORMAL) {
    Render(aContext, eRenderFill | eRenderStroke, aTransform);
    PaintMarkers(aContext, aTransform);
  } else {
    while (paintOrder) {
      uint32_t component =
        paintOrder & ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) - 1);
      switch (component) {
        case NS_STYLE_PAINT_ORDER_FILL:
          Render(aContext, eRenderFill, aTransform);
          break;
        case NS_STYLE_PAINT_ORDER_STROKE:
          Render(aContext, eRenderStroke, aTransform);
          break;
        case NS_STYLE_PAINT_ORDER_MARKERS:
          PaintMarkers(aContext, aTransform);
          break;
      }
      paintOrder >>= NS_STYLE_PAINT_ORDER_BITWIDTH;
    }
  }

  return NS_OK;
}

nsIFrame*
nsSVGPathGeometryFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  FillRule fillRule;
  uint16_t hitTestFlags;
  if (GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) {
    hitTestFlags = SVG_HIT_TEST_FILL;
    fillRule = StyleSVG()->mClipRule == NS_STYLE_FILL_RULE_NONZERO
                 ? FillRule::FILL_WINDING : FillRule::FILL_EVEN_ODD;
  } else {
    hitTestFlags = GetHitTestFlags();
    if (!hitTestFlags) {
      return nullptr;
    }
    if (hitTestFlags & SVG_HIT_TEST_CHECK_MRECT) {
      gfxRect rect =
        nsLayoutUtils::RectToGfxRect(mRect, PresContext()->AppUnitsPerCSSPixel());
      if (!rect.Contains(aPoint)) {
        return nullptr;
      }
    }
    fillRule = StyleSVG()->mFillRule == NS_STYLE_FILL_RULE_NONZERO
                 ? FillRule::FILL_WINDING : FillRule::FILL_EVEN_ODD;
  }

  bool isHit = false;

  nsSVGPathGeometryElement* content =
    static_cast<nsSVGPathGeometryElement*>(mContent);

  
  
  
  RefPtr<DrawTarget> drawTarget =
    gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
  RefPtr<PathBuilder> builder =
    drawTarget->CreatePathBuilder(fillRule);
  RefPtr<Path> path = content->BuildPath(builder);
  if (!path) {
    return nullptr; 
  }

  if (hitTestFlags & SVG_HIT_TEST_FILL) {
    isHit = path->ContainsPoint(ToPoint(aPoint), Matrix());
  }
  if (!isHit && (hitTestFlags & SVG_HIT_TEST_STROKE)) {
    Point point = ToPoint(aPoint);
    SVGContentUtils::AutoStrokeOptions stroke;
    SVGContentUtils::GetStrokeOptions(&stroke, content, StyleContext(), nullptr);
    Matrix nonScalingStrokeMatrix = ToMatrix(nsSVGUtils::GetStrokeTransform(this));
    if (!nonScalingStrokeMatrix.IsIdentity()) {
      
      
      
      
      if (!nonScalingStrokeMatrix.Invert()) {
        return nullptr;
      }
      point = nonScalingStrokeMatrix * point;
      RefPtr<PathBuilder> builder =
        path->TransformedCopyToBuilder(nonScalingStrokeMatrix, fillRule);
      path = builder->Finish();
    }
    isHit = path->StrokeContainsPoint(stroke, point, Matrix());
  }

  if (isHit && nsSVGUtils::HitTestClip(this, aPoint))
    return this;

  return nullptr;
}

nsRect
nsSVGPathGeometryFrame::GetCoveredRegion()
{
  return nsSVGUtils::TransformFrameRectToOuterSVG(
           mRect, GetCanvasTM(), PresContext());
}

void
nsSVGPathGeometryFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
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
 
  
  
  
  
  
  
  
  
  
  
  gfxSize scaleFactors = GetCanvasTM().ScaleFactors(true);
  bool applyScaling = fabs(scaleFactors.width) >= 1e-6 &&
                      fabs(scaleFactors.height) >= 1e-6;
  gfx::Matrix scaling;
  if (applyScaling) {
    scaling.PreScale(scaleFactors.width, scaleFactors.height);
  }
  gfxRect extent = GetBBoxContribution(scaling, flags).ToThebesRect();
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

  
  
  
  
  
  

  
  
  
  
  

  if (aFlags & COORD_CONTEXT_CHANGED) {
    
    
    
    
    
    if (static_cast<nsSVGPathGeometryElement*>(mContent)->GeometryDependsOnCoordCtx() ||
        StyleSVG()->mStrokeWidth.HasPercent()) {
      nsSVGUtils::ScheduleReflowSVG(this);
    }
  }

  if ((aFlags & TRANSFORM_CHANGED) &&
      StyleSVGReset()->mVectorEffect ==
        NS_STYLE_VECTOR_EFFECT_NON_SCALING_STROKE) {
    
    
    nsSVGUtils::ScheduleReflowSVG(this);
  } 
}

SVGBBox
nsSVGPathGeometryFrame::GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                            uint32_t aFlags)
{
  SVGBBox bbox;

  if (aToBBoxUserspace.IsSingular()) {
    
    return bbox;
  }

  RefPtr<DrawTarget> tmpDT;
#ifdef XP_WIN
  
  
  
  
  
  nsRefPtr<gfxASurface> refSurf =
    gfxPlatform::GetPlatform()->ScreenReferenceSurface();
  tmpDT = gfxPlatform::GetPlatform()->
    CreateDrawTargetForSurface(refSurf, IntSize(1, 1));
#else
  tmpDT = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
#endif
  nsRefPtr<gfxContext> tmpCtx = new gfxContext(tmpDT);

  GeneratePath(tmpCtx, aToBBoxUserspace);
  tmpCtx->SetMatrix(gfxMatrix());

  
  
  
  
  
  
  
  
  
  
  

  gfxRect pathExtents = tmpCtx->GetUserPathExtent();

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeFillGeometry) ||
      ((aFlags & nsSVGUtils::eBBoxIncludeFill) &&
       StyleSVG()->mFill.mType != eStyleSVGPaintType_None)) {
    bbox = pathExtents;
  }

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeStrokeGeometry) ||
      ((aFlags & nsSVGUtils::eBBoxIncludeStroke) &&
       nsSVGUtils::HasStroke(this))) {
    
    
    
    if (pathExtents.Width() <= 0 && pathExtents.Height() <= 0) {
      
      
      
      
      
      
      pathExtents.MoveTo(tmpCtx->GetUserStrokeExtent().Center());
      pathExtents.SizeTo(0, 0);
    }
    bbox.UnionEdges(nsSVGUtils::PathExtentsToMaxStrokeExtents(pathExtents,
                                                              this,
                                                              ThebesMatrix(aToBBoxUserspace)));
  }

  
  if ((aFlags & nsSVGUtils::eBBoxIncludeMarkers) != 0 &&
      static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {

    float strokeWidth = nsSVGUtils::GetStrokeWidth(this);
    MarkerProperties properties = GetMarkerProperties(this);

    if (properties.MarkersExist()) {
      nsTArray<nsSVGMark> marks;
      static_cast<nsSVGPathGeometryElement*>(mContent)->GetMarkPoints(&marks);
      uint32_t num = marks.Length();

      
      nsSVGMarkerFrame* markerFrames[] = {
        properties.GetMarkerStartFrame(),
        properties.GetMarkerMidFrame(),
        properties.GetMarkerEndFrame(),
      };
      PR_STATIC_ASSERT(MOZ_ARRAY_LENGTH(markerFrames) == nsSVGMark::eTypeCount);

      for (uint32_t i = 0; i < num; i++) {
        nsSVGMark& mark = marks[i];
        nsSVGMarkerFrame* frame = markerFrames[mark.type];
        if (frame) {
          SVGBBox mbbox =
            frame->GetMarkBBoxContribution(aToBBoxUserspace, aFlags, this,
                                           &marks[i], strokeWidth);
          bbox.UnionEdges(mbbox);
        }
      }
    }
  }

  return bbox;
}




gfxMatrix
nsSVGPathGeometryFrame::GetCanvasTM()
{
  NS_ASSERTION(GetParent(), "null parent");

  nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
  dom::SVGGraphicsElement *content = static_cast<dom::SVGGraphicsElement*>(mContent);

  return content->PrependLocalTransformsTo(parent->GetCanvasTM());
}

nsSVGPathGeometryFrame::MarkerProperties
nsSVGPathGeometryFrame::GetMarkerProperties(nsSVGPathGeometryFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  MarkerProperties result;
  const nsStyleSVG *style = aFrame->StyleSVG();
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
                               uint32_t aRenderComponents,
                               const gfxMatrix& aTransform)
{
  gfxContext *gfx = aContext->ThebesContext();

  uint16_t renderMode = SVGAutoRenderState::GetRenderMode(aContext);

  switch (StyleSVG()->mShapeRendering) {
  case NS_STYLE_SHAPE_RENDERING_OPTIMIZESPEED:
  case NS_STYLE_SHAPE_RENDERING_CRISPEDGES:
    gfx->SetAntialiasMode(AntialiasMode::NONE);
    break;
  default:
    gfx->SetAntialiasMode(AntialiasMode::SUBPIXEL);
    break;
  }

  if (renderMode != SVGAutoRenderState::NORMAL) {
    NS_ABORT_IF_FALSE(renderMode == SVGAutoRenderState::CLIP ||
                      renderMode == SVGAutoRenderState::CLIP_MASK,
                      "Unknown render mode");

    
    
    
    
    
    
    
    
    
    
    
    
    gfxContextMatrixAutoSaveRestore autoSaveRestore;
    
    
    
      autoSaveRestore.SetContext(gfx);
    

    GeneratePath(gfx, ToMatrix(aTransform));

    
    
    
    
    
    
    

    gfxContext::FillRule oldFillRull = gfx->CurrentFillRule();

    if (StyleSVG()->mClipRule == NS_STYLE_FILL_RULE_EVENODD)
      gfx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    else
      gfx->SetFillRule(gfxContext::FILL_RULE_WINDING);

    if (renderMode == SVGAutoRenderState::CLIP_MASK) {
      gfx->SetColor(gfxRGBA(1.0f, 1.0f, 1.0f, 1.0f));
      gfx->Fill();
      gfx->SetFillRule(oldFillRull); 
      gfx->NewPath();
    }

    return;
  }

  gfxContextAutoSaveRestore autoSaveRestore(gfx);

  GeneratePath(gfx, ToMatrix(aTransform));

  gfxTextContextPaint *contextPaint =
    (gfxTextContextPaint*)aContext->GetDrawTarget()->GetUserData(&gfxTextContextPaint::sUserDataKey);

  if ((aRenderComponents & eRenderFill)) {
    nsRefPtr<gfxPattern> fillPattern =
      nsSVGUtils::MakeFillPatternFor(this, gfx, contextPaint);
    if (fillPattern) {
      gfx->SetPattern(fillPattern);
      gfx->SetFillRule(nsSVGUtils::ThebesFillRule(StyleSVG()->mFillRule));
      gfx->Fill();
    }
  }

  if ((aRenderComponents & eRenderStroke) &&
       nsSVGUtils::SetupCairoStroke(this, gfx, contextPaint)) {
    gfx->Stroke();
  }

  gfx->NewPath();
}

void
nsSVGPathGeometryFrame::GeneratePath(gfxContext* aContext,
                                     const Matrix &aTransform)
{
  if (aTransform.IsSingular()) {
    aContext->SetMatrix(gfxMatrix());
    aContext->NewPath();
    return;
  }

  aContext->SetMatrix(
    aContext->CurrentMatrix().PreMultiply(ThebesMatrix(aTransform)).
                              NudgeToIntegers());

  
  const nsStyleSVG* style = StyleSVG();
  if (style->mStrokeLinecap == NS_STYLE_STROKE_LINECAP_SQUARE) {
    aContext->SetLineCap(gfxContext::LINE_CAP_SQUARE);
  }

  aContext->NewPath();
  static_cast<nsSVGPathGeometryElement*>(mContent)->ConstructPath(aContext);
}

void
nsSVGPathGeometryFrame::PaintMarkers(nsRenderingContext* aContext,
                                     const gfxMatrix& aTransform)
{
  gfxTextContextPaint *contextPaint =
    (gfxTextContextPaint*)aContext->GetDrawTarget()->GetUserData(&gfxTextContextPaint::sUserDataKey);

  if (static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {
    MarkerProperties properties = GetMarkerProperties(this);

    if (properties.MarkersExist()) {
      float strokeWidth = nsSVGUtils::GetStrokeWidth(this, contextPaint);

      nsTArray<nsSVGMark> marks;
      static_cast<nsSVGPathGeometryElement*>
                 (mContent)->GetMarkPoints(&marks);

      uint32_t num = marks.Length();
      if (num) {
        
        nsSVGMarkerFrame* markerFrames[] = {
          properties.GetMarkerStartFrame(),
          properties.GetMarkerMidFrame(),
          properties.GetMarkerEndFrame(),
        };
        PR_STATIC_ASSERT(MOZ_ARRAY_LENGTH(markerFrames) == nsSVGMark::eTypeCount);

        for (uint32_t i = 0; i < num; i++) {
          nsSVGMark& mark = marks[i];
          nsSVGMarkerFrame* frame = markerFrames[mark.type];
          if (frame) {
            frame->PaintMark(aContext, aTransform, this, &mark, strokeWidth);
          }
        }
      }
    }
  }
}

uint16_t
nsSVGPathGeometryFrame::GetHitTestFlags()
{
  return nsSVGUtils::GetGeometryHitTestFlags(this);
}
