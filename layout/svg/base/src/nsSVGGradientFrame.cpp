






































#include "nsIDOMSVGAnimatedNumber.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGTransformList.h"
#include "nsSVGMatrix.h"
#include "nsIDOMSVGStopElement.h"
#include "nsSVGGradientElement.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGGradientFrame.h"
#include "gfxContext.h"
#include "nsIDOMSVGRect.h"
#include "gfxPattern.h"




nsSVGGradientFrame::nsSVGGradientFrame(nsStyleContext* aContext,
                                       nsIDOMSVGURIReference *aRef) :
  nsSVGGradientFrameBase(aContext),
  mNextGrad(nsnull), 
  mLoopFlag(PR_FALSE),
  mInitialized(PR_FALSE) 
{
  if (aRef) {
    
    aRef->GetHref(getter_AddRefs(mHref));
  }
}

nsSVGGradientFrame::~nsSVGGradientFrame()
{
  WillModify(mod_die);
  
  DidModify(mod_die);

  if (mNextGrad) 
    mNextGrad->RemoveObserver(this);
}




NS_INTERFACE_MAP_BEGIN(nsSVGGradientFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGradientFrameBase)



NS_IMETHODIMP
nsSVGGradientFrame::WillModifySVGObservable(nsISVGValue* observable,
                                            modificationType aModType)
{
  
  if (mLoopFlag) {
    
    NS_WARNING("gradient reference loop detected while notifying observers!");
    return NS_OK;
  }

  
  if (aModType == mod_die)
    aModType = mod_other;

  WillModify(aModType);
  return NS_OK;
}
                                                                                
NS_IMETHODIMP
nsSVGGradientFrame::DidModifySVGObservable(nsISVGValue* observable, 
                                           nsISVGValue::modificationType aModType)
{
  
  if (mLoopFlag) {
    
    NS_WARNING("gradient reference loop detected while notifying observers!");
    return NS_OK;
  }

  
  if (mNextGrad && aModType == nsISVGValue::mod_die) {
    nsIFrame *gradient = nsnull;
    CallQueryInterface(observable, &gradient);
    if (mNextGrad == gradient)
      mNextGrad = nsnull;
  }

  
  if (aModType == mod_die)
    aModType = mod_other;

  DidModify(aModType);
  return NS_OK;
}




NS_IMETHODIMP
nsSVGGradientFrame::DidSetStyleContext()
{
  WillModify();
  DidModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGradientFrame::RemoveFrame(nsIAtom*        aListName,
                                nsIFrame*       aOldFrame)
{
  WillModify();
  PRBool result = mFrames.DestroyFrame(aOldFrame);
  DidModify();
  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsIAtom*
nsSVGGradientFrame::GetType() const
{
  return nsGkAtoms::svgGradientFrame;
}

NS_IMETHODIMP
nsSVGGradientFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::gradientUnits ||
       aAttribute == nsGkAtoms::gradientTransform ||
       aAttribute == nsGkAtoms::spreadMethod)) {
    WillModify();
    DidModify();
    return NS_OK;
  } 

  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    if (mNextGrad)
      mNextGrad->RemoveObserver(this);
    WillModify();
    GetRefedGradientFromHref();
    DidModify();
    return NS_OK;
  }

  return nsSVGGradientFrameBase::AttributeChanged(aNameSpaceID,
                                                  aAttribute, aModType);
}



PRUint32
nsSVGGradientFrame::GetStopCount()
{
  return GetStopFrame(-1, nsnull);
}

