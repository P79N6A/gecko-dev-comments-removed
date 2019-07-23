



































#include "nsPresContext.h"
#include "nsSVGUtils.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "nsContentUtils.h"
#include "gfxContext.h"
#include "nsSVGEffects.h"




NS_IMETHODIMP
nsSVGGeometryFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
  AddStateBits((aParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD) |
               NS_STATE_SVG_PROPAGATE_TRANSFORM);
  nsresult rv = nsSVGGeometryFrameBase::Init(aContent, aParent, aPrevInFlow);
  return rv;
}



nsSVGPaintServerFrame *
nsSVGGeometryFrame::GetPaintServer(const nsStyleSVGPaint *aPaint,
                                   nsIAtom *aType)
{
  if (aPaint->mType != eStyleSVGPaintType_Server)
    return nsnull;

  nsSVGPaintingProperty *property =
    nsSVGEffects::GetPaintingProperty(aPaint->mPaint.mPaintServer, this, aType);
  if (!property)
    return nsnull;
  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nsnull;

  nsIAtom *type = result->GetType();
  if (type != nsGkAtoms::svgLinearGradientFrame &&
      type != nsGkAtoms::svgRadialGradientFrame &&
      type != nsGkAtoms::svgPatternFrame)
    return nsnull;

  return static_cast<nsSVGPaintServerFrame*>(result);
}

float
nsSVGGeometryFrame::GetStrokeWidth()
{
  nsSVGElement *ctx = static_cast<nsSVGElement*>
                                 (GetType() == nsGkAtoms::svgGlyphFrame ?
                                     mContent->GetParent() : mContent);

  return
    nsSVGUtils::CoordToFloat(PresContext(),
                             ctx,
                             GetStyleSVG()->mStrokeWidth);
}

