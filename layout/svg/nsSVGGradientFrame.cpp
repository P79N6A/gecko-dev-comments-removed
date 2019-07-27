





#include "nsSVGGradientFrame.h"
#include <algorithm>


#include "gfxPattern.h"
#include "mozilla/dom/SVGGradientElement.h"
#include "mozilla/dom/SVGStopElement.h"
#include "nsContentUtils.h"
#include "nsSVGEffects.h"
#include "nsSVGAnimatedTransformList.h"
#include "gfxColor.h"



using namespace mozilla;
using namespace mozilla::dom;




class MOZ_STACK_CLASS nsSVGGradientFrame::AutoGradientReferencer
{
public:
  explicit AutoGradientReferencer(nsSVGGradientFrame *aFrame
                                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mFrame(aFrame)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    
    
    NS_ABORT_IF_FALSE(!mFrame->mLoopFlag, "Undetected reference loop!");
    mFrame->mLoopFlag = true;
  }
  ~AutoGradientReferencer() {
    mFrame->mLoopFlag = false;
  }
private:
  nsSVGGradientFrame *mFrame;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};




nsSVGGradientFrame::nsSVGGradientFrame(nsStyleContext* aContext) :
  nsSVGGradientFrameBase(aContext),
  mLoopFlag(false),
  mNoHRefURI(false)
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGradientFrame)




nsresult
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



uint16_t
nsSVGGradientFrame::GetEnumValue(uint32_t aIndex, nsIContent *aDefault)
{
  const nsSVGEnum& thisEnum =
    static_cast<dom::SVGGradientElement*>(mContent)->mEnumAttributes[aIndex];

  if (thisEnum.IsExplicitlySet())
    return thisEnum.GetAnimValue();

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetEnumValue(aIndex, aDefault) :
    static_cast<dom::SVGGradientElement*>(aDefault)->
      mEnumAttributes[aIndex].GetAnimValue();
}

uint16_t
nsSVGGradientFrame::GetGradientUnits()
{
  
  return GetEnumValue(dom::SVGGradientElement::GRADIENTUNITS);
}

uint16_t
nsSVGGradientFrame::GetSpreadMethod()
{
  return GetEnumValue(dom::SVGGradientElement::SPREADMETHOD);
}

const nsSVGAnimatedTransformList*
nsSVGGradientFrame::GetGradientTransformList(nsIContent* aDefault)
{
  nsSVGAnimatedTransformList *thisTransformList =
    static_cast<dom::SVGGradientElement*>(mContent)->GetAnimatedTransformList();

  if (thisTransformList && thisTransformList->IsExplicitlySet())
    return thisTransformList;

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetGradientTransformList(aDefault) :
    static_cast<const dom::SVGGradientElement*>(aDefault)
      ->mGradientTransform.get();
}

gfxMatrix
nsSVGGradientFrame::GetGradientTransform(nsIFrame *aSource,
                                         const gfxRect *aOverrideBounds)
{
  gfxMatrix bboxMatrix;

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits != SVG_UNIT_TYPE_USERSPACEONUSE) {
    NS_ASSERTION(gradientUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
                 "Unknown gradientUnits type");
    

    gfxRect bbox =
      aOverrideBounds ? *aOverrideBounds : nsSVGUtils::GetBBox(aSource);
    bboxMatrix =
      gfxMatrix(bbox.Width(), 0, 0, bbox.Height(), bbox.X(), bbox.Y());
  }

  const nsSVGAnimatedTransformList* animTransformList =
    GetGradientTransformList(mContent);
  if (!animTransformList)
    return bboxMatrix;

  gfxMatrix gradientTransform =
    animTransformList->GetAnimValue().GetConsolidationMatrix();
  return bboxMatrix.PreMultiply(gradientTransform);
}

dom::SVGLinearGradientElement*
nsSVGGradientFrame::GetLinearGradientWithLength(uint32_t aIndex,
  dom::SVGLinearGradientElement* aDefault)
{
  
  
  

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetLinearGradientWithLength(aIndex, aDefault) : aDefault;
}

