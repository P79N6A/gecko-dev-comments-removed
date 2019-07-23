





































#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGTransformList.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsStyleContext.h"
#include "nsINameSpaceManager.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsSVGRect.h"
#include "nsSVGUtils.h"
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




nsSVGPatternFrame::nsSVGPatternFrame(nsStyleContext* aContext,
                                     nsIDOMSVGURIReference *aRef) :
  nsSVGPatternFrameBase(aContext),
  mNextPattern(nsnull),
  mLoopFlag(PR_FALSE)
{
  if (aRef) {
    
    aRef->GetHref(getter_AddRefs(mHref));
  }
}

nsSVGPatternFrame::~nsSVGPatternFrame()
{
  WillModify(mod_die);
  if (mNextPattern)
    mNextPattern->RemoveObserver(this);

  
  DidModify(mod_die);
}




NS_INTERFACE_MAP_BEGIN(nsSVGPatternFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPatternFrameBase)



NS_IMETHODIMP
nsSVGPatternFrame::WillModifySVGObservable(nsISVGValue* observable,
                                            modificationType aModType)
{
  WillModify(aModType);
  return NS_OK;
}
                                                                                
NS_IMETHODIMP
nsSVGPatternFrame::DidModifySVGObservable(nsISVGValue* observable, 
                                          nsISVGValue::modificationType aModType)
{
  nsIFrame *pattern = nsnull;
  CallQueryInterface(observable, &pattern);
  
  if (mNextPattern && aModType == nsISVGValue::mod_die && pattern) {
    
    if (mNextPattern == pattern) {
      mNextPattern = nsnull;
    }
  }
  
  DidModify(aModType);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGPatternFrame::DidSetStyleContext()
{
  WillModify();
  DidModify();
  return NS_OK;
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
    WillModify();
    DidModify();
    return NS_OK;
  } 

  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    if (mNextPattern)
      mNextPattern->RemoveObserver(this);
    mNextPattern = nsnull;
    WillModify();
    DidModify();
    return NS_OK;
  }

  return nsSVGPatternFrameBase::AttributeChanged(aNameSpaceID,
                                                 aAttribute, aModType);
}

nsIAtom*
nsSVGPatternFrame::GetType() const
{
  return nsGkAtoms::svgPatternFrame;
}








