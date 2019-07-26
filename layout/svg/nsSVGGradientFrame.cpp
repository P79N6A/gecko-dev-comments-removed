





#include "nsSVGGradientFrame.h"


#include "gfxPattern.h"
#include "nsContentUtils.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsIDOMSVGStopElement.h"
#include "nsSVGEffects.h"
#include "nsSVGGradientElement.h"
#include "SVGAnimatedTransformList.h"

using mozilla::SVGAnimatedTransformList;




class nsSVGGradientFrame::AutoGradientReferencer
{
public:
  AutoGradientReferencer(nsSVGGradientFrame *aFrame)
    : mFrame(aFrame)
  {
    
    
    NS_ABORT_IF_FALSE(!mFrame->mLoopFlag, "Undetected reference loop!");
    mFrame->mLoopFlag = true;
  }
  ~AutoGradientReferencer() {
    mFrame->mLoopFlag = false;
  }
private:
  nsSVGGradientFrame *mFrame;
};




nsSVGGradientFrame::nsSVGGradientFrame(nsStyleContext* aContext) :
  nsSVGGradientFrameBase(aContext),
  mLoopFlag(false),
  mNoHRefURI(false)
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGradientFrame)




 void
nsSVGGradientFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGEffects::InvalidateDirectRenderingObservers(this);
  nsSVGGradientFrameBase::DidSetStyleContext(aOldStyleContext);
}

NS_IMETHODIMP
nsSVGGradientFrame::AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::gradientUnits ||
       aAttribute == nsGkAtoms::gradientTransform ||
       aAttribute == nsGkAtoms::spreadMethod)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    mNoHRefURI = false;
    
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGGradientFrameBase::AttributeChanged(aNameSpaceID,
                                                  aAttribute, aModType);
}



uint32_t
nsSVGGradientFrame::GetStopCount()
{
  return GetStopFrame(-1, nullptr);
}

void
nsSVGGradientFrame::GetStopInformation(int32_t aIndex,
                                       float *aOffset,
                                       nscolor *aStopColor,
                                       float *aStopOpacity)
{
  *aOffset = 0.0f;
  *aStopColor = NS_RGBA(0, 0, 0, 0);
  *aStopOpacity = 1.0f;

  nsIFrame *stopFrame = nullptr;
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

  *aStopColor   = stopFrame->GetStyleSVGReset()->mStopColor;
  *aStopOpacity = stopFrame->GetStyleSVGReset()->mStopOpacity;
}

uint16_t
nsSVGGradientFrame::GetEnumValue(uint32_t aIndex, nsIContent *aDefault)
{
  const nsSVGEnum& thisEnum =
    static_cast<nsSVGGradientElement *>(mContent)->mEnumAttributes[aIndex];

  if (thisEnum.IsExplicitlySet())
    return thisEnum.GetAnimValue();

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetEnumValue(aIndex, aDefault) :
    static_cast<nsSVGGradientElement *>(aDefault)->
      mEnumAttributes[aIndex].GetAnimValue();
}

uint16_t
nsSVGGradientFrame::GetGradientUnits()
{
  
  return GetEnumValue(nsSVGGradientElement::GRADIENTUNITS);
}

uint16_t
nsSVGGradientFrame::GetSpreadMethod()
{
  return GetEnumValue(nsSVGGradientElement::SPREADMETHOD);
}

const SVGAnimatedTransformList*
nsSVGGradientFrame::GetGradientTransformList(nsIContent* aDefault)
{
  SVGAnimatedTransformList *thisTransformList =
    static_cast<nsSVGGradientElement *>(mContent)->GetAnimatedTransformList();

  if (thisTransformList && thisTransformList->IsExplicitlySet())
    return thisTransformList;

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetGradientTransformList(aDefault) :
    static_cast<const nsSVGGradientElement *>(aDefault)
      ->mGradientTransform.get();
}

gfxMatrix
nsSVGGradientFrame::GetGradientTransform(nsIFrame *aSource,
                                         const gfxRect *aOverrideBounds)
{
  gfxMatrix bboxMatrix;

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    
    
    
    if (aSource->GetContent()->IsNodeOfType(nsINode::eTEXT))
      mSource = aSource->GetParent();
    else
      mSource = aSource;
  } else {
    NS_ASSERTION(
      gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
      "Unknown gradientUnits type");
    

    gfxRect bbox =
      aOverrideBounds ? *aOverrideBounds : nsSVGUtils::GetBBox(aSource);
    bboxMatrix =
      gfxMatrix(bbox.Width(), 0, 0, bbox.Height(), bbox.X(), bbox.Y());
  }

  const SVGAnimatedTransformList* animTransformList =
    GetGradientTransformList(mContent);
  if (!animTransformList)
    return bboxMatrix;

  gfxMatrix gradientTransform =
    animTransformList->GetAnimValue().GetConsolidationMatrix();
  return bboxMatrix.PreMultiply(gradientTransform);
}