dom::SVGRadialGradientElement*
nsSVGGradientFrame::GetRadialGradientWithLength(uint32_t aIndex,
  dom::SVGRadialGradientElement* aDefault)
{
  
  
  

  AutoGradientReferencer gradientRef(this);

  nsSVGGradientFrame *next = GetReferencedGradientIfNotInUse();
  return next ? next->GetRadialGradientWithLength(aIndex, aDefault) : aDefault;
}





static void GetStopInformation(nsIFrame* aStopFrame,
                               float *aOffset,
                               nscolor *aStopColor,
                               float *aStopOpacity)
{
  nsIContent* stopContent = aStopFrame->GetContent();
  MOZ_ASSERT(stopContent && stopContent->IsSVG(nsGkAtoms::stop));

  static_cast<SVGStopElement*>(stopContent)->
    GetAnimatedNumberValues(aOffset, nullptr);

  *aOffset = mozilla::clamped(*aOffset, 0.0f, 1.0f);
  *aStopColor = aStopFrame->StyleSVGReset()->mStopColor;
  *aStopOpacity = aStopFrame->StyleSVGReset()->mStopOpacity;
}

already_AddRefed<gfxPattern>
nsSVGGradientFrame::GetPaintServerPattern(nsIFrame *aSource,
                                          const gfxMatrix& aContextMatrix,
                                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                          float aGraphicOpacity,
                                          const gfxRect *aOverrideBounds)
{
  uint16_t gradientUnits = GetGradientUnits();
  MOZ_ASSERT(gradientUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX ||
             gradientUnits == SVG_UNIT_TYPE_USERSPACEONUSE);
  if (gradientUnits == SVG_UNIT_TYPE_USERSPACEONUSE) {
    
    
    
    mSource = aSource->GetContent()->IsNodeOfType(nsINode::eTEXT) ?
                aSource->GetParent() : aSource;
  }

  nsAutoTArray<nsIFrame*,8> stopFrames;
  GetStopFrames(&stopFrames);

  uint32_t nStops = stopFrames.Length();

  
  
  if (nStops == 0) {
    nsRefPtr<gfxPattern> pattern = new gfxPattern(gfxRGBA(0, 0, 0, 0));
    return pattern.forget();
  }

  if (nStops == 1 || GradientVectorLengthIsZero()) {
    
    
    float stopOpacity = stopFrames[nStops-1]->StyleSVGReset()->mStopOpacity;
    nscolor stopColor = stopFrames[nStops-1]->StyleSVGReset()->mStopColor;

    nsRefPtr<gfxPattern> pattern = new gfxPattern(
                           gfxRGBA(NS_GET_R(stopColor)/255.0,
                                   NS_GET_G(stopColor)/255.0,
                                   NS_GET_B(stopColor)/255.0,
                                   NS_GET_A(stopColor)/255.0 *
                                     stopOpacity * aGraphicOpacity));
    return pattern.forget();
  }

  
  
  
  gfxMatrix patternMatrix = GetGradientTransform(aSource, aOverrideBounds);

  if (patternMatrix.IsSingular()) {
    return nullptr;
  }

  
  if (aFillOrStroke == &nsStyleSVG::mStroke) {
    gfxMatrix nonScalingStrokeTM = nsSVGUtils::GetStrokeTransform(aSource);
    if (!nonScalingStrokeTM.Invert()) {
      return nullptr;
    }
    patternMatrix *= nonScalingStrokeTM;
  }

  if (!patternMatrix.Invert()) {
    return nullptr;
  }

  nsRefPtr<gfxPattern> gradient = CreateGradient();
  if (!gradient || gradient->CairoStatus())
    return nullptr;

  uint16_t aSpread = GetSpreadMethod();
  if (aSpread == SVG_SPREADMETHOD_PAD)
    gradient->SetExtend(gfxPattern::EXTEND_PAD);
  else if (aSpread == SVG_SPREADMETHOD_REFLECT)
    gradient->SetExtend(gfxPattern::EXTEND_REFLECT);
  else if (aSpread == SVG_SPREADMETHOD_REPEAT)
    gradient->SetExtend(gfxPattern::EXTEND_REPEAT);

  gradient->SetMatrix(patternMatrix);

  
  float lastOffset = 0.0f;

  for (uint32_t i = 0; i < nStops; i++) {
    float offset, stopOpacity;
    nscolor stopColor;

    GetStopInformation(stopFrames[i], &offset, &stopColor, &stopOpacity);

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
    
    dom::SVGGradientElement*grad = static_cast<dom::SVGGradientElement*>(mContent);
    nsAutoString href;
    grad->mStringAttributes[dom::SVGGradientElement::HREF].GetAnimValue(href, grad);
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

void
nsSVGGradientFrame::GetStopFrames(nsTArray<nsIFrame*>* aStopFrames)
{
  nsIFrame *stopFrame = nullptr;
  for (stopFrame = mFrames.FirstChild(); stopFrame;
       stopFrame = stopFrame->GetNextSibling()) {
    if (stopFrame->GetType() == nsGkAtoms::svgStopFrame) {
      aStopFrames->AppendElement(stopFrame);
    }
  }
  if (aStopFrames->Length() > 0) {
    return;
  }

  

  AutoGradientReferencer gradientRef(this);
  nsSVGGradientFrame* next = GetReferencedGradientIfNotInUse();
  if (!next) {
    return;
  }

  return next->GetStopFrames(aStopFrames);
}





#ifdef DEBUG
void
nsSVGLinearGradientFrame::Init(nsIContent*       aContent,
                               nsContainerFrame* aParent,
                               nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::linearGradient),
               "Content is not an SVG linearGradient");

  nsSVGLinearGradientFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom*
nsSVGLinearGradientFrame::GetType() const
{
  return nsGkAtoms::svgLinearGradientFrame;
}

nsresult
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
  dom::SVGLinearGradientElement* lengthElement =
    GetLinearGradientWithLength(aIndex,
      static_cast<dom::SVGLinearGradientElement*>(mContent));
  
  
  NS_ABORT_IF_FALSE(lengthElement,
    "Got unexpected null element from GetLinearGradientWithLength");
  const nsSVGLength2 &length = lengthElement->mLengthAttributes[aIndex];

  
  
  

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits == SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSource, &length);
  }

  NS_ASSERTION(
    gradientUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
    "Unknown gradientUnits type");

  return length.GetAnimValue(static_cast<SVGSVGElement*>(nullptr));
}

