





































#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGTransformList.h"
#include "nsStyleContext.h"
#include "nsINameSpaceManager.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGRect.h"
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

#ifdef DEBUG_scooter
static void printCTM(char *msg, gfxMatrix aCTM);
static void printCTM(char *msg, nsIDOMSVGMatrix *aCTM);
static void printRect(char *msg, nsIDOMSVGRect *aRect);
#endif




nsSVGPatternFrame::nsSVGPatternFrame(nsStyleContext* aContext) :
  nsSVGPatternFrameBase(aContext),
  mLoopFlag(PR_FALSE), mPaintLoopFlag(PR_FALSE),
  mNoHRefURI(PR_FALSE)
{
}




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
    
    DeleteProperty(nsGkAtoms::href);
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
                                nsSVGGeometryFrame *aSource,
                                float aGraphicOpacity)
{
  










  *surface = nsnull;

  
  nsIFrame *firstKid;
  if (NS_FAILED(GetPatternFirstChild(&firstKid)))
    return NS_ERROR_FAILURE; 

  


















  
  
  nsSVGElement *callerContent;
  nsCOMPtr<nsIDOMSVGRect> callerBBox;
  nsCOMPtr<nsIDOMSVGMatrix> callerCTM;
  if (NS_FAILED(GetCallerGeometry(getter_AddRefs(callerCTM),
                                  getter_AddRefs(callerBBox),
                                  &callerContent, aSource)))
    return NS_ERROR_FAILURE;

  
  
  if (NS_FAILED(ConstructCTM(getter_AddRefs(mCTM), callerBBox, callerCTM)))
    return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIDOMSVGRect> bbox;
  if (NS_FAILED(GetPatternRect(getter_AddRefs(bbox),
                               callerBBox, callerCTM,
                               callerContent)))
    return NS_ERROR_FAILURE;

  
  
  *patternMatrix = GetPatternMatrix(bbox, callerBBox, callerCTM);

#ifdef DEBUG_scooter
  printRect("Geometry Rect: ", callerBBox);
  printRect("Pattern Rect: ", bbox);
  printCTM("Pattern TM ", *patternMatrix);
  printCTM("Child TM ", mCTM);
#endif

  
  
  float patternWidth, patternHeight;
  bbox->GetWidth(&patternWidth);
  bbox->GetHeight(&patternHeight);

  PRBool resultOverflows;
  gfxIntSize surfaceSize =
    nsSVGUtils::ConvertToSurfaceSize(gfxSize(patternWidth, patternHeight),
                                     &resultOverflows);

  
  if (surfaceSize.width <= 0 || surfaceSize.height <= 0)
    return NS_ERROR_FAILURE;

  if (resultOverflows) {
    
    nsCOMPtr<nsIDOMSVGMatrix> tempTM, aCTM;
    NS_NewSVGMatrix(getter_AddRefs(tempTM),
                    surfaceSize.width / patternWidth, 0.0f,
                    0.0f, surfaceSize.height / patternHeight,
                    0.0f, 0.0f);
    mCTM->Multiply(tempTM, getter_AddRefs(aCTM));
    aCTM.swap(mCTM);

    
    patternMatrix->Scale(patternWidth / surfaceSize.width,
                         patternHeight / surfaceSize.height);
  }

#ifdef DEBUG_scooter
  printf("Creating %dX%d surface\n", int(surfaceSize.width), int(surfaceSize.height));
#endif

  nsRefPtr<gfxASurface> tmpSurface =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(surfaceSize,
                                                       gfxASurface::ImageFormatARGB32);
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

  
  
  

  
  mSource = aSource;

  
  
  if (!mPaintLoopFlag) {
    mPaintLoopFlag = PR_TRUE;
    for (nsIFrame* kid = firstKid; kid;
         kid = kid->GetNextSibling()) {
      nsSVGUtils::PaintFrameWithEffects(&tmpState, nsnull, kid);
    }
    mPaintLoopFlag = PR_FALSE;
  }

  mSource = nsnull;

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

  gfxMatrix matrix;
  nsCOMPtr<nsIDOMSVGTransformList> lTrans;
  patternElement->mPatternTransform->GetAnimVal(getter_AddRefs(lTrans));
  nsCOMPtr<nsIDOMSVGMatrix> patternTransform =
    nsSVGTransformList::GetConsolidationMatrix(lTrans);
  if (patternTransform) {
    matrix = nsSVGUtils::ConvertSVGMatrixToThebes(patternTransform);
  }
  return matrix;
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

  nsSVGPaintingProperty *property =
    static_cast<nsSVGPaintingProperty*>(GetProperty(nsGkAtoms::href));

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

    property = nsSVGEffects::GetPaintingProperty(targetURI, this, nsGkAtoms::href);
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





nsresult
nsSVGPatternFrame::GetPatternRect(nsIDOMSVGRect **patternRect,
                                  nsIDOMSVGRect *bbox,
                                  nsIDOMSVGMatrix *callerCTM,
                                  nsSVGElement *content)
{
  
  PRUint16 type = GetPatternUnits();

  
  float x,y,width,height;

  
  const nsSVGLength2 *tmpX, *tmpY, *tmpHeight, *tmpWidth;
  tmpX = GetX();
  tmpY = GetY();
  tmpHeight = GetHeight();
  tmpWidth = GetWidth();

  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    x = nsSVGUtils::ObjectSpace(bbox, tmpX);
    y = nsSVGUtils::ObjectSpace(bbox, tmpY);
    width = nsSVGUtils::ObjectSpace(bbox, tmpWidth);
    height = nsSVGUtils::ObjectSpace(bbox, tmpHeight);
  } else {
    float scale = nsSVGUtils::MaxExpansion(callerCTM);
    x = nsSVGUtils::UserSpace(content, tmpX) * scale;
    y = nsSVGUtils::UserSpace(content, tmpY) * scale;
    width = nsSVGUtils::UserSpace(content, tmpWidth) * scale;
    height = nsSVGUtils::UserSpace(content, tmpHeight) * scale;
  }

  return NS_NewSVGRect(patternRect, x, y, width, height);
}