void
nsSVGGradientFrame::GetStopInformation(PRInt32 aIndex,
                                       float *aOffset,
                                       nscolor *aStopColor,
                                       float *aStopOpacity)
{
  *aOffset = 0.0f;
  *aStopColor = NS_RGBA(0, 0, 0, 0);
  *aStopOpacity = 1.0f;

  nsIFrame *stopFrame = nsnull;
  GetStopFrame(aIndex, &stopFrame);
  nsCOMPtr<nsIDOMSVGStopElement> stopElement =
    do_QueryInterface(stopFrame->GetContent());

  if (stopElement) {
    nsCOMPtr<nsIDOMSVGAnimatedNumber> aNum;
    stopElement->GetOffset(getter_AddRefs(aNum));

    aNum->GetAnimVal(aOffset);
    if (*aOffset < 0.0f)
      *aOffset = 0.0f;
    else if (*aOffset > 1.0f)
      *aOffset = 1.0f;
  }

  if (stopFrame) {
    *aStopColor   = stopFrame->GetStyleSVGReset()->mStopColor;
    *aStopOpacity = stopFrame->GetStyleSVGReset()->mStopOpacity;
  }
#ifdef DEBUG
  
  else if (stopElement) {
    NS_WARNING("We *do* have a stop but can't use it because it doesn't have "
               "a frame - we need frame free gradients and stops!");
  }
  else {
    NS_ERROR("Don't call me with an invalid stop index!");
  }
#endif
}

gfxMatrix
nsSVGGradientFrame::GetGradientTransform(nsSVGGeometryFrame *aSource)
{
  gfxMatrix bboxMatrix;

  PRUint16 gradientUnits = GetGradientUnits();
  nsIAtom *callerType = aSource->GetType();
  if (gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_USERSPACEONUSE) {
    
    
    
    if (callerType ==  nsGkAtoms::svgGlyphFrame)
      mSourceContent = static_cast<nsSVGElement*>
                                  (aSource->GetContent()->GetParent());
    else
      mSourceContent = static_cast<nsSVGElement*>(aSource->GetContent());
    NS_ASSERTION(mSourceContent, "Can't get content for gradient");
  }
  else {
    NS_ASSERTION(gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_OBJECTBOUNDINGBOX,
                 "Unknown gradientUnits type");
    

    nsISVGChildFrame *frame = nsnull;
    if (aSource) {
      if (callerType == nsGkAtoms::svgGlyphFrame)
        CallQueryInterface(aSource->GetParent(), &frame);
      else
        CallQueryInterface(aSource, &frame);
    }
    nsCOMPtr<nsIDOMSVGRect> rect;
    if (frame) {
      frame->SetMatrixPropagation(PR_FALSE);
      frame->NotifyCanvasTMChanged(PR_TRUE);
      frame->GetBBox(getter_AddRefs(rect));
      frame->SetMatrixPropagation(PR_TRUE);
      frame->NotifyCanvasTMChanged(PR_TRUE);
    }
    if (rect) {
      float x, y, width, height;
      rect->GetX(&x);
      rect->GetY(&y);
      rect->GetWidth(&width);
      rect->GetHeight(&height);
      bboxMatrix = gfxMatrix(width, 0, 0, height, x, y);
    }
  }

  nsIContent *gradient = GetGradientWithAttr(nsGkAtoms::gradientTransform);
  if (!gradient)
    gradient = mContent;  

  nsSVGGradientElement *gradElement = static_cast<nsSVGGradientElement*>
                                                 (gradient);
  nsCOMPtr<nsIDOMSVGTransformList> trans;
  gradElement->mGradientTransform->GetAnimVal(getter_AddRefs(trans));
  nsCOMPtr<nsIDOMSVGMatrix> gradientTransform =
    nsSVGTransformList::GetConsolidationMatrix(trans);

  if (!gradientTransform)
    return bboxMatrix;

  return nsSVGUtils::ConvertSVGMatrixToThebes(gradientTransform) * bboxMatrix;
}

PRUint16
nsSVGGradientFrame::GetSpreadMethod()
{
  nsIContent *gradient = GetGradientWithAttr(nsGkAtoms::spreadMethod);
  if (!gradient)
    gradient = mContent;  

  nsSVGGradientElement *gradElement = static_cast<nsSVGGradientElement*>
                                                 (gradient);

  PRUint16 val;
  gradElement->mSpreadMethod->GetAnimVal(&val);
  return val;
}