already_AddRefed<nsIDOMSVGMatrix> 
nsSVGPatternFrame::GetCanvasTM() {
  nsIDOMSVGMatrix *rCTM;
  
  if (mCTM) {
    rCTM = mCTM;
    NS_IF_ADDREF(rCTM);
  } else {
    
    if (mSource) {
      
      mSource->GetCanvasTM(&rCTM);
    } else {
      
      
      NS_NewSVGMatrix(&rCTM);
    }
  }
  return rCTM;  
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

  
  
  if (NS_FAILED(ConstructCTM(getter_AddRefs(mCTM), callerBBox)))
    return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIDOMSVGRect> bbox;
  if (NS_FAILED(GetPatternRect(getter_AddRefs(bbox), callerBBox, 
                               callerContent)))
    return NS_ERROR_FAILURE;

  
  
  *patternMatrix = GetPatternMatrix(bbox, callerBBox, callerCTM);

#ifdef DEBUG_scooter
  printRect("Bounding Rect: ",bbox);
  printCTM("Pattern TM ",*patternMatrix);
  printCTM("Child TM ",mCTM);
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
  if (!tmpSurface)
    return NS_ERROR_FAILURE;

  gfxContext tmpContext(tmpSurface);
  nsSVGRenderState tmpState(&tmpContext);

  
  tmpContext.SetOperator(gfxContext::OPERATOR_CLEAR);
  tmpContext.Paint();
  tmpContext.SetOperator(gfxContext::OPERATOR_OVER);

  if (aGraphicOpacity != 1.0f) {
    tmpContext.Save();
    tmpContext.PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }

  
  
  

  
  mSource = aSource;

  for (nsIFrame* kid = firstKid; kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintChildWithEffects(&tmpState, nsnull, kid);
  }
  mSource = nsnull;

  if (aGraphicOpacity != 1.0f) {
    tmpContext.PopGroupToSource();
    tmpContext.Paint(aGraphicOpacity);
    tmpContext.Restore();
  }

  
  tmpSurface.swap(*surface);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGPatternFrame::GetPatternFirstChild(nsIFrame **kid)
{
  nsresult rv = NS_OK;

  
  if (!(*kid = mFrames.FirstChild())) {
    
    if (checkURITarget())
      rv = mNextPattern->GetPatternFirstChild(kid);
    else
      rv = NS_ERROR_FAILURE; 
  }
  mLoopFlag = PR_FALSE;
  return rv;
}

PRUint16
nsSVGPatternFrame::GetPatternUnits()
{
  PRUint16 rv;

  
  if (!checkURITarget(nsGkAtoms::patternUnits)) {
    
    nsSVGPatternElement *patternElement = NS_STATIC_CAST(nsSVGPatternElement*,
                                                         mContent);
    patternElement->mPatternUnits->GetAnimVal(&rv);
  } else {
    
    rv = mNextPattern->GetPatternUnits();
  }  
  mLoopFlag = PR_FALSE;
  return rv;
}

PRUint16
nsSVGPatternFrame::GetPatternContentUnits()
{
  PRUint16 rv;

  
  if (!checkURITarget(nsGkAtoms::patternContentUnits)) {
    
    nsSVGPatternElement *patternElement = NS_STATIC_CAST(nsSVGPatternElement*,
                                                         mContent);
    patternElement->mPatternContentUnits->GetAnimVal(&rv);
  } else {
    
    rv = mNextPattern->GetPatternContentUnits();
  }  
  mLoopFlag = PR_FALSE;
  return rv;
}

gfxMatrix
nsSVGPatternFrame::GetPatternTransform()
{
  gfxMatrix matrix;
  
  if (!checkURITarget(nsGkAtoms::patternTransform)) {
    
    nsSVGPatternElement *patternElement = NS_STATIC_CAST(nsSVGPatternElement*,
                                                         mContent);
    nsCOMPtr<nsIDOMSVGTransformList> lTrans;
    patternElement->mPatternTransform->GetAnimVal(getter_AddRefs(lTrans));
    nsCOMPtr<nsIDOMSVGMatrix> patternTransform =
      nsSVGTransformList::GetConsolidationMatrix(lTrans);
    if (patternTransform) {
      matrix = nsSVGUtils::ConvertSVGMatrixToThebes(patternTransform);
    }
  } else {
    
    matrix = mNextPattern->GetPatternTransform();
  }
  mLoopFlag = PR_FALSE;

  return matrix;
}

NS_IMETHODIMP
nsSVGPatternFrame::GetViewBox(nsIDOMSVGRect **aViewBox)
{
  
  if (!checkURITarget(nsGkAtoms::viewBox)) {
    
    nsCOMPtr<nsIDOMSVGFitToViewBox> patternElement = 
                                            do_QueryInterface(mContent);
    nsCOMPtr<nsIDOMSVGAnimatedRect> viewBox;
    patternElement->GetViewBox(getter_AddRefs(viewBox));
    viewBox->GetAnimVal(aViewBox);
  } else {
    
    mNextPattern->GetViewBox(aViewBox);
  }
  mLoopFlag = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGPatternFrame::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio 
                                          **aPreserveAspectRatio)
{
  
  if (!checkURITarget(nsGkAtoms::preserveAspectRatio)) {
    
    nsCOMPtr<nsIDOMSVGFitToViewBox> patternElement = 
                                            do_QueryInterface(mContent);
    patternElement->GetPreserveAspectRatio(aPreserveAspectRatio);
  } else {
    
    mNextPattern->GetPreserveAspectRatio(aPreserveAspectRatio);
  }
  mLoopFlag = PR_FALSE;
  return NS_OK;
}

nsSVGLength2 *
nsSVGPatternFrame::GetX()
{
  nsSVGLength2 *rv = nsnull;

  
  if (checkURITarget(nsGkAtoms::x)) {
    
    rv = mNextPattern->GetX();
  } else {
    
    nsSVGPatternElement *pattern =
      NS_STATIC_CAST(nsSVGPatternElement*, mContent);
    rv = &pattern->mLengthAttributes[nsSVGPatternElement::X];
  }
  mLoopFlag = PR_FALSE;
  return rv;
}

nsSVGLength2 *
nsSVGPatternFrame::GetY()
{
  nsSVGLength2 *rv = nsnull;

  
  if (checkURITarget(nsGkAtoms::y)) {
    
    rv = mNextPattern->GetY();
  } else {
    
    nsSVGPatternElement *pattern =
      NS_STATIC_CAST(nsSVGPatternElement*, mContent);
    rv = &pattern->mLengthAttributes[nsSVGPatternElement::Y];
  }
  mLoopFlag = PR_FALSE;
  return rv;
}

nsSVGLength2 *
nsSVGPatternFrame::GetWidth()
{
  nsSVGLength2 *rv = nsnull;

  
  if (checkURITarget(nsGkAtoms::width)) {
    
    rv = mNextPattern->GetWidth();
  } else {
    
    nsSVGPatternElement *pattern =
      NS_STATIC_CAST(nsSVGPatternElement*, mContent);
    rv = &pattern->mLengthAttributes[nsSVGPatternElement::WIDTH];
  }
  mLoopFlag = PR_FALSE;
  return rv;
}

nsSVGLength2 *
nsSVGPatternFrame::GetHeight()
{
  nsSVGLength2 *rv = nsnull;

  
  if (checkURITarget(nsGkAtoms::height)) {
    
    rv = mNextPattern->GetHeight();
  } else {
    
    nsSVGPatternElement *pattern =
      NS_STATIC_CAST(nsSVGPatternElement*, mContent);
    rv = &pattern->mLengthAttributes[nsSVGPatternElement::HEIGHT];
  }
  mLoopFlag = PR_FALSE;
  return rv;
}


PRBool 
nsSVGPatternFrame::checkURITarget(nsIAtom *attr) {
  
  if (mContent->HasAttr(kNameSpaceID_None, attr)) {
    
    return PR_FALSE;
  }
  return checkURITarget();
}

PRBool
nsSVGPatternFrame::checkURITarget(void) {
  nsIFrame *nextPattern;
  mLoopFlag = PR_TRUE; 
  
  if (mNextPattern != nsnull) {
    return PR_TRUE;
  }

  
  
  nsAutoString href;
  mHref->GetAnimVal(href);
  
  if (href.IsEmpty()) {
    return PR_FALSE; 
  }

  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI),
    href, mContent->GetCurrentDoc(), base);

  
  
  if (NS_SUCCEEDED(
          nsSVGUtils::GetReferencedFrame(&nextPattern, targetURI, 
                                         mContent, 
                                         PresContext()->PresShell()))) {
    nsIAtom* frameType = nextPattern->GetType();
    if (frameType != nsGkAtoms::svgPatternFrame)
      return PR_FALSE;
    mNextPattern = (nsSVGPatternFrame *)nextPattern;
    
    if (mNextPattern->mLoopFlag) {
      
      NS_WARNING("Pattern loop detected!");
      mNextPattern = nsnull;
      return PR_FALSE;
    }
    
    if (mNextPattern) {
      
      mNextPattern->AddObserver(this);
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}





nsresult 
nsSVGPatternFrame::GetPatternRect(nsIDOMSVGRect **patternRect, 
                                  nsIDOMSVGRect *bbox, 
                                  nsSVGElement *content)
{
  
  PRUint16 type = GetPatternUnits();

  
  float x,y,width,height;

  
  nsSVGLength2 *tmpX, *tmpY, *tmpHeight, *tmpWidth;
  tmpX = GetX();
  tmpY = GetY();
  tmpHeight = GetHeight();
  tmpWidth = GetWidth();

  if (type == nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX) {
    x = nsSVGUtils::ObjectSpace(bbox, tmpX);
    y = nsSVGUtils::ObjectSpace(bbox, tmpY);
    width = nsSVGUtils::ObjectSpace(bbox, tmpWidth);
    height = nsSVGUtils::ObjectSpace(bbox, tmpHeight);
  } else {
    x = nsSVGUtils::UserSpace(content, tmpX);
    y = nsSVGUtils::UserSpace(content, tmpY);
    width = nsSVGUtils::UserSpace(content, tmpWidth);
    height = nsSVGUtils::UserSpace(content, tmpHeight);
  }

  return NS_NewSVGRect(patternRect, x, y, width, height);
}

static float
GetLengthValue(nsSVGLength2 *aLength)
{
  return aLength->GetAnimValue(NS_STATIC_CAST(nsSVGSVGElement*, nsnull));
}

nsresult
nsSVGPatternFrame::ConstructCTM(nsIDOMSVGMatrix **aCTM,
                                nsIDOMSVGRect *callerBBox)
{
  nsCOMPtr<nsIDOMSVGMatrix> tCTM, tempTM;

  
  
  PRUint16 type = GetPatternContentUnits();

  if (type == nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX) {
    
    float width, height;
    callerBBox->GetWidth(&width);
    callerBBox->GetHeight(&height);
    NS_NewSVGMatrix(getter_AddRefs(tCTM), width, 0.0f, 0.0f, 
                    height, 0.0f, 0.0f);
  } else {
    NS_NewSVGMatrix(getter_AddRefs(tCTM));
  }

  
  nsCOMPtr<nsIDOMSVGRect> viewRect;
  GetViewBox(getter_AddRefs(viewRect));

  
  float viewBoxX, viewBoxY, viewBoxHeight, viewBoxWidth;
  viewRect->GetX(&viewBoxX);
  viewRect->GetY(&viewBoxY);
  viewRect->GetHeight(&viewBoxHeight);
  viewRect->GetWidth(&viewBoxWidth);
  if (viewBoxHeight != 0.0f && viewBoxWidth != 0.0f) {

    float viewportWidth = GetLengthValue(GetWidth());
    float viewportHeight = GetLengthValue(GetHeight());
    float refX = GetLengthValue(GetX());
    float refY = GetLengthValue(GetY());

    nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> par;
    GetPreserveAspectRatio(getter_AddRefs(par));

    tempTM = nsSVGUtils::GetViewBoxTransform(viewportWidth, viewportHeight,
                                             viewBoxX + refX, viewBoxY + refY,
                                             viewBoxWidth, viewBoxHeight,
                                             par,
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
  if (type == nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX) {
    float x, y;
    callerBBox->GetX(&x);
    callerBBox->GetY(&y);
    minx += x;
    miny += y;
  }

  patternTransform.Translate(gfxPoint(minx, miny));

  
  return patternTransform * nsSVGUtils::ConvertSVGMatrixToThebes(callerCTM);
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
    *aContent = NS_STATIC_CAST(nsSVGElement*,
                               aSource->GetContent()->GetParent());
  } else {
    *aContent = NS_STATIC_CAST(nsSVGElement*, aSource->GetContent());
  }
  NS_ASSERTION(aContent,"Caller does not have any content!");
  if (!aContent)
    return NS_ERROR_FAILURE;

  
  
  nsISVGChildFrame *callerSVGFrame;
  if (callerType == nsGkAtoms::svgGlyphFrame)
    CallQueryInterface(aSource->GetParent(), &callerSVGFrame);
  else
    CallQueryInterface(aSource, &callerSVGFrame);
  callerSVGFrame->GetBBox(aBBox);
  
  PRUint16 type = GetPatternUnits();
  if (type == nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX) {
    float width, height;
    (*aBBox)->GetWidth(&width);
    (*aBBox)->GetHeight(&height);
    if (width <= 0 || height <= 0) {
      return NS_ERROR_FAILURE;
    }
  }

  
  aSource->GetCanvasTM(aCTM);

  
  
  {
    float x, y, width, height;
    (*aBBox)->GetX(&x);
    (*aBBox)->GetY(&y);
    (*aBBox)->GetWidth(&width);
    (*aBBox)->GetHeight(&height);
    float xpos, ypos, xscale, yscale;
    (*aCTM)->GetA(&xscale);
    (*aCTM)->GetD(&yscale);
    (*aCTM)->GetE(&xpos);
    (*aCTM)->GetF(&ypos);
    x = (x - xpos) / xscale;
    y = (y - ypos) / yscale;
    width = width / xscale;
    height = height / yscale;
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
                                    float aGraphicOpacity,
                                    void **aClosure)
{
  *aClosure = nsnull;

  if (aGraphicOpacity == 0.0f)
    return PR_FALSE;

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

  
  matrix.Invert();
  pMatrix *= matrix;

  if (pMatrix.IsSingular()) {
    return NS_ERROR_FAILURE;
  }
  pMatrix.Invert();

  nsRefPtr<gfxPattern> pattern = new gfxPattern(surface);

  if (!pattern)
    return NS_ERROR_FAILURE;

  pattern->SetMatrix(pMatrix);
  pattern->SetExtend(gfxPattern::EXTEND_REPEAT);

  aContext->SetPattern(pattern);

  return PR_TRUE;
}





nsIFrame* NS_NewSVGPatternFrame(nsIPresShell*   aPresShell,
                                nsIContent*     aContent,
                                nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGPatternElement> patternElement = do_QueryInterface(aContent);
  NS_ASSERTION(patternElement, 
               "NS_NewSVGPatternFrame -- Content doesn't support nsIDOMSVGPattern");
  if (!patternElement)
    return nsnull;

  nsCOMPtr<nsIDOMSVGURIReference> ref = do_QueryInterface(aContent);
  NS_ASSERTION(ref, 
               "NS_NewSVGPatternFrame -- Content doesn't support nsIDOMSVGURIReference");

#ifdef DEBUG_scooter
  printf("NS_NewSVGPatternFrame\n");
#endif
  return new (aPresShell) nsSVGPatternFrame(aContext, ref);
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
