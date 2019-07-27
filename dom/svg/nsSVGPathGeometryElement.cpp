





#include "nsSVGPathGeometryElement.h"

#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "nsComputedDOMStyle.h"
#include "nsSVGUtils.h"
#include "nsSVGLength2.h"
#include "SVGContentUtils.h"

using namespace mozilla;
using namespace mozilla::gfx;




nsSVGPathGeometryElement::nsSVGPathGeometryElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsSVGPathGeometryElementBase(aNodeInfo)
{
}

nsresult
nsSVGPathGeometryElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                       const nsAttrValue* aValue, bool aNotify)
{
  if (mCachedPath &&
      aNamespaceID == kNameSpaceID_None &&
      AttributeDefinesGeometry(aName)) {
    mCachedPath = nullptr;
  }
  return nsSVGPathGeometryElementBase::AfterSetAttr(aNamespaceID, aName,
                                                    aValue, aNotify);
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

already_AddRefed<Path>
nsSVGPathGeometryElement::GetOrBuildPath(const DrawTarget& aDrawTarget,
                                         FillRule aFillRule)
{
  
  bool cacheable  = aDrawTarget.GetBackendType() ==
                      gfxPlatform::GetPlatform()->GetContentBackend();

  
  
  
  
  if (cacheable && mCachedPath) {
    if (aDrawTarget.GetBackendType() == mCachedPath->GetBackendType()) {
      RefPtr<Path> path(mCachedPath);
      return path.forget();
    }
  }
  RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder(aFillRule);
  RefPtr<Path> path = BuildPath(builder);
  if (cacheable && NS_SVGPathCachingEnabled()) {
    mCachedPath = path;
  }
  return path.forget();
}

already_AddRefed<Path>
nsSVGPathGeometryElement::GetOrBuildPathForMeasuring()
{
  return nullptr;
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