PRBool
nsSVGGradientFrame::SetupPaintServer(gfxContext *aContext,
                                     nsSVGGeometryFrame *aSource,
                                     float aGraphicOpacity,
                                     void **aClosure)
{
  *aClosure = nsnull;

  PRUint32 nStops = GetStopCount();

  
  
  if (nStops == 0)
    return PR_FALSE;

  
  gfxMatrix patternMatrix = GetGradientTransform(aSource);

  if (patternMatrix.IsSingular())
    return PR_FALSE;

  patternMatrix.Invert();

  nsRefPtr<gfxPattern> gradient = CreateGradient();
  if (!gradient)
    return PR_FALSE;

  PRUint16 aSpread = GetSpreadMethod();
  if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD)
    gradient->SetExtend(gfxPattern::EXTEND_PAD);
  else if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REFLECT)
    gradient->SetExtend(gfxPattern::EXTEND_REFLECT);
  else if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REPEAT)
    gradient->SetExtend(gfxPattern::EXTEND_REPEAT);

  gradient->SetMatrix(patternMatrix);

  
  float lastOffset = 0.0f;

  for (PRUint32 i = 0; i < nStops; i++) {
    float offset, stopOpacity;
    nscolor stopColor;

    GetStopInformation(i, &offset, &stopColor, &stopOpacity);

    if (offset < lastOffset)
      offset = lastOffset;
    else
      lastOffset = offset;

    gradient->AddColorStop(offset,
                           gfxRGBA(NS_GET_R(stopColor)/255.0,
                                   NS_GET_G(stopColor)/255.0,
                                   NS_GET_B(stopColor)/255.0,
                                   NS_GET_A(stopColor)/255.0 *
                                     stopOpacity * aGraphicOpacity));
  }

  aContext->SetPattern(gradient);

  return PR_TRUE;
}



void
nsSVGGradientFrame::GetRefedGradientFromHref()
{
  mNextGrad = nsnull;
  mInitialized = PR_TRUE;

  
  nsAutoString href;
  mHref->GetAnimVal(href);
  if (href.IsEmpty()) {
    return; 
  }

  
  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                            mContent->GetCurrentDoc(), base);

  
  
  
  nsIFrame *nextGrad;
  if (NS_SUCCEEDED(nsSVGUtils::GetReferencedFrame(&nextGrad, targetURI, mContent,
                                                  PresContext()->PresShell()))) {
    nsIAtom* frameType = nextGrad->GetType();
    if (frameType != nsGkAtoms::svgLinearGradientFrame && 
        frameType != nsGkAtoms::svgRadialGradientFrame)
      return;

    mNextGrad = reinterpret_cast<nsSVGGradientFrame*>(nextGrad);

    
    if (mNextGrad) {
      
      mNextGrad->AddObserver(this);
    }
  }
}



nsIContent*
nsSVGGradientFrame::GetGradientWithAttr(nsIAtom *aAttrName)
{
  if (mContent->HasAttr(kNameSpaceID_None, aAttrName))
    return mContent;

  if (!mInitialized)  
    GetRefedGradientFromHref();

  if (!mNextGrad)
    return nsnull;

  nsIContent *grad = nsnull;

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!mNextGrad->mLoopFlag, "gradient reference loop detected "
                                          "while inheriting attribute!");
  if (!mNextGrad->mLoopFlag)
    grad = mNextGrad->GetGradientWithAttr(aAttrName);
  mLoopFlag = PR_FALSE;

  return grad;
}

nsIContent*
nsSVGGradientFrame::GetGradientWithAttr(nsIAtom *aAttrName, nsIAtom *aGradType)
{
  if (GetType() == aGradType && mContent->HasAttr(kNameSpaceID_None, aAttrName))
    return mContent;

  if (!mInitialized)
    GetRefedGradientFromHref();  

  if (!mNextGrad)
    return nsnull;

  nsIContent *grad = nsnull;

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!mNextGrad->mLoopFlag, "gradient reference loop detected "
                                          "while inheriting attribute!");
  if (!mNextGrad->mLoopFlag)
    grad = mNextGrad->GetGradientWithAttr(aAttrName, aGradType);
  mLoopFlag = PR_FALSE;

  return grad;
}