nsSVGLinearGradientElement *
nsSVGGradientFrame::GetLinearGradientWithLength(uint32_t aIndex,
  nsSVGLinearGradientElement* aDefault)
{
  
  
  

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetLinearGradientWithLength(aIndex, aDefault) : aDefault;
}

nsSVGRadialGradientElement *
nsSVGGradientFrame::GetRadialGradientWithLength(uint32_t aIndex,
  nsSVGRadialGradientElement* aDefault)
{
  
  
  

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetRadialGradientWithLength(aIndex, aDefault) : aDefault;
}




already_AddRefed<gfxPattern>
nsSVGGradientFrame::GetPaintServerPattern(nsIFrame *aSource,
                                          const gfxMatrix& aContextMatrix,
                                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                          float aGraphicOpacity,
                                          const gfxRect *aOverrideBounds)
{
  
  gfxMatrix patternMatrix = GetGradientTransform(aSource, aOverrideBounds);

  if (patternMatrix.IsSingular())
    return nullptr;

  uint32_t nStops = GetStopCount();

  
  
  if (nStops == 0) {
    nsRefPtr<gfxPattern> pattern = new gfxPattern(gfxRGBA(0, 0, 0, 0));
    return pattern.forget();
  }

  
  if (aFillOrStroke == &nsStyleSVG::mStroke) {
    patternMatrix.Multiply(nsSVGUtils::GetStrokeTransform(aSource).Invert());
  }

  patternMatrix.Invert();

  nsRefPtr<gfxPattern> gradient = CreateGradient();
  if (!gradient || gradient->CairoStatus())
    return nullptr;

  uint16_t aSpread = GetSpreadMethod();
  if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD)
    gradient->SetExtend(gfxPattern::EXTEND_PAD);
  else if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REFLECT)
    gradient->SetExtend(gfxPattern::EXTEND_REFLECT);
  else if (aSpread == nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REPEAT)
    gradient->SetExtend(gfxPattern::EXTEND_REPEAT);

  gradient->SetMatrix(patternMatrix);

  
  float lastOffset = 0.0f;

  for (uint32_t i = 0; i < nStops; i++) {
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

  return gradient.forget();
}



nsSVGGradientFrame *
nsSVGGradientFrame::GetReferencedGradient()
{
  if (mNoHRefURI)
    return nullptr;

  nsSVGPaintingProperty *property = static_cast<nsSVGPaintingProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    
    nsSVGGradientElement *grad = static_cast<nsSVGGradientElement *>(mContent);
    nsAutoString href;
    grad->mStringAttributes[nsSVGGradientElement::HREF].GetAnimValue(href, grad);
    if (href.IsEmpty()) {
      mNoHRefURI = true;
      return nullptr; 
    }

    
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetPaintingProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nullptr;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nullptr;

  nsIAtom* frameType = result->GetType();
  if (frameType != nsGkAtoms::svgLinearGradientFrame &&
      frameType != nsGkAtoms::svgRadialGradientFrame)
    return nullptr;

  return static_cast<nsSVGGradientFrame*>(result);
}

nsSVGGradientFrame *
nsSVGGradientFrame::GetReferencedGradientIfNotInUse()
{
  nsSVGGradientFrame *referenced = GetReferencedGradient();
  if (!referenced)
    return nullptr;

  if (referenced->mLoopFlag) {
    
    NS_WARNING("gradient reference loop detected while inheriting attribute!");
    return nullptr;
  }

  return referenced;
}

int32_t
nsSVGGradientFrame::GetStopFrame(int32_t aIndex, nsIFrame * *aStopFrame)
{
  int32_t stopCount = 0;
  nsIFrame *stopFrame = nullptr;
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

  

  AutoGradientReferencer gradientRef(this);
  nsSVGGradientFrame* next = GetReferencedGradientIfNotInUse();
  if (!next)
    return 0;

  return next->GetStopFrame(aIndex, aStopFrame);
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
nsSVGLinearGradientFrame::AttributeChanged(int32_t         aNameSpaceID,
                                           nsIAtom*        aAttribute,
                                           int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x1 ||
       aAttribute == nsGkAtoms::y1 ||
       aAttribute == nsGkAtoms::x2 ||
       aAttribute == nsGkAtoms::y2)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGLinearGradientFrame::GetLengthValue(uint32_t aIndex)
{
  nsSVGLinearGradientElement* lengthElement =
    GetLinearGradientWithLength(aIndex,
      static_cast<nsSVGLinearGradientElement *>(mContent));
  
  
  NS_ABORT_IF_FALSE(lengthElement,
    "Got unexpected null element from GetLinearGradientWithLength");
  const nsSVGLength2 &length = lengthElement->mLengthAttributes[aIndex];

  
  
  

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSource, &length);
  }

  NS_ASSERTION(
    gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
    "Unknown gradientUnits type");

  return length.GetAnimValue(static_cast<nsSVGSVGElement*>(nullptr));
}