nsresult
nsSVGGeometryFrame::GetStrokeDashArray(gfxFloat **aDashes, PRUint32 *aCount)
{
  *aDashes = nsnull;
  *aCount = 0;

  PRUint32 count = GetStyleSVG()->mStrokeDasharrayLength;
  gfxFloat *dashes = nsnull;

  if (count) {
    const nsStyleCoord *dasharray = GetStyleSVG()->mStrokeDasharray;
    nsPresContext *presContext = PresContext();
    gfxFloat totalLength = 0.0f;

    dashes = new gfxFloat[count];
    if (dashes) {
      for (PRUint32 i = 0; i < count; i++) {
        dashes[i] =
          nsSVGUtils::CoordToFloat(presContext,
                                   static_cast<nsSVGElement*>(mContent),
                                   dasharray[i]);
        if (dashes[i] < 0.0f) {
          delete [] dashes;
          return NS_OK;
        }
        totalLength += dashes[i];
      }
    } else {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (totalLength == 0.0f) {
      delete [] dashes;
      return NS_OK;
    }

    *aDashes = dashes;
    *aCount = count;
  }

  return NS_OK;
}

float
nsSVGGeometryFrame::GetStrokeDashoffset()
{
  return
    nsSVGUtils::CoordToFloat(PresContext(),
                             static_cast<nsSVGElement*>(mContent),
                             GetStyleSVG()->mStrokeDashoffset);
}

PRUint16
nsSVGGeometryFrame::GetClipRule()
{
  return GetStyleSVG()->mClipRule;
}

PRBool
nsSVGGeometryFrame::IsClipChild()
{
  nsIContent *node = mContent;

  do {
    
    
    if (node->GetNameSpaceID() != kNameSpaceID_SVG) {
      break;
    }
    if (node->NodeInfo()->Equals(nsGkAtoms::clipPath, kNameSpaceID_SVG)) {
      return PR_TRUE;
    }
    node = node->GetParent();
  } while (node);
    
  return PR_FALSE;
}

static void
SetupCairoColor(gfxContext *aContext, nscolor aRGB, float aOpacity)
{
  aContext->SetColor(gfxRGBA(NS_GET_R(aRGB)/255.0,
                             NS_GET_G(aRGB)/255.0,
                             NS_GET_B(aRGB)/255.0,
                             NS_GET_A(aRGB)/255.0 * aOpacity));
}

float
nsSVGGeometryFrame::MaybeOptimizeOpacity(float aOpacity)
{
  if (nsSVGUtils::CanOptimizeOpacity(this)) {
    aOpacity *= GetStyleDisplay()->mOpacity;
  }
  return aOpacity;
}

PRBool
nsSVGGeometryFrame::SetupCairoFill(gfxContext *aContext)
{
  if (GetStyleSVG()->mFillRule == NS_STYLE_FILL_RULE_EVENODD)
    aContext->SetFillRule(gfxContext::FILL_RULE_EVEN_ODD);
  else
    aContext->SetFillRule(gfxContext::FILL_RULE_WINDING);

  float opacity = MaybeOptimizeOpacity(GetStyleSVG()->mFillOpacity);

  nsSVGPaintServerFrame *ps =
    GetPaintServer(&GetStyleSVG()->mFill, nsGkAtoms::fill);
  if (ps && ps->SetupPaintServer(aContext, this, opacity))
    return PR_TRUE;

  
  
  
  if (GetStyleSVG()->mFill.mType == eStyleSVGPaintType_Server) {
    SetupCairoColor(aContext,
                    GetStyleSVG()->mFill.mFallbackColor,
                    opacity);
  } else
    SetupCairoColor(aContext,
                    GetStyleSVG()->mFill.mPaint.mColor,
                    opacity);

  return PR_TRUE;
}

PRBool
nsSVGGeometryFrame::SetupCairoStrokeGeometry(gfxContext *aContext)
{
  const nsStyleSVG* style = GetStyleSVG();
  if (style->mStroke.mType == eStyleSVGPaintType_None)
    return PR_FALSE;
  
  float width = GetStrokeWidth();
  if (width <= 0)
    return PR_FALSE;
  aContext->SetLineWidth(width);

  switch (style->mStrokeLinecap) {
  case NS_STYLE_STROKE_LINECAP_BUTT:
    aContext->SetLineCap(gfxContext::LINE_CAP_BUTT);
    break;
  case NS_STYLE_STROKE_LINECAP_ROUND:
    aContext->SetLineCap(gfxContext::LINE_CAP_ROUND);
    break;
  case NS_STYLE_STROKE_LINECAP_SQUARE:
    aContext->SetLineCap(gfxContext::LINE_CAP_SQUARE);
    break;
  }

  aContext->SetMiterLimit(style->mStrokeMiterlimit);

  switch (style->mStrokeLinejoin) {
  case NS_STYLE_STROKE_LINEJOIN_MITER:
    aContext->SetLineJoin(gfxContext::LINE_JOIN_MITER);
    break;
  case NS_STYLE_STROKE_LINEJOIN_ROUND:
    aContext->SetLineJoin(gfxContext::LINE_JOIN_ROUND);
    break;
  case NS_STYLE_STROKE_LINEJOIN_BEVEL:
    aContext->SetLineJoin(gfxContext::LINE_JOIN_BEVEL);
    break;
  }

  return PR_TRUE;
}

PRBool
nsSVGGeometryFrame::SetupCairoStrokeHitGeometry(gfxContext *aContext)
{
  if (!SetupCairoStrokeGeometry(aContext))
    return PR_FALSE;

  gfxFloat *dashArray;
  PRUint32 count;
  GetStrokeDashArray(&dashArray, &count);
  if (count > 0) {
    aContext->SetDash(dashArray, count, GetStrokeDashoffset());
    delete [] dashArray;
  }
  return PR_TRUE;
}

PRBool
nsSVGGeometryFrame::SetupCairoStroke(gfxContext *aContext)
{
  if (!SetupCairoStrokeHitGeometry(aContext))
    return PR_FALSE;

  float opacity = MaybeOptimizeOpacity(GetStyleSVG()->mStrokeOpacity);

  nsSVGPaintServerFrame *ps =
    GetPaintServer(&GetStyleSVG()->mStroke, nsGkAtoms::stroke);
  if (ps && ps->SetupPaintServer(aContext, this, opacity))
    return PR_TRUE;

  
  
  
  if (GetStyleSVG()->mStroke.mType == eStyleSVGPaintType_Server) {
    SetupCairoColor(aContext,
                    GetStyleSVG()->mStroke.mFallbackColor,
                    opacity);
  } else
    SetupCairoColor(aContext,
                    GetStyleSVG()->mStroke.mPaint.mColor,
                    opacity);

  return PR_TRUE;
}