static float
GetLengthValue(const nsSVGLength2 *aLength)
{
  return aLength->GetAnimValue(static_cast<nsSVGSVGElement*>(nsnull));
}

nsresult
nsSVGPatternFrame::ConstructCTM(nsIDOMSVGMatrix **aCTM,
                                nsIDOMSVGRect *callerBBox,
                                nsIDOMSVGMatrix *callerCTM)
{
  nsCOMPtr<nsIDOMSVGMatrix> tCTM, tempTM;

  
  
  PRUint16 type = GetPatternContentUnits();

  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    
    float width, height;
    callerBBox->GetWidth(&width);
    callerBBox->GetHeight(&height);
    NS_NewSVGMatrix(getter_AddRefs(tCTM), width, 0.0f, 0.0f,
                    height, 0.0f, 0.0f);
  } else {
    float scale = nsSVGUtils::MaxExpansion(callerCTM);
    NS_NewSVGMatrix(getter_AddRefs(tCTM), scale, 0, 0, scale, 0, 0);
  }

  const nsSVGViewBoxRect viewBox = GetViewBox().GetAnimValue();

  if (viewBox.height > 0.0f && viewBox.width > 0.0f) {

    float viewportWidth = GetLengthValue(GetWidth());
    float viewportHeight = GetLengthValue(GetHeight());
    float refX = GetLengthValue(GetX());
    float refY = GetLengthValue(GetY());

    tempTM = nsSVGUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                             viewBox.x + refX, viewBox.y + refY,
                                             viewBox.width, viewBox.height,
                                             GetPreserveAspectRatio(),
                                             PR_TRUE);

  } else {
    
    NS_NewSVGMatrix(getter_AddRefs(tempTM));
  }
  tCTM->Multiply(tempTM, aCTM);
  return NS_OK;
}

gfxMatrix
nsSVGPatternFrame::GetPatternMatrix(nsIDOMSVGRect *bbox,
                                    nsIDOMSVGRect *callerBBox,
                                    nsIDOMSVGMatrix *callerCTM)
{
  
  gfxMatrix patternTransform = GetPatternTransform();

  
  float minx, miny;
  bbox->GetX(&minx);
  bbox->GetY(&miny);

  PRUint16 type = GetPatternContentUnits();
  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    float x, y;
    callerBBox->GetX(&x);
    callerBBox->GetY(&y);
    minx += x;
    miny += y;
  }

  float scale = 1.0f / nsSVGUtils::MaxExpansion(callerCTM);
  patternTransform.Scale(scale, scale);
  patternTransform.Translate(gfxPoint(minx, miny));

  return patternTransform;
}

