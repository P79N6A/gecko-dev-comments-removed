





































#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGTransformList.h"
#include "nsStyleContext.h"
#include "nsINameSpaceManager.h"
#include "nsISVGChildFrame.h"
#include "nsSVGMatrix.h"
#include "nsSVGRect.h"
#include "nsSVGUtils.h"
#include "nsSVGEffects.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGPatternElement.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGPatternFrame.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxPattern.h"
#include "gfxMatrix.h"





nsSVGPatternFrame::nsSVGPatternFrame(nsStyleContext* aContext) :
  nsSVGPatternFrameBase(aContext),
  mLoopFlag(PR_FALSE),
  mNoHRefURI(PR_FALSE)
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPatternFrame)




 void
nsSVGPatternFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGEffects::InvalidateRenderingObservers(this);
  nsSVGPatternFrameBase::DidSetStyleContext(aOldStyleContext);
}

NS_IMETHODIMP
nsSVGPatternFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::patternUnits ||
       aAttribute == nsGkAtoms::patternContentUnits ||
       aAttribute == nsGkAtoms::patternTransform ||
       aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height ||
       aAttribute == nsGkAtoms::preserveAspectRatio ||
       aAttribute == nsGkAtoms::viewBox)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    mNoHRefURI = PR_FALSE;
    
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGPatternFrameBase::AttributeChanged(aNameSpaceID,
                                                 aAttribute, aModType);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSVGPatternFrame::Init(nsIContent* aContent,
                        nsIFrame* aParent,
                        nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGPatternElement> patternElement = do_QueryInterface(aContent);
  NS_ASSERTION(patternElement, "Content is not an SVG pattern");

  return nsSVGPatternFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom*
nsSVGPatternFrame::GetType() const
{
  return nsGkAtoms::svgPatternFrame;
}








gfxMatrix
nsSVGPatternFrame::GetCanvasTM()
{
  if (mCTM) {
    return nsSVGUtils::ConvertSVGMatrixToThebes(mCTM);
  }

  
  if (mSource) {
    
    return mSource->GetCanvasTM();
  }

  
  return gfxMatrix();
}

nsresult
nsSVGPatternFrame::PaintPattern(gfxASurface** surface,
                                gfxMatrix* patternMatrix,
                                nsIFrame *aSource,
                                float aGraphicOpacity,
                                const gfxRect *aOverrideBounds)
{
  










  *surface = nsnull;

  
  nsIFrame *firstKid;
  if (NS_FAILED(GetPatternFirstChild(&firstKid)))
    return NS_ERROR_FAILURE; 

  


















  
  
  gfxRect callerBBox;
  gfxMatrix callerCTM;
  if (NS_FAILED(GetTargetGeometry(&callerCTM,
                                  &callerBBox,
                                  aSource,
                                  aOverrideBounds)))
    return NS_ERROR_FAILURE;

  
  
  gfxMatrix ctm = ConstructCTM(callerBBox, callerCTM, aSource);
  if (ctm.IsSingular()) {
    return NS_ERROR_FAILURE;
  }

  
  nsSVGPatternFrame *patternFrame =
    static_cast<nsSVGPatternFrame*>(firstKid->GetParent());
  patternFrame->mCTM = NS_NewSVGMatrix(ctm);

  
  
  
  gfxRect bbox = GetPatternRect(callerBBox, callerCTM, aSource);

  
  
  *patternMatrix = GetPatternMatrix(bbox, callerBBox, callerCTM);

  
  
  float patternWidth = bbox.Width();
  float patternHeight = bbox.Height();

  PRBool resultOverflows;
  gfxIntSize surfaceSize =
    nsSVGUtils::ConvertToSurfaceSize(gfxSize(patternWidth, patternHeight),
                                     &resultOverflows);

  
  if (surfaceSize.width <= 0 || surfaceSize.height <= 0)
    return NS_ERROR_FAILURE;

  if (resultOverflows ||
      patternWidth != surfaceSize.width ||
      patternHeight != surfaceSize.height) {
    
    nsCOMPtr<nsIDOMSVGMatrix> tempTM, aCTM;
    NS_NewSVGMatrix(getter_AddRefs(tempTM),
                    surfaceSize.width / patternWidth, 0.0f,
                    0.0f, surfaceSize.height / patternHeight,
                    0.0f, 0.0f);
    patternFrame->mCTM->Multiply(tempTM, getter_AddRefs(aCTM));
    aCTM.swap(patternFrame->mCTM);

    
    patternMatrix->Scale(patternWidth / surfaceSize.width,
                         patternHeight / surfaceSize.height);
  }

  nsRefPtr<gfxASurface> tmpSurface =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(surfaceSize,
                                                       gfxASurface::CONTENT_COLOR_ALPHA);
  if (!tmpSurface || tmpSurface->CairoStatus())
    return NS_ERROR_FAILURE;

  nsSVGRenderState tmpState(tmpSurface);
  gfxContext* tmpContext = tmpState.GetGfxContext();

  
  tmpContext->SetOperator(gfxContext::OPERATOR_CLEAR);
  tmpContext->Paint();
  tmpContext->SetOperator(gfxContext::OPERATOR_OVER);

  if (aGraphicOpacity != 1.0f) {
    tmpContext->Save();
    tmpContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }

  
  
  

  if (aSource->IsFrameOfType(nsIFrame::eSVGGeometry)) {
    
    patternFrame->mSource = static_cast<nsSVGGeometryFrame*>(aSource);
  }

  
  
  if (!(patternFrame->GetStateBits() & NS_FRAME_DRAWING_AS_PAINTSERVER)) {
    patternFrame->AddStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);
    for (nsIFrame* kid = firstKid; kid;
         kid = kid->GetNextSibling()) {
      nsSVGUtils::PaintFrameWithEffects(&tmpState, nsnull, kid);
    }
    patternFrame->RemoveStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);
  }

  patternFrame->mSource = nsnull;

  if (aGraphicOpacity != 1.0f) {
    tmpContext->PopGroupToSource();
    tmpContext->Paint(aGraphicOpacity);
    tmpContext->Restore();
  }

  
  tmpSurface.swap(*surface);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGPatternFrame::GetPatternFirstChild(nsIFrame **kid)
{
  
  *kid = mFrames.FirstChild();
  if (*kid)
    return NS_OK;

  
  nsSVGPatternFrame *next = GetReferencedPattern();

  mLoopFlag = PR_TRUE;
  if (!next || next->mLoopFlag) {
    mLoopFlag = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  nsresult rv = next->GetPatternFirstChild(kid);
  mLoopFlag = PR_FALSE;
  return rv;
}

PRUint16
nsSVGPatternFrame::GetPatternUnits()
{
  
  nsSVGPatternElement *patternElement =
    GetPatternWithAttr(nsGkAtoms::patternUnits, mContent);
  return patternElement->mEnumAttributes[nsSVGPatternElement::PATTERNUNITS].GetAnimValue();
}

PRUint16
nsSVGPatternFrame::GetPatternContentUnits()
{
  nsSVGPatternElement *patternElement =
    GetPatternWithAttr(nsGkAtoms::patternContentUnits, mContent);
  return patternElement->mEnumAttributes[nsSVGPatternElement::PATTERNCONTENTUNITS].GetAnimValue();
}

gfxMatrix
nsSVGPatternFrame::GetPatternTransform()
{
  nsSVGPatternElement *patternElement =
    GetPatternWithAttr(nsGkAtoms::patternTransform, mContent);

  static const gfxMatrix identityMatrix;
  if (!patternElement->mPatternTransform) {
    return identityMatrix;
  }
  nsCOMPtr<nsIDOMSVGTransformList> lTrans;
  patternElement->mPatternTransform->GetAnimVal(getter_AddRefs(lTrans));
  nsCOMPtr<nsIDOMSVGMatrix> patternTransform =
    nsSVGTransformList::GetConsolidationMatrix(lTrans);
  if (!patternTransform) {
    return identityMatrix;
  }
  return nsSVGUtils::ConvertSVGMatrixToThebes(patternTransform);
}

const nsSVGViewBox &
nsSVGPatternFrame::GetViewBox()
{
  nsSVGPatternElement *patternElement =
    GetPatternWithAttr(nsGkAtoms::viewBox, mContent);

  return patternElement->mViewBox;
}

const nsSVGPreserveAspectRatio &
nsSVGPatternFrame::GetPreserveAspectRatio()
{
  nsSVGPatternElement *patternElement =
    GetPatternWithAttr(nsGkAtoms::preserveAspectRatio, mContent);

  return patternElement->mPreserveAspectRatio;
}

const nsSVGLength2 *
nsSVGPatternFrame::GetX()
{
  nsSVGPatternElement *pattern = GetPatternWithAttr(nsGkAtoms::x, mContent);
  return &pattern->mLengthAttributes[nsSVGPatternElement::X];
}

const nsSVGLength2 *
nsSVGPatternFrame::GetY()
{
  nsSVGPatternElement *pattern = GetPatternWithAttr(nsGkAtoms::y, mContent);
  return &pattern->mLengthAttributes[nsSVGPatternElement::Y];
}

const nsSVGLength2 *
nsSVGPatternFrame::GetWidth()
{
  nsSVGPatternElement *pattern = GetPatternWithAttr(nsGkAtoms::width, mContent);
  return &pattern->mLengthAttributes[nsSVGPatternElement::WIDTH];
}

const nsSVGLength2 *
nsSVGPatternFrame::GetHeight()
{
  nsSVGPatternElement *pattern = GetPatternWithAttr(nsGkAtoms::height, mContent);
  return &pattern->mLengthAttributes[nsSVGPatternElement::HEIGHT];
}


nsSVGPatternFrame *
nsSVGPatternFrame::GetReferencedPattern()
{
  if (mNoHRefURI)
    return nsnull;

  nsSVGPaintingProperty *property = static_cast<nsSVGPaintingProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    
    nsSVGPatternElement *pattern = static_cast<nsSVGPatternElement *>(mContent);
    nsAutoString href;
    pattern->mStringAttributes[nsSVGPatternElement::HREF].GetAnimValue(href, pattern);
    if (href.IsEmpty()) {
      mNoHRefURI = PR_TRUE;
      return nsnull; 
    }

    
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetPaintingProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nsnull;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nsnull;

  nsIAtom* frameType = result->GetType();
  if (frameType != nsGkAtoms::svgPatternFrame)
    return nsnull;

  return static_cast<nsSVGPatternFrame*>(result);
}

nsSVGPatternElement *
nsSVGPatternFrame::GetPatternWithAttr(nsIAtom *aAttrName, nsIContent *aDefault)
{
  
  
  
  

  if (mContent->HasAttr(kNameSpaceID_None, aAttrName))
    return static_cast<nsSVGPatternElement *>(mContent);

  nsSVGPatternElement *pattern = static_cast<nsSVGPatternElement *>(aDefault);

  nsSVGPatternFrame *next = GetReferencedPattern();
  if (!next)
    return pattern;

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!next->mLoopFlag, "gradient reference loop detected "
                                     "while inheriting attribute!");
  if (!next->mLoopFlag)
    pattern = next->GetPatternWithAttr(aAttrName, aDefault);
  mLoopFlag = PR_FALSE;

  return pattern;
}





