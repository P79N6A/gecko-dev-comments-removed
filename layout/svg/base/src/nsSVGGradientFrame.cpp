






































#include "nsIDOMSVGAnimatedNumber.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGTransformList.h"
#include "nsSVGMatrix.h"
#include "nsSVGEffects.h"
#include "nsIDOMSVGStopElement.h"
#include "nsSVGGradientElement.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGGradientFrame.h"
#include "gfxContext.h"
#include "gfxPattern.h"




nsSVGGradientFrame::nsSVGGradientFrame(nsStyleContext* aContext) :
  nsSVGGradientFrameBase(aContext),
  mLoopFlag(PR_FALSE),
  mNoHRefURI(PR_FALSE)
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGradientFrame)




 void
nsSVGGradientFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGEffects::InvalidateRenderingObservers(this);
  nsSVGGradientFrameBase::DidSetStyleContext(aOldStyleContext);
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
    nsSVGEffects::InvalidateRenderingObservers(this);
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    DeleteProperty(nsGkAtoms::href);
    mNoHRefURI = PR_FALSE;
    
    nsSVGEffects::InvalidateRenderingObservers(this);
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
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    
    
    
    if (callerType ==  nsGkAtoms::svgGlyphFrame)
      mSourceContent = static_cast<nsSVGElement*>
                                  (aSource->GetContent()->GetParent());
    else
      mSourceContent = static_cast<nsSVGElement*>(aSource->GetContent());
    NS_ASSERTION(mSourceContent, "Can't get content for gradient");
  }
  else {
    NS_ASSERTION(gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
                 "Unknown gradientUnits type");
    

    nsIFrame *frame = (callerType == nsGkAtoms::svgGlyphFrame) ?
                        aSource->GetParent() : aSource;
    gfxRect bbox = nsSVGUtils::GetBBox(frame);
    bboxMatrix = gfxMatrix(bbox.Width(), 0, 0, bbox.Height(), bbox.X(), bbox.Y());
  }

  nsSVGGradientElement *element =
    GetGradientWithAttr(nsGkAtoms::gradientTransform, mContent);

  nsCOMPtr<nsIDOMSVGTransformList> trans;
  element->mGradientTransform->GetAnimVal(getter_AddRefs(trans));
  nsCOMPtr<nsIDOMSVGMatrix> gradientTransform =
    nsSVGTransformList::GetConsolidationMatrix(trans);

  if (!gradientTransform)
    return bboxMatrix;

  return bboxMatrix.PreMultiply(nsSVGUtils::ConvertSVGMatrixToThebes(gradientTransform));
}

PRUint16
nsSVGGradientFrame::GetSpreadMethod()
{
  nsSVGGradientElement *element =
    GetGradientWithAttr(nsGkAtoms::spreadMethod, mContent);

  return element->mEnumAttributes[nsSVGGradientElement::SPREADMETHOD].GetAnimValue();
}