nsresult
nsSVGPatternFrame::GetCallerGeometry(nsIDOMSVGMatrix **aCTM,
                                     nsIDOMSVGRect **aBBox,
                                     nsSVGElement **aContent,
                                     nsSVGGeometryFrame *aSource)
{
  *aCTM = nsnull;
  *aBBox = nsnull;
  *aContent = nsnull;

  
  
  
  
  nsIAtom *callerType = aSource->GetType();
  if (callerType ==  nsGkAtoms::svgGlyphFrame) {
    *aContent = static_cast<nsSVGElement*>
                           (aSource->GetContent()->GetParent());
  } else {
    *aContent = static_cast<nsSVGElement*>(aSource->GetContent());
  }
  NS_ASSERTION(aContent,"Caller does not have any content!");
  if (!aContent)
    return NS_ERROR_FAILURE;

  if (callerType == nsGkAtoms::svgGlyphFrame) {
    *aBBox = nsSVGUtils::GetBBox(aSource->GetParent()).get();
  } else {
    *aBBox = nsSVGUtils::GetBBox(aSource).get();
  }
  
  
  PRUint16 type = GetPatternUnits();
  if (type == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    float width, height;
    (*aBBox)->GetWidth(&width);
    (*aBBox)->GetHeight(&height);
    if (width <= 0 || height <= 0) {
      return NS_ERROR_FAILURE;
    }
  }

  
  *aCTM = NS_NewSVGMatrix(aSource->GetCanvasTM()).get();

  
  
  {
    float x, y, width, height;
    (*aBBox)->GetX(&x);
    (*aBBox)->GetY(&y);
    (*aBBox)->GetWidth(&width);
    (*aBBox)->GetHeight(&height);
    float scale = nsSVGUtils::MaxExpansion(*aCTM);
#ifdef DEBUG_scooter
    fprintf(stderr, "pattern scale %f\n", scale);
    fprintf(stderr, "x,y,width,height: %f %f %f %f\n", x, y, width, height);
#endif
    x *= scale;
    y *= scale;
    width *= scale;
    height *= scale;
    (*aBBox)->SetX(x);
    (*aBBox)->SetY(y);
    (*aBBox)->SetWidth(width);
    (*aBBox)->SetHeight(height);
  }
  return NS_OK;
}




PRBool
nsSVGPatternFrame::SetupPaintServer(gfxContext *aContext,
                                    nsSVGGeometryFrame *aSource,
                                    float aGraphicOpacity)
{
  if (aGraphicOpacity == 0.0f) {
    aContext->SetColor(gfxRGBA(0, 0, 0, 0));
    return PR_TRUE;
  }

  gfxMatrix matrix = aContext->CurrentMatrix();

  
  nsRefPtr<gfxASurface> surface;
  gfxMatrix pMatrix;
  aContext->IdentityMatrix();
  nsresult rv = PaintPattern(getter_AddRefs(surface), &pMatrix,
                             aSource, aGraphicOpacity);

  aContext->SetMatrix(matrix);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  if (pMatrix.IsSingular()) {
    return PR_FALSE;
  }

  pMatrix.Invert();

  nsRefPtr<gfxPattern> pattern = new gfxPattern(surface);

  if (!pattern || pattern->CairoStatus())
    return PR_FALSE;

  pattern->SetMatrix(pMatrix);
  pattern->SetExtend(gfxPattern::EXTEND_REPEAT);

  aContext->SetPattern(pattern);

  return PR_TRUE;
}





nsIFrame* NS_NewSVGPatternFrame(nsIPresShell*   aPresShell,
                                nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGPatternFrame(aContext);
}

#ifdef DEBUG_scooter
static void printCTM(char *msg, gfxMatrix aCTM)
{
  printf("%s {%f,%f,%f,%f,%f,%f}\n", msg,
         aCTM.xx, aCTM.yx, aCTM.xy, aCTM.yy, aCTM.x0, aCTM.y0);
}

static void printCTM(char *msg, nsIDOMSVGMatrix *aCTM)
{
  float a,b,c,d,e,f;
  aCTM->GetA(&a);
  aCTM->GetB(&b);
  aCTM->GetC(&c);
  aCTM->GetD(&d);
  aCTM->GetE(&e);
  aCTM->GetF(&f);
  printf("%s {%f,%f,%f,%f,%f,%f}\n",msg,a,b,c,d,e,f);
}

static void printRect(char *msg, nsIDOMSVGRect *aRect)
{
  float x,y,width,height;
  aRect->GetX(&x);
  aRect->GetY(&y);
  aRect->GetWidth(&width);
  aRect->GetHeight(&height);
  printf("%s {%f,%f,%f,%f}\n",msg,x,y,width,height);
}
#endif