gfxRect
nsSVGPatternFrame::GetPatternRect(const gfxRect &aTargetBBox,
                                  const gfxMatrix &aTargetCTM,
                                  nsIFrame *aTarget)
{
  
  PRUint16 type = GetPatternUnits();

  
  float x,y,width,height;

  
  const nsSVGLength2 *tmpX, *tmpY, *tmpHeight, *tmpWidth;
  tmpX = GetX();
  tmpY = GetY();
  tmpHeight = GetHeight();
  tmpWidth = GetWidth();

  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    x = nsSVGUtils::ObjectSpace(aTargetBBox, tmpX);
    y = nsSVGUtils::ObjectSpace(aTargetBBox, tmpY);
    width = nsSVGUtils::ObjectSpace(aTargetBBox, tmpWidth);
    height = nsSVGUtils::ObjectSpace(aTargetBBox, tmpHeight);
  } else {
    float scale = nsSVGUtils::MaxExpansion(aTargetCTM);
    x = nsSVGUtils::UserSpace(aTarget, tmpX) * scale;
    y = nsSVGUtils::UserSpace(aTarget, tmpY) * scale;
    width = nsSVGUtils::UserSpace(aTarget, tmpWidth) * scale;
    height = nsSVGUtils::UserSpace(aTarget, tmpHeight) * scale;
  }

  return gfxRect(x, y, width, height);
}

