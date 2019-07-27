





#include "nsSVGPathGeometryFrame.h"


#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxSVGGlyphs.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Helpers.h"
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
    MOZ_ASSERT(aFrame, "Must have a frame!");
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGPathGeometry() {
    MOZ_COUNT_DTOR(nsDisplaySVGPathGeometry);
  }
#endif
 
  NS_DISPLAY_DECL_NAME("nsDisplaySVGPathGeometry", TYPE_SVG_PATH_GEOMETRY)

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
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

  gfxMatrix tm = nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(mFrame) *
                   gfxMatrix::Translation(devPixelOffset);
  static_cast<nsSVGPathGeometryFrame*>(mFrame)->PaintSVG(*aCtx->ThebesContext(), tm);
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
    nsLayoutUtils::PostRestyleEvent(
      mContent->AsElement(), nsRestyleHint(0),
      nsChangeHint_InvalidateRenderingObservers);
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

    nsSVGPathGeometryElement* element =
      static_cast<nsSVGPathGeometryElement*>(mContent);

    if (aOldStyleContext->PeekStyleSVG()) {
      if ((StyleSVG()->mStrokeLinecap !=
             aOldStyleContext->PeekStyleSVG()->mStrokeLinecap) &&
          element->IsSVGElement(nsGkAtoms::path)) {
        
        
        
        
        element->ClearAnyCachedPath();
      } else if (GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) {
        if (StyleSVG()->mClipRule !=
              aOldStyleContext->PeekStyleSVG()->mClipRule) {
          
          
          element->ClearAnyCachedPath();
        }
      } else {
        if (StyleSVG()->mFillRule !=
              aOldStyleContext->PeekStyleSVG()->mFillRule) {
          
          element->ClearAnyCachedPath();
        }
      }
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
nsSVGPathGeometryFrame::PaintSVG(gfxContext& aContext,
                                 const gfxMatrix& aTransform,
                                 const nsIntRect* aDirtyRect)
{
  if (!StyleVisibility()->IsVisible())
    return NS_OK;

  
  gfxMatrix newMatrix =
    aContext.CurrentMatrix().PreMultiply(aTransform).NudgeToIntegers();
  if (newMatrix.IsSingular()) {
    return NS_OK;
  }

  uint32_t paintOrder = StyleSVG()->mPaintOrder;
  if (paintOrder == NS_STYLE_PAINT_ORDER_NORMAL) {
    Render(&aContext, eRenderFill | eRenderStroke, newMatrix);
    PaintMarkers(aContext, aTransform);
  } else {
    while (paintOrder) {
      uint32_t component =
        paintOrder & ((1 << NS_STYLE_PAINT_ORDER_BITWIDTH) - 1);
      switch (component) {
        case NS_STYLE_PAINT_ORDER_FILL:
          Render(&aContext, eRenderFill, newMatrix);
          break;
        case NS_STYLE_PAINT_ORDER_STROKE:
          Render(&aContext, eRenderStroke, newMatrix);
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
    fillRule = nsSVGUtils::ToFillRule(StyleSVG()->mClipRule);
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
    fillRule = nsSVGUtils::ToFillRule(StyleSVG()->mFillRule);
  }

  bool isHit = false;

  nsSVGPathGeometryElement* content =
    static_cast<nsSVGPathGeometryElement*>(mContent);

  
  
  
  RefPtr<DrawTarget> drawTarget =
    gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
  RefPtr<Path> path = content->GetOrBuildPath(*drawTarget, fillRule);
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
    gfxMatrix userToOuterSVG;
    if (nsSVGUtils::GetNonScalingStrokeTransform(this, &userToOuterSVG)) {
      
      
      
      
      point = ToMatrix(userToOuterSVG) * point;
      RefPtr<PathBuilder> builder =
        path->TransformedCopyToBuilder(ToMatrix(userToOuterSVG), fillRule);
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

  MOZ_ASSERT(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
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
 
  gfxRect extent = GetBBoxContribution(Matrix(), flags).ToThebesRect();
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
  MOZ_ASSERT(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
             "Invalidation logic may need adjusting");

  
  
  
  
  
  

  
  
  
  
  

  if (aFlags & COORD_CONTEXT_CHANGED) {
    
    
    
    
    
    if (static_cast<nsSVGPathGeometryElement*>(mContent)->GeometryDependsOnCoordCtx() ||
        StyleSVG()->mStrokeWidth.HasPercent()) {
      static_cast<nsSVGPathGeometryElement*>(mContent)->ClearAnyCachedPath();
      nsSVGUtils::ScheduleReflowSVG(this);
    }
  }

  if ((aFlags & TRANSFORM_CHANGED) && StyleSVGReset()->HasNonScalingStroke()) {
    
    
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

  nsSVGPathGeometryElement* element =
    static_cast<nsSVGPathGeometryElement*>(mContent);

  bool getFill = (aFlags & nsSVGUtils::eBBoxIncludeFillGeometry) ||
                 ((aFlags & nsSVGUtils::eBBoxIncludeFill) &&
                  StyleSVG()->mFill.mType != eStyleSVGPaintType_None);

  bool getStroke = (aFlags & nsSVGUtils::eBBoxIncludeStrokeGeometry) ||
                   ((aFlags & nsSVGUtils::eBBoxIncludeStroke) &&
                    nsSVGUtils::HasStroke(this));

  bool gotSimpleBounds = false;
  if (!StyleSVGReset()->HasNonScalingStroke()) {
    SVGContentUtils::AutoStrokeOptions strokeOptions;
    strokeOptions.mLineWidth = 0.f;
    if (getStroke) {
      SVGContentUtils::GetStrokeOptions(&strokeOptions, element,
        StyleContext(), nullptr,
        SVGContentUtils::eIgnoreStrokeDashing);
    }
    Rect simpleBounds;
    gotSimpleBounds = element->GetGeometryBounds(&simpleBounds,
                                                 strokeOptions,
                                                 aToBBoxUserspace);
    if (gotSimpleBounds) {
      bbox = simpleBounds;
    }
  }

  if (!gotSimpleBounds) {
    
    RefPtr<DrawTarget> tmpDT;
#ifdef XP_WIN
    
    
    
    
    
    nsRefPtr<gfxASurface> refSurf =
      gfxPlatform::GetPlatform()->ScreenReferenceSurface();
    tmpDT = gfxPlatform::GetPlatform()->
      CreateDrawTargetForSurface(refSurf, IntSize(1, 1));
#else
    tmpDT = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
#endif

    FillRule fillRule = nsSVGUtils::ToFillRule(StyleSVG()->mFillRule);
    RefPtr<Path> pathInUserSpace = element->GetOrBuildPath(*tmpDT, fillRule);
    if (!pathInUserSpace) {
      return bbox;
    }
    RefPtr<Path> pathInBBoxSpace;
    if (aToBBoxUserspace.IsIdentity()) {
      pathInBBoxSpace = pathInUserSpace;
    } else {
      RefPtr<PathBuilder> builder =
        pathInUserSpace->TransformedCopyToBuilder(aToBBoxUserspace, fillRule);
      pathInBBoxSpace = builder->Finish();
      if (!pathInBBoxSpace) {
        return bbox;
      }
    }

    
    
    
    
    
    
    
    
    
    
    

    Rect pathBBoxExtents = pathInBBoxSpace->GetBounds();
    if (!pathBBoxExtents.IsFinite()) {
      
      
      return bbox;
    }

    
    if (getFill) {
      bbox = pathBBoxExtents;
    }

    
    if (getStroke) {
#if 0
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      SVGContentUtils::AutoStrokeOptions strokeOptions;
      SVGContentUtils::GetStrokeOptions(&strokeOptions, element,
                                        StyleContext(), nullptr,
                                        SVGContentUtils::eIgnoreStrokeDashing);
      Rect strokeBBoxExtents;
      gfxMatrix userToOuterSVG;
      if (nsSVGUtils::GetNonScalingStrokeTransform(this, &userToOuterSVG)) {
        Matrix outerSVGToUser = ToMatrix(userToOuterSVG);
        outerSVGToUser.Invert();
        Matrix outerSVGToBBox = aToBBoxUserspace * outerSVGToUser;
        RefPtr<PathBuilder> builder =
          pathInUserSpace->TransformedCopyToBuilder(ToMatrix(userToOuterSVG));
        RefPtr<Path> pathInOuterSVGSpace = builder->Finish();
        strokeBBoxExtents =
          pathInOuterSVGSpace->GetStrokedBounds(strokeOptions, outerSVGToBBox);
      } else {
        strokeBBoxExtents =
          pathInUserSpace->GetStrokedBounds(strokeOptions, aToBBoxUserspace);
      }
      MOZ_ASSERT(strokeBBoxExtents.IsFinite(), "bbox is about to go bad");
      bbox.UnionEdges(strokeBBoxExtents);
#else
    
      gfxRect strokeBBoxExtents =
        nsSVGUtils::PathExtentsToMaxStrokeExtents(ThebesRect(pathBBoxExtents),
                                                  this,
                                                  ThebesMatrix(aToBBoxUserspace));
      MOZ_ASSERT(ToRect(strokeBBoxExtents).IsFinite(), "bbox is about to go bad");
      bbox.UnionEdges(strokeBBoxExtents);
#endif
    }
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
          MOZ_ASSERT(mbbox.IsFinite(), "bbox is about to go bad");
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
nsSVGPathGeometryFrame::Render(gfxContext* aContext,
                               uint32_t aRenderComponents,
                               const gfxMatrix& aNewTransform)
{
  MOZ_ASSERT(!aNewTransform.IsSingular());

  DrawTarget* drawTarget = aContext->GetDrawTarget();

  FillRule fillRule =
    nsSVGUtils::ToFillRule((GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) ?
                             StyleSVG()->mClipRule : StyleSVG()->mFillRule);

  nsSVGPathGeometryElement* element =
    static_cast<nsSVGPathGeometryElement*>(mContent);

  AntialiasMode aaMode =
    (StyleSVG()->mShapeRendering == NS_STYLE_SHAPE_RENDERING_OPTIMIZESPEED ||
     StyleSVG()->mShapeRendering == NS_STYLE_SHAPE_RENDERING_CRISPEDGES) ?
    AntialiasMode::NONE : AntialiasMode::SUBPIXEL;

  
  
  
  gfxContextMatrixAutoSaveRestore autoRestoreTransform(aContext);
  aContext->SetMatrix(aNewTransform);

  if (GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD) {
    
    
    RefPtr<Path> path = element->GetOrBuildPath(*drawTarget, fillRule);
    if (path) {
      ColorPattern white(ToDeviceColor(Color(1.0f, 1.0f, 1.0f, 1.0f)));
      drawTarget->Fill(path, white,
                       DrawOptions(1.0f, CompositionOp::OP_OVER, aaMode));
    }
    return;
  }

  nsSVGPathGeometryElement::SimplePath simplePath;
  RefPtr<Path> path;

  element->GetAsSimplePath(&simplePath);
  if (!simplePath.IsPath()) {
    path = element->GetOrBuildPath(*drawTarget, fillRule);
    if (!path) {
      return;
    }
  }

  gfxTextContextPaint *contextPaint =
    (gfxTextContextPaint*)drawTarget->
      GetUserData(&gfxTextContextPaint::sUserDataKey);

  if (aRenderComponents & eRenderFill) {
    GeneralPattern fillPattern;
    nsSVGUtils::MakeFillPatternFor(this, aContext, &fillPattern, contextPaint);
    if (fillPattern.GetPattern()) {
      DrawOptions drawOptions(1.0f, CompositionOp::OP_OVER, aaMode);
      if (simplePath.IsRect()) {
        drawTarget->FillRect(simplePath.AsRect(), fillPattern, drawOptions);
      } else if (path) {
        drawTarget->Fill(path, fillPattern, drawOptions);
      }
    }
  }

  if ((aRenderComponents & eRenderStroke) &&
      nsSVGUtils::HasStroke(this, contextPaint)) {
    
    gfxMatrix userToOuterSVG;
    if (nsSVGUtils::GetNonScalingStrokeTransform(this, &userToOuterSVG)) {
      
      
      if (!path) {
        path = element->GetOrBuildPath(*drawTarget, fillRule);
        if (!path) {
          return;
        }
        simplePath.Reset();
      }
      
      
      
      gfxMatrix outerSVGToUser = userToOuterSVG;
      outerSVGToUser.Invert();
      aContext->Multiply(outerSVGToUser);
      RefPtr<PathBuilder> builder =
        path->TransformedCopyToBuilder(ToMatrix(userToOuterSVG), fillRule);
      path = builder->Finish();
    }
    GeneralPattern strokePattern;
    nsSVGUtils::MakeStrokePatternFor(this, aContext, &strokePattern, contextPaint);
    if (strokePattern.GetPattern()) {
      SVGContentUtils::AutoStrokeOptions strokeOptions;
      SVGContentUtils::GetStrokeOptions(&strokeOptions,
                                        static_cast<nsSVGElement*>(mContent),
                                        StyleContext(), contextPaint);
      
      if (strokeOptions.mLineWidth <= 0) {
        return;
      }
      DrawOptions drawOptions(1.0f, CompositionOp::OP_OVER, aaMode);
      if (simplePath.IsRect()) {
        drawTarget->StrokeRect(simplePath.AsRect(), strokePattern,
                               strokeOptions, drawOptions);
      } else if (simplePath.IsLine()) {
        drawTarget->StrokeLine(simplePath.Point1(), simplePath.Point2(),
                               strokePattern, strokeOptions, drawOptions);
      } else {
        drawTarget->Stroke(path, strokePattern, strokeOptions, drawOptions);
      }
    }
  }
}

void
nsSVGPathGeometryFrame::PaintMarkers(gfxContext& aContext,
                                     const gfxMatrix& aTransform)
{
  gfxTextContextPaint *contextPaint =
    (gfxTextContextPaint*)aContext.GetDrawTarget()->GetUserData(&gfxTextContextPaint::sUserDataKey);

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