dom::SVGLinearGradientElement*
nsSVGLinearGradientFrame::GetLinearGradientWithLength(uint32_t aIndex,
  dom::SVGLinearGradientElement* aDefault)
{
  dom::SVGLinearGradientElement* thisElement =
    static_cast<dom::SVGLinearGradientElement*>(mContent);
  const nsSVGLength2 &length = thisElement->mLengthAttributes[aIndex];

  if (length.IsExplicitlySet()) {
    return thisElement;
  }

  return nsSVGLinearGradientFrameBase::GetLinearGradientWithLength(aIndex,
                                                                   aDefault);
}

bool
nsSVGLinearGradientFrame::GradientVectorLengthIsZero()
{
  return GetLengthValue(dom::SVGLinearGradientElement::ATTR_X1) ==
         GetLengthValue(dom::SVGLinearGradientElement::ATTR_X2) &&
         GetLengthValue(dom::SVGLinearGradientElement::ATTR_Y1) ==
         GetLengthValue(dom::SVGLinearGradientElement::ATTR_Y2);
}

already_AddRefed<gfxPattern>
nsSVGLinearGradientFrame::CreateGradient()
{
  float x1, y1, x2, y2;

  x1 = GetLengthValue(dom::SVGLinearGradientElement::ATTR_X1);
  y1 = GetLengthValue(dom::SVGLinearGradientElement::ATTR_Y1);
  x2 = GetLengthValue(dom::SVGLinearGradientElement::ATTR_X2);
  y2 = GetLengthValue(dom::SVGLinearGradientElement::ATTR_Y2);

  nsRefPtr<gfxPattern> pattern = new gfxPattern(x1, y1, x2, y2);
  return pattern.forget();
}





