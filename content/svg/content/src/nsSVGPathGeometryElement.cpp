




#include "nsSVGPathGeometryElement.h"

#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "nsComputedDOMStyle.h"
#include "nsSVGLength2.h"
#include "SVGContentUtils.h"

using namespace mozilla;
using namespace mozilla::gfx;




nsSVGPathGeometryElement::nsSVGPathGeometryElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsSVGPathGeometryElementBase(aNodeInfo)
{
}

bool
nsSVGPathGeometryElement::AttributeDefinesGeometry(const nsIAtom *aName)
{
  
  LengthAttributesInfo info = GetLengthInfo();
  for (uint32_t i = 0; i < info.mLengthCount; i++) {
    if (aName == *info.mLengthInfo[i].mName) {
      return true;
    }
  }

  return false;
}

bool
nsSVGPathGeometryElement::GeometryDependsOnCoordCtx()
{
  
  LengthAttributesInfo info = const_cast<nsSVGPathGeometryElement*>(this)->GetLengthInfo();
  for (uint32_t i = 0; i < info.mLengthCount; i++) {
    if (info.mLengths[i].GetSpecifiedUnitType() == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
      return true;
    }   
  }
  return false;
}

bool
nsSVGPathGeometryElement::IsMarkable()
{
  return false;
}

void
nsSVGPathGeometryElement::GetMarkPoints(nsTArray<nsSVGMark> *aMarks)
{
}

TemporaryRef<Path>
nsSVGPathGeometryElement::GetPathForLengthOrPositionMeasuring()
{
  return nullptr;
}

TemporaryRef<PathBuilder>
nsSVGPathGeometryElement::CreatePathBuilder()
{
  RefPtr<DrawTarget> drawTarget =
    gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
  NS_ASSERTION(gfxPlatform::GetPlatform()->
                 SupportsAzureContentForDrawTarget(drawTarget),
               "Should support Moz2D content drawing");

  
  
  
  
  
  
  

  return drawTarget->CreatePathBuilder(GetFillRule());
}

FillRule
nsSVGPathGeometryElement::GetFillRule()
{
  FillRule fillRule = FillRule::FILL_WINDING; 

  nsRefPtr<nsStyleContext> styleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(this, nullptr,
                                                         nullptr);
  
  if (styleContext) {
    MOZ_ASSERT(styleContext->StyleSVG()->mFillRule ==
                                           NS_STYLE_FILL_RULE_NONZERO ||
               styleContext->StyleSVG()->mFillRule ==
                                           NS_STYLE_FILL_RULE_EVENODD);

    if (styleContext->StyleSVG()->mFillRule == NS_STYLE_FILL_RULE_EVENODD) {
      fillRule = FillRule::FILL_EVEN_ODD;
    }
  } else {
    
    NS_WARNING("Couldn't get style context for content in GetFillRule");
  }

  return fillRule;
}