nsSVGLinearGradientElement *
nsSVGLinearGradientFrame::GetLinearGradientWithLength(uint32_t aIndex,
  nsSVGLinearGradientElement* aDefault)
{
  nsSVGLinearGradientElement* thisElement =
    static_cast<nsSVGLinearGradientElement *>(mContent);
  const nsSVGLength2 &length = thisElement->mLengthAttributes[aIndex];

  if (length.IsExplicitlySet()) {
    return thisElement;
  }

  return nsSVGLinearGradientFrameBase::GetLinearGradientWithLength(aIndex,
                                                                   aDefault);
}

already_AddRefed<gfxPattern>
nsSVGLinearGradientFrame::CreateGradient()
{
  float x1, y1, x2, y2;

  x1 = GetLengthValue(nsSVGLinearGradientElement::X1);
  y1 = GetLengthValue(nsSVGLinearGradientElement::Y1);
  x2 = GetLengthValue(nsSVGLinearGradientElement::X2);
  y2 = GetLengthValue(nsSVGLinearGradientElement::Y2);

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
nsSVGRadialGradientFrame::AttributeChanged(int32_t         aNameSpaceID,
                                           nsIAtom*        aAttribute,
                                           int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::r ||
       aAttribute == nsGkAtoms::cx ||
       aAttribute == nsGkAtoms::cy ||
       aAttribute == nsGkAtoms::fx ||
       aAttribute == nsGkAtoms::fy)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGGradientFrame::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}



float
nsSVGRadialGradientFrame::GetLengthValue(uint32_t aIndex)
{
  nsSVGRadialGradientElement* lengthElement =
    GetRadialGradientWithLength(aIndex,
      static_cast<nsSVGRadialGradientElement *>(mContent));
  
  
  NS_ABORT_IF_FALSE(lengthElement,
    "Got unexpected null element from GetRadialGradientWithLength");
  return GetLengthValueFromElement(aIndex, *lengthElement);
}

float
nsSVGRadialGradientFrame::GetLengthValue(uint32_t aIndex, float aDefaultValue)
{
  nsSVGRadialGradientElement* lengthElement =
    GetRadialGradientWithLength(aIndex, nullptr);

  return lengthElement ? GetLengthValueFromElement(aIndex, *lengthElement)
                       : aDefaultValue;
}

float
nsSVGRadialGradientFrame::GetLengthValueFromElement(uint32_t aIndex,
  nsSVGRadialGradientElement& aElement)
{
  const nsSVGLength2 &length = aElement.mLengthAttributes[aIndex];

  
  
  

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSource, &length);
  }

  NS_ASSERTION(
    gradientUnits == nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
    "Unknown gradientUnits type");

  return length.GetAnimValue(static_cast<nsSVGSVGElement*>(nullptr));
}

nsSVGRadialGradientElement *
nsSVGRadialGradientFrame::GetRadialGradientWithLength(uint32_t aIndex,
  nsSVGRadialGradientElement* aDefault)
{
  nsSVGRadialGradientElement* thisElement =
    static_cast<nsSVGRadialGradientElement *>(mContent);
  const nsSVGLength2 &length = thisElement->mLengthAttributes[aIndex];

  if (length.IsExplicitlySet()) {
    return thisElement;
  }

  return nsSVGRadialGradientFrameBase::GetRadialGradientWithLength(aIndex,
                                                                   aDefault);
}

already_AddRefed<gfxPattern>
nsSVGRadialGradientFrame::CreateGradient()
{
  float cx, cy, r, fx, fy;

  cx = GetLengthValue(nsSVGRadialGradientElement::CX);
  cy = GetLengthValue(nsSVGRadialGradientElement::CY);
  r  = GetLengthValue(nsSVGRadialGradientElement::R);
  
  fx = GetLengthValue(nsSVGRadialGradientElement::FX, cx);
  fy = GetLengthValue(nsSVGRadialGradientElement::FY, cy);

  if (fx != cx || fy != cy) {
    
    
    
    
    
    
    
    double dMax = NS_MAX(0.0, r - 1.0/128);
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