gfxMatrix
nsSVGPatternFrame::ConstructCTM(const gfxRect &callerBBox,
                                const gfxMatrix &callerCTM,
                                nsIFrame *aTarget)
{
  gfxMatrix tCTM;
  nsSVGSVGElement *ctx = nsnull;
  nsIContent* targetContent = aTarget->GetContent();

  
  if (GetPatternContentUnits() ==
      nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    tCTM.Scale(callerBBox.Width(), callerBBox.Height());
  } else {
    if (targetContent->IsSVG()) {
      ctx = static_cast<nsSVGElement*>(targetContent)->GetCtx();
    }
    float scale = nsSVGUtils::MaxExpansion(callerCTM);
    tCTM.Scale(scale, scale);
  }

  nsSVGPatternElement *patternElement =
    static_cast<nsSVGPatternElement*>(mContent);
  gfxMatrix tm;
  const nsSVGViewBoxRect viewBox = GetViewBox().GetAnimValue();

  if (viewBox.height > 0.0f && viewBox.width > 0.0f) {
    float viewportWidth, viewportHeight, refX, refY;
    if (targetContent->IsSVG()) {
      
      
      
      viewportWidth = GetWidth()->GetAnimValue(ctx);
      viewportHeight = GetHeight()->GetAnimValue(ctx);
      refX = GetX()->GetAnimValue(ctx);
      refY = GetY()->GetAnimValue(ctx);
    } else {
      
      viewportWidth = GetWidth()->GetAnimValue(aTarget);
      viewportHeight = GetHeight()->GetAnimValue(aTarget);
      refX = GetX()->GetAnimValue(aTarget);
      refY = GetY()->GetAnimValue(aTarget);
    }
    gfxMatrix viewBoxTM = nsSVGUtils::GetViewBoxTransform(patternElement,
                                                          viewportWidth, viewportHeight,
                                                          viewBox.x, viewBox.y,
                                                          viewBox.width, viewBox.height,
                                                          GetPreserveAspectRatio());

    gfxPoint ref = viewBoxTM.Transform(gfxPoint(refX, refY));

    tm = viewBoxTM * gfxMatrix().Translate(gfxPoint(-ref.x, -ref.y));
  }
  return tm * tCTM;
}