PRInt32 
nsSVGGradientFrame::GetStopFrame(PRInt32 aIndex, nsIFrame * *aStopFrame)
{
  PRInt32 stopCount = 0;
  nsIFrame *stopFrame = nsnull;
  for (stopFrame = mFrames.FirstChild(); stopFrame;
       stopFrame = stopFrame->GetNextSibling()) {
    if (stopFrame->GetType() == nsGkAtoms::svgStopFrame) {
      
      if (stopCount++ == aIndex)
        break; 
    }
  }
  if (stopCount > 0) {
    if (aStopFrame)
      *aStopFrame = stopFrame;
    return stopCount;
  }

  

  if (!mInitialized)
    GetRefedGradientFromHref();  

  if (!mNextGrad) {
    if (aStopFrame)
      *aStopFrame = nsnull;
    return 0;
  }

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!mNextGrad->mLoopFlag, "gradient reference loop detected "
                                          "while inheriting stop!");
  if (!mNextGrad->mLoopFlag)
    stopCount = mNextGrad->GetStopFrame(aIndex, aStopFrame);
  mLoopFlag = PR_FALSE;

  return stopCount;
}

PRUint16
nsSVGGradientFrame::GetGradientUnits()
{
  

  nsIContent *gradient = GetGradientWithAttr(nsGkAtoms::gradientUnits);
  if (!gradient)
    gradient = mContent;  

  nsSVGGradientElement *gradElement = static_cast<nsSVGGradientElement*>
                                                 (gradient);

  PRUint16 units;
  gradElement->mGradientUnits->GetAnimVal(&units);
  return units;
}





nsIAtom*
nsSVGLinearGradientFrame::GetType() const
{
  return nsGkAtoms::svgLinearGradientFrame;
}

NS_IMETHODIMP
nsSVGLinearGradientFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                           nsIAtom*        aAttribute,
                                           PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x1 ||
       aAttribute == nsGkAtoms::y1 ||
       aAttribute == nsGkAtoms::x2 ||
       aAttribute == nsGkAtoms::y2)) {
    WillModify();
    DidModify();
    return NS_OK;
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGLinearGradientFrame::GradientLookupAttribute(nsIAtom *aAtomName,
                                                  PRUint16 aEnumName)
{
  nsIContent *gradient = GetLinearGradientWithAttr(aAtomName);
  if (!gradient)
    gradient = mContent;  

  nsSVGLinearGradientElement *element =
    static_cast<nsSVGLinearGradientElement*>(gradient);

  
  
  

  PRUint16 gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSourceContent,
                                 &element->mLengthAttributes[aEnumName]);
  }

  NS_ASSERTION(gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_OBJECTBOUNDINGBOX,
               "Unknown gradientUnits type");

  return element->mLengthAttributes[aEnumName].
    GetAnimValue(static_cast<nsSVGSVGElement*>(nsnull));
}

already_AddRefed<gfxPattern>
nsSVGLinearGradientFrame::CreateGradient()
{
  float x1, y1, x2, y2;

  x1 = GradientLookupAttribute(nsGkAtoms::x1, nsSVGLinearGradientElement::X1);
  y1 = GradientLookupAttribute(nsGkAtoms::y1, nsSVGLinearGradientElement::Y1);
  x2 = GradientLookupAttribute(nsGkAtoms::x2, nsSVGLinearGradientElement::X2);
  y2 = GradientLookupAttribute(nsGkAtoms::y2, nsSVGLinearGradientElement::Y2);

  gfxPattern *pattern = new gfxPattern(x1, y1, x2, y2);
  NS_IF_ADDREF(pattern);
  return pattern;
}





nsIAtom*
nsSVGRadialGradientFrame::GetType() const
{
  return nsGkAtoms::svgRadialGradientFrame;
}

