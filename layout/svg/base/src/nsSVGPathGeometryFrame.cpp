





































#include "nsSVGPathGeometryFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGMarkerFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGEffects.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGRect.h"
#include "nsSVGPathGeometryElement.h"
#include "gfxContext.h"




nsIFrame*
NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell,
                           nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGPathGeometryFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPathGeometryFrame)




NS_QUERYFRAME_HEAD(nsSVGPathGeometryFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGPathGeometryFrameBase)




NS_IMETHODIMP
nsSVGPathGeometryFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                         nsIAtom*        aAttribute,
                                         PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (static_cast<nsSVGPathGeometryElement*>
                  (mContent)->AttributeDefinesGeometry(aAttribute) ||
       aAttribute == nsGkAtoms::transform))
    nsSVGUtils::UpdateGraphic(this);

  return NS_OK;
}

 void
nsSVGPathGeometryFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGPathGeometryFrameBase::DidSetStyleContext(aOldStyleContext);

  
  
  
  
  

  nsSVGUtils::UpdateGraphic(this);
}

nsIAtom *
nsSVGPathGeometryFrame::GetType() const
{
  return nsGkAtoms::svgPathGeometryFrame;
}




NS_IMETHODIMP
nsSVGPathGeometryFrame::PaintSVG(nsSVGRenderState *aContext,
                                 const nsIntRect *aDirtyRect)
{
  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;

  
  Render(aContext);

  if (static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {
    MarkerProperties properties = GetMarkerProperties(this);
      
    if (properties.MarkersExist()) {
      float strokeWidth = GetStrokeWidth();
        
      nsTArray<nsSVGMark> marks;
      static_cast<nsSVGPathGeometryElement*>
                 (mContent)->GetMarkPoints(&marks);
        
      PRUint32 num = marks.Length();

      if (num) {
        nsSVGMarkerFrame *frame = properties.GetMarkerStartFrame();
        if (frame)
          frame->PaintMark(aContext, this, &marks[0], strokeWidth);

        frame = properties.GetMarkerMidFrame();
        if (frame) {
          for (PRUint32 i = 1; i < num - 1; i++)
            frame->PaintMark(aContext, this, &marks[i], strokeWidth);
        }

        frame = properties.GetMarkerEndFrame();
        if (frame)
          frame->PaintMark(aContext, this, &marks[num-1], strokeWidth);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGPathGeometryFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  PRUint16 fillRule, mask;
  
  
  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD) {
    NS_ASSERTION(IsClipChild(), "should be in clipPath but we're not");
    mask = HITTEST_MASK_FILL;
    fillRule = GetClipRule();
  } else {
    mask = GetHittestMask();
    if (!mask || (!(mask & HITTEST_MASK_FORCE_TEST) &&
                  !mRect.Contains(aPoint)))
      return nsnull;
    fillRule = GetStyleSVG()->mFillRule;
  }

  PRBool isHit = PR_FALSE;

  gfxContext context(nsSVGUtils::GetThebesComputationalSurface());

  GeneratePath(&context);
  gfxPoint userSpacePoint =
    context.DeviceToUser(gfxPoint(PresContext()->AppUnitsToGfxUnits(aPoint.x),
                                  PresContext()->AppUnitsToGfxUnits(aPoint.y)));

  if (fillRule == NS_STYLE_FILL_RULE_EVENODD)
    context.SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
  else
    context.SetFillRule(gfxContext::FILL_RULE_WINDING);

  if (mask & HITTEST_MASK_FILL)
    isHit = context.PointInFill(userSpacePoint);
  if (!isHit && (mask & HITTEST_MASK_STROKE)) {
    SetupCairoStrokeHitGeometry(&context);
    isHit = context.PointInStroke(userSpacePoint);
  }

  if (isHit && nsSVGUtils::HitTestClip(this, aPoint))
    return this;

  return nsnull;
}

NS_IMETHODIMP_(nsRect)
nsSVGPathGeometryFrame::GetCoveredRegion()
{
  
  
  

  if (static_cast<nsSVGPathGeometryElement*>(mContent)->IsMarkable()) {
    MarkerProperties properties = GetMarkerProperties(this);

    if (!properties.MarkersExist())
      return mRect;

    nsRect rect(mRect);

    float strokeWidth = GetStrokeWidth();

    nsTArray<nsSVGMark> marks;
    static_cast<nsSVGPathGeometryElement*>(mContent)->GetMarkPoints(&marks);

    PRUint32 num = marks.Length();

    if (num) {
      nsSVGMarkerFrame *frame = properties.GetMarkerStartFrame();
      if (frame) {
        nsRect mark = frame->RegionMark(this, &marks[0], strokeWidth);
        rect.UnionRect(rect, mark);
      }

      frame = properties.GetMarkerMidFrame();
      if (frame) {
        for (PRUint32 i = 1; i < num - 1; i++) {
          nsRect mark = frame->RegionMark(this, &marks[i], strokeWidth);
          rect.UnionRect(rect, mark);
        }
      }

      frame = properties.GetMarkerEndFrame();
      if (frame) {
        nsRect mark = frame->RegionMark(this, &marks[num-1], strokeWidth);
        rect.UnionRect(rect, mark);
      }
    }

    return rect;
  }

  return mRect;
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::UpdateCoveredRegion()
{
  mRect.Empty();

  gfxContext context(nsSVGUtils::GetThebesComputationalSurface());

  GeneratePath(&context);
  context.IdentityMatrix();

  gfxRect extent = context.GetUserPathExtent();

  
  
  
  
  
  
  
  
  
  
  

  if (HasStroke()) {
    SetupCairoStrokeGeometry(&context);
    extent = nsSVGUtils::PathExtentsToMaxStrokeExtents(extent, this);
  } else if (GetStyleSVG()->mFill.mType == eStyleSVGPaintType_None) {
    extent = gfxRect(0, 0, 0, 0);
  }

  if (!extent.IsEmpty()) {
    mRect = nsSVGUtils::ToAppPixelRect(PresContext(), extent);
  }

  
  mRect = GetCoveredRegion();

  return NS_OK;
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::InitialUpdate()
{
  NS_ASSERTION(GetStateBits() & NS_FRAME_FIRST_REFLOW,
               "Yikes! We've been called already! Hopefully we weren't called "
               "before our nsSVGOuterSVGFrame's initial Reflow()!!!");

  nsSVGUtils::UpdateGraphic(this);

  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "We don't actually participate in reflow");
  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
  return NS_OK;
}

void
nsSVGPathGeometryFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (!(aFlags & SUPPRESS_INVALIDATION)) {
    nsSVGUtils::UpdateGraphic(this);
  }
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::NotifyRedrawSuspended()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::NotifyRedrawUnsuspended()
{
  if (GetStateBits() & NS_STATE_SVG_DIRTY)
    nsSVGUtils::UpdateGraphic(this);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGPathGeometryFrame::SetMatrixPropagation(PRBool aPropagate)
{
  if (aPropagate) {
    AddStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  } else {
    RemoveStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  }
  return NS_OK;
}

PRBool
nsSVGPathGeometryFrame::GetMatrixPropagation()
{
  return (GetStateBits() & NS_STATE_SVG_PROPAGATE_TRANSFORM) != 0;
}

gfxRect
nsSVGPathGeometryFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace)
{
  if (aToBBoxUserspace.IsSingular()) {
    
    return gfxRect(0.0, 0.0, 0.0, 0.0);
  }
  gfxContext context(nsSVGUtils::GetThebesComputationalSurface());
  GeneratePath(&context, &aToBBoxUserspace);
  context.IdentityMatrix();
  return context.GetUserPathExtent();
}




gfxMatrix
nsSVGPathGeometryFrame::GetCanvasTM()
{
  NS_ASSERTION(mParent, "null parent");

  nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
  nsSVGGraphicElement *content = static_cast<nsSVGGraphicElement*>(mContent);

  return content->PrependLocalTransformTo(parent->GetCanvasTM());
}




nsSVGPathGeometryFrame::MarkerProperties
nsSVGPathGeometryFrame::GetMarkerProperties(nsSVGPathGeometryFrame *aFrame)
{
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "aFrame should be first continuation");

  MarkerProperties result;
  const nsStyleSVG *style = aFrame->GetStyleSVG();
  result.mMarkerStart = nsSVGEffects::GetMarkerProperty(
                          style->mMarkerStart, aFrame, nsGkAtoms::marker_start);
  result.mMarkerMid = nsSVGEffects::GetMarkerProperty(
                        style->mMarkerMid, aFrame, nsGkAtoms::marker_mid);
  result.mMarkerEnd = nsSVGEffects::GetMarkerProperty(
                        style->mMarkerEnd, aFrame, nsGkAtoms::marker_end);
  return result;
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerStartFrame()
{
  if (!mMarkerStart)
    return nsnull;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerStart->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nsnull));
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerMidFrame()
{
  if (!mMarkerMid)
    return nsnull;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerMid->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nsnull));
}

nsSVGMarkerFrame *
nsSVGPathGeometryFrame::MarkerProperties::GetMarkerEndFrame()
{
  if (!mMarkerEnd)
    return nsnull;
  return static_cast<nsSVGMarkerFrame *>
    (mMarkerEnd->GetReferencedFrame(nsGkAtoms::svgMarkerFrame, nsnull));
}

void
nsSVGPathGeometryFrame::Render(nsSVGRenderState *aContext)
{
  gfxContext *gfx = aContext->GetGfxContext();

  PRUint16 renderMode = aContext->GetRenderMode();

  
  gfx->Save();

  GeneratePath(gfx);

  switch (GetStyleSVG()->mShapeRendering) {
  case NS_STYLE_SHAPE_RENDERING_OPTIMIZESPEED:
  case NS_STYLE_SHAPE_RENDERING_CRISPEDGES:
    gfx->SetAntialiasMode(gfxContext::MODE_ALIASED);
    break;
  default:
    gfx->SetAntialiasMode(gfxContext::MODE_COVERAGE);
    break;
  }

  if (renderMode != nsSVGRenderState::NORMAL) {
    if (GetClipRule() == NS_STYLE_FILL_RULE_EVENODD)
      gfx->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
    else
      gfx->SetFillRule(gfxContext::FILL_RULE_WINDING);

    if (renderMode == nsSVGRenderState::CLIP_MASK) {
      gfx->SetColor(gfxRGBA(1.0f, 1.0f, 1.0f, 1.0f));
      gfx->Fill();
      gfx->NewPath();
    }
    gfx->Restore();

    return;
  }

  if (SetupCairoFill(gfx)) {
    gfx->Fill();
  }

  if (SetupCairoStroke(gfx)) {
    gfx->Stroke();
  }

  gfx->NewPath();

  gfx->Restore();
}

void
nsSVGPathGeometryFrame::GeneratePath(gfxContext* aContext,
                                     const gfxMatrix *aOverrideTransform)
{
  gfxMatrix matrix;
  if (aOverrideTransform) {
    matrix = *aOverrideTransform;
  } else {
    matrix = GetCanvasTM();
  }

  if (matrix.IsSingular()) {
    aContext->IdentityMatrix();
    aContext->NewPath();
    return;
  }

  aContext->Multiply(matrix);

  aContext->NewPath();
  static_cast<nsSVGPathGeometryElement*>(mContent)->ConstructPath(aContext);
}

PRUint16
nsSVGPathGeometryFrame::GetHittestMask()
{
  PRUint16 mask = 0;

  switch(GetStyleVisibility()->mPointerEvents) {
    case NS_STYLE_POINTER_EVENTS_NONE:
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED:
    case NS_STYLE_POINTER_EVENTS_AUTO:
      if (GetStyleVisibility()->IsVisible()) {
        if (GetStyleSVG()->mFill.mType != eStyleSVGPaintType_None)
          mask |= HITTEST_MASK_FILL;
        if (GetStyleSVG()->mStroke.mType != eStyleSVGPaintType_None)
          mask |= HITTEST_MASK_STROKE;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEFILL:
      if (GetStyleVisibility()->IsVisible()) {
        mask |= HITTEST_MASK_FILL | HITTEST_MASK_FORCE_TEST;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLESTROKE:
      if (GetStyleVisibility()->IsVisible()) {
        mask |= HITTEST_MASK_STROKE | HITTEST_MASK_FORCE_TEST;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLE:
      if (GetStyleVisibility()->IsVisible()) {
        mask |=
          HITTEST_MASK_FILL |
          HITTEST_MASK_STROKE |
          HITTEST_MASK_FORCE_TEST;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_PAINTED:
      if (GetStyleSVG()->mFill.mType != eStyleSVGPaintType_None)
        mask |= HITTEST_MASK_FILL;
      if (GetStyleSVG()->mStroke.mType != eStyleSVGPaintType_None)
        mask |= HITTEST_MASK_STROKE;
      break;
    case NS_STYLE_POINTER_EVENTS_FILL:
      mask |= HITTEST_MASK_FILL | HITTEST_MASK_FORCE_TEST;
      break;
    case NS_STYLE_POINTER_EVENTS_STROKE:
      mask |= HITTEST_MASK_STROKE | HITTEST_MASK_FORCE_TEST;
      break;
    case NS_STYLE_POINTER_EVENTS_ALL:
      mask |=
        HITTEST_MASK_FILL |
        HITTEST_MASK_STROKE |
        HITTEST_MASK_FORCE_TEST;
      break;
    default:
      NS_ERROR("not reached");
      break;
  }

  return mask;
}