#ifdef DEBUG
void
nsSVGRadialGradientFrame::Init(nsIContent*       aContent,
                               nsContainerFrame* aParent,
                               nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::radialGradient),
               "Content is not an SVG radialGradient");

  nsSVGRadialGradientFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom*
nsSVGRadialGradientFrame::GetType() const
{
  return nsGkAtoms::svgRadialGradientFrame;
}

nsresult
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
  dom::SVGRadialGradientElement* lengthElement =
    GetRadialGradientWithLength(aIndex,
      static_cast<dom::SVGRadialGradientElement*>(mContent));
  
  
  NS_ABORT_IF_FALSE(lengthElement,
    "Got unexpected null element from GetRadialGradientWithLength");
  return GetLengthValueFromElement(aIndex, *lengthElement);
}

float
nsSVGRadialGradientFrame::GetLengthValue(uint32_t aIndex, float aDefaultValue)
{
  dom::SVGRadialGradientElement* lengthElement =
    GetRadialGradientWithLength(aIndex, nullptr);

  return lengthElement ? GetLengthValueFromElement(aIndex, *lengthElement)
                       : aDefaultValue;
}

float
nsSVGRadialGradientFrame::GetLengthValueFromElement(uint32_t aIndex,
  dom::SVGRadialGradientElement& aElement)
{
  const nsSVGLength2 &length = aElement.mLengthAttributes[aIndex];

  
  
  

  uint16_t gradientUnits = GetGradientUnits();
  if (gradientUnits == SVG_UNIT_TYPE_USERSPACEONUSE) {
    return nsSVGUtils::UserSpace(mSource, &length);
  }

  NS_ASSERTION(
    gradientUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX,
    "Unknown gradientUnits type");

  return length.GetAnimValue(static_cast<SVGSVGElement*>(nullptr));
}

dom::SVGRadialGradientElement*
nsSVGRadialGradientFrame::GetRadialGradientWithLength(uint32_t aIndex,
  dom::SVGRadialGradientElement* aDefault)
{
  dom::SVGRadialGradientElement* thisElement =
    static_cast<dom::SVGRadialGradientElement*>(mContent);
  const nsSVGLength2 &length = thisElement->mLengthAttributes[aIndex];

  if (length.IsExplicitlySet()) {
    return thisElement;
  }

  return nsSVGRadialGradientFrameBase::GetRadialGradientWithLength(aIndex,
                                                                   aDefault);
}

bool
nsSVGRadialGradientFrame::GradientVectorLengthIsZero()
{
  return GetLengthValue(dom::SVGRadialGradientElement::ATTR_R) == 0;
}

already_AddRefed<gfxPattern>
nsSVGRadialGradientFrame::CreateGradient()
{
  float cx, cy, r, fx, fy;

  cx = GetLengthValue(dom::SVGRadialGradientElement::ATTR_CX);
  cy = GetLengthValue(dom::SVGRadialGradientElement::ATTR_CY);
  r  = GetLengthValue(dom::SVGRadialGradientElement::ATTR_R);
  
  fx = GetLengthValue(dom::SVGRadialGradientElement::ATTR_FX, cx);
  fy = GetLengthValue(dom::SVGRadialGradientElement::ATTR_FY, cy);

  if (fx != cx || fy != cy) {
    
    
    
    
    
    
    
    double dMax = std::max(0.0, r - 1.0/128);
    float dx = fx - cx;
    float dy = fy - cy;
    double d = sqrt((dx * dx) + (dy * dy));
    if (d > dMax) {
      double angle = atan2(dy, dx);
      fx = (float)(dMax * cos(angle)) + cx;
      fy = (float)(dMax * sin(angle)) + cy;
    }
  }

  nsRefPtr<gfxPattern> pattern = new gfxPattern(fx, fy, 0, cx, cy, r);
  return pattern.forget();
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