PRBool
nsSVGGradientFrame::SetupPaintServer(gfxContext *aContext,
                                     nsSVGGeometryFrame *aSource,
                                     float aGraphicOpacity)
{
  
  gfxMatrix patternMatrix = GetGradientTransform(aSource);

  if (patternMatrix.IsSingular())
    return PR_FALSE;

  PRUint32 nStops = GetStopCount();

  
  
  if (nStops == 0) {
    aContext->SetColor(gfxRGBA(0, 0, 0, 0));
    return PR_TRUE;
  }

  patternMatrix.Invert();

  nsRefPtr<gfxPattern> gradient = CreateGradient();
  if (!gradient || gradient->CairoStatus())
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



nsSVGGradientFrame *
nsSVGGradientFrame::GetReferencedGradient()
{
  if (mNoHRefURI)
    return nsnull;

  nsSVGPaintingProperty *property =
    static_cast<nsSVGPaintingProperty*>(GetProperty(nsGkAtoms::href));

  if (!property) {
    
    nsSVGGradientElement *grad = static_cast<nsSVGGradientElement *>(mContent);
    nsAutoString href;
    grad->mStringAttributes[nsSVGGradientElement::HREF].GetAnimValue(href, grad);
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
  if (frameType != nsGkAtoms::svgLinearGradientFrame &&
      frameType != nsGkAtoms::svgRadialGradientFrame)
    return nsnull;

  return static_cast<nsSVGGradientFrame*>(result);
}

nsSVGGradientElement *
nsSVGGradientFrame::GetGradientWithAttr(nsIAtom *aAttrName, nsIContent *aDefault)
{
  if (mContent->HasAttr(kNameSpaceID_None, aAttrName))
    return static_cast<nsSVGGradientElement *>(mContent);

  nsSVGGradientElement *grad = static_cast<nsSVGGradientElement *>(aDefault);

  nsSVGGradientFrame *next = GetReferencedGradient();
  if (!next)
    return grad;

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!next->mLoopFlag, "gradient reference loop detected "
                                     "while inheriting attribute!");
  if (!next->mLoopFlag)
    grad = next->GetGradientWithAttr(aAttrName, aDefault);
  mLoopFlag = PR_FALSE;

  return grad;
}

nsSVGGradientElement *
nsSVGGradientFrame::GetGradientWithAttr(nsIAtom *aAttrName, nsIAtom *aGradType,
                                        nsIContent *aDefault)
{
  if (GetType() == aGradType && mContent->HasAttr(kNameSpaceID_None, aAttrName))
    return static_cast<nsSVGGradientElement *>(mContent);

  nsSVGGradientElement *grad = static_cast<nsSVGGradientElement *>(aDefault);

  nsSVGGradientFrame *next = GetReferencedGradient();
  if (!next)
    return grad;

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!next->mLoopFlag, "gradient reference loop detected "
                                     "while inheriting attribute!");
  if (!next->mLoopFlag)
    grad = next->GetGradientWithAttr(aAttrName, aGradType, aDefault);
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

  

  nsSVGGradientFrame *next = GetReferencedGradient();
  if (!next) {
    if (aStopFrame)
      *aStopFrame = nsnull;
    return 0;
  }

  
  mLoopFlag = PR_TRUE;
  
  NS_WARN_IF_FALSE(!next->mLoopFlag, "gradient reference loop detected "
                                     "while inheriting stop!");
  if (!next->mLoopFlag)
    stopCount = next->GetStopFrame(aIndex, aStopFrame);
  mLoopFlag = PR_FALSE;

  return stopCount;
}

PRUint16
nsSVGGradientFrame::GetGradientUnits()
{
  

  nsSVGGradientElement *element =
    GetGradientWithAttr(nsGkAtoms::gradientUnits, mContent);
  return element->mEnumAttributes[nsSVGGradientElement::GRADIENTUNITS].GetAnimValue();
}





#ifdef DEBUG
NS_IMETHODIMP
nsSVGLinearGradientFrame::Init(nsIContent* aContent,
                               nsIFrame* aParent,
                               nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGLinearGradientElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "Content is not an SVG linearGradient");

  return nsSVGLinearGradientFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

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
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGLinearGradientFrame::GradientLookupAttribute(nsIAtom *aAtomName,
                                                  PRUint16 aEnumName)
{
  nsSVGLinearGradientElement *element =
    GetLinearGradientWithAttr(aAtomName, mContent);

  
  
  

  PRUint16 gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSourceContent,
                                 &element->mLengthAttributes[aEnumName]);
  }

  NS_ASSERTION(gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
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





#ifdef DEBUG
NS_IMETHODIMP
nsSVGRadialGradientFrame::Init(nsIContent* aContent,
                               nsIFrame* aParent,
                               nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGRadialGradientElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "Content is not an SVG radialGradient");

  return nsSVGRadialGradientFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

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
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGRadialGradientFrame::GradientLookupAttribute(nsIAtom *aAtomName,
                                                  PRUint16 aEnumName,
                                                  nsSVGRadialGradientElement *aElement)
{
  nsSVGRadialGradientElement *element;

  if (aElement) {
    element = aElement;
  } else {
    element = GetRadialGradientWithAttr(aAtomName, mContent);
  }

  
  
  

  PRUint16 gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSourceContent,
                                 &element->mLengthAttributes[aEnumName]);
  }

  NS_ASSERTION(gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
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

  nsSVGRadialGradientElement *gradient;

  if (!(gradient = GetRadialGradientWithAttr(nsGkAtoms::fx, nsnull)))
    fx = cx;  
  else
    fx = GradientLookupAttribute(nsGkAtoms::fx, nsSVGRadialGradientElement::FX, gradient);

  if (!(gradient = GetRadialGradientWithAttr(nsGkAtoms::fy, nsnull)))
    fy = cy;  
  else
    fy = GradientLookupAttribute(nsGkAtoms::fy, nsSVGRadialGradientElement::FY, gradient);

  if (fx != cx || fy != cy) {
    
    
    
    
    
    
    
    
    
    
    double dMax = 0.99 * r;
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
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGLinearGradientFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGLinearGradientFrame)

nsIFrame*
NS_NewSVGRadialGradientFrame(nsIPresShell*   aPresShell,
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGRadialGradientFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGRadialGradientFrame)