gfxMatrix
nsSVGPatternFrame::GetPatternMatrix(const gfxRect &bbox,
                                    const gfxRect &callerBBox,
                                    const gfxMatrix &callerCTM)
{
  
  gfxMatrix patternTransform = GetPatternTransform();

  
  float minx = bbox.X();
  float miny = bbox.Y();

  PRUint16 type = GetPatternContentUnits();
  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    minx += callerBBox.X();
    miny += callerBBox.Y();
  }

  float scale = 1.0f / nsSVGUtils::MaxExpansion(callerCTM);
  patternTransform.Scale(scale, scale);
  patternTransform.Translate(gfxPoint(minx, miny));

  return patternTransform;
}

nsresult
nsSVGPatternFrame::GetTargetGeometry(gfxMatrix *aCTM,
                                     gfxRect *aBBox,
                                     nsIFrame *aTarget,
                                     const gfxRect *aOverrideBounds)
{
  *aBBox = aOverrideBounds ? *aOverrideBounds : nsSVGUtils::GetBBox(aTarget);

  
  PRUint16 type = GetPatternUnits();
  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    if (aBBox->Width() <= 0 || aBBox->Height() <= 0) {
      return NS_ERROR_FAILURE;
    }
  }

  
  *aCTM = nsSVGUtils::GetCanvasTM(aTarget);

  
  
  {
    float scale = nsSVGUtils::MaxExpansion(*aCTM);
    if (scale <= 0) {
      return NS_ERROR_FAILURE;
    }
    aBBox->Scale(scale);
  }
  return NS_OK;
}




already_AddRefed<gfxPattern>
nsSVGPatternFrame::GetPaintServerPattern(nsIFrame *aSource,
                                         float aGraphicOpacity,
                                         const gfxRect *aOverrideBounds)
{
  if (aGraphicOpacity == 0.0f) {
    nsRefPtr<gfxPattern> pattern = new gfxPattern(gfxRGBA(0, 0, 0, 0));
    return pattern.forget();
  }

  
  nsRefPtr<gfxASurface> surface;
  gfxMatrix pMatrix;
  nsresult rv = PaintPattern(getter_AddRefs(surface), &pMatrix,
                             aSource, aGraphicOpacity, aOverrideBounds);

  if (NS_FAILED(rv)) {
    return nsnull;
  }

  if (pMatrix.IsSingular()) {
    return nsnull;
  }

  pMatrix.Invert();

  nsRefPtr<gfxPattern> pattern = new gfxPattern(surface);

  if (!pattern || pattern->CairoStatus())
    return nsnull;

  pattern->SetMatrix(pMatrix);
  pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
  return pattern.forget();
}





nsIFrame* NS_NewSVGPatternFrame(nsIPresShell*   aPresShell,
                                nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGPatternFrame(aContext);
}