NS_IMETHODIMP
nsSVGRadialGradientFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                           nsIAtom*        aAttribute,
                                           PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::r ||
       aAttribute == nsGkAtoms::cx ||
       aAttribute == nsGkAtoms::cy ||
       aAttribute == nsGkAtoms::fx ||
       aAttribute == nsGkAtoms::fy)) {
    WillModify();
    DidModify();
    return NS_OK;
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGRadialGradientFrame::GradientLookupAttribute(nsIAtom *aAtomName,
                                                  PRUint16 aEnumName,
                                                  nsIContent *aElement)
{
  nsIContent *gradient;

  if (aElement) {
    gradient = aElement;
  } else {
    gradient = GetRadialGradientWithAttr(aAtomName);
    if (!gradient)
      gradient = mContent;  
  }

  nsSVGRadialGradientElement *element =
    static_cast<nsSVGRadialGradientElement*>(gradient);

  
  
  

  PRUint16 gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSourceContent,
                                 &element->mLengthAttributes[aEnumName]);
  }

  NS_ASSERTION(gradientUnits == nsIDOMSVGGradientElement::SVG_GRUNITS_OBJECTBOUNDINGBOX,
               "Unknown gradientUnits type");

  return element->mLengthAttributes[aEnumName].
    GetAnimValue(static_cast<nsSVGSVGElement*>(nsnull));
}

already_AddRefed<gfxPattern>
nsSVGRadialGradientFrame::CreateGradient()
{
  float cx, cy, r, fx, fy;

  cx = GradientLookupAttribute(nsGkAtoms::cx, nsSVGRadialGradientElement::CX);
  cy = GradientLookupAttribute(nsGkAtoms::cy, nsSVGRadialGradientElement::CY);
  r  = GradientLookupAttribute(nsGkAtoms::r,  nsSVGRadialGradientElement::R);

  nsIContent *gradient;

  if (!(gradient = GetRadialGradientWithAttr(nsGkAtoms::fx)))
    fx = cx;  
  else
    fx = GradientLookupAttribute(nsGkAtoms::fx, nsSVGRadialGradientElement::FX, gradient);

  if (!(gradient = GetRadialGradientWithAttr(nsGkAtoms::fy)))
    fy = cy;  
  else
    fy = GradientLookupAttribute(nsGkAtoms::fy, nsSVGRadialGradientElement::FY, gradient);

  if (fx != cx || fy != cy) {
    
    
    
    
    
    
    
    double dMax = 0.999 * r;
    float dx = fx - cx;
    float dy = fy - cy;
    double d = sqrt((dx * dx) + (dy * dy));
    if (d > dMax) {
      double angle = atan2(dy, dx);
      fx = (float)(dMax * cos(angle)) + cx;
      fy = (float)(dMax * sin(angle)) + cy;
    }
  }

  gfxPattern *pattern = new gfxPattern(fx, fy, 0, cx, cy, r);
  NS_IF_ADDREF(pattern);
  return pattern;
}





nsIFrame* 
NS_NewSVGLinearGradientFrame(nsIPresShell*   aPresShell,
                             nsIContent*     aContent,
                             nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGLinearGradientElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "NS_NewSVGLinearGradientFrame -- Content doesn't support nsIDOMSVGLinearGradient");
  if (!grad)
    return nsnull;
  
  nsCOMPtr<nsIDOMSVGURIReference> aRef = do_QueryInterface(aContent);
  NS_ASSERTION(aRef, "NS_NewSVGLinearGradientFrame -- Content doesn't support nsIDOMSVGURIReference");

  return new (aPresShell) nsSVGLinearGradientFrame(aContext, aRef);
}

nsIFrame*
NS_NewSVGRadialGradientFrame(nsIPresShell*   aPresShell,
                             nsIContent*     aContent,
                             nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGRadialGradientElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "NS_NewSVGRadialGradientFrame -- Content doesn't support nsIDOMSVGRadialGradient");
  if (!grad)
    return nsnull;
  
  nsCOMPtr<nsIDOMSVGURIReference> aRef = do_QueryInterface(aContent);
  NS_ASSERTION(aRef, "NS_NewSVGRadialGradientFrame -- Content doesn't support nsIDOMSVGURIReference");

  return new (aPresShell) nsSVGRadialGradientFrame(aContext, aRef);
}
