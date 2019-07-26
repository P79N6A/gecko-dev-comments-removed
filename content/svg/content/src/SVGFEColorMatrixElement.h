




#ifndef mozilla_dom_SVGFEColorMatrixElement_h
#define mozilla_dom_SVGFEColorMatrixElement_h

#include "nsSVGEnum.h"
#include "nsSVGFilters.h"
#include "SVGAnimatedNumberList.h"

nsresult NS_NewSVGFEColorMatrixElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGFE SVGFEColorMatrixElementBase;

class SVGFEColorMatrixElement : public SVGFEColorMatrixElementBase
{
  friend nsresult (::NS_NewSVGFEColorMatrixElement(nsIContent **aResult,
                                                   already_AddRefed<nsINodeInfo> aNodeInfo));
protected:
  SVGFEColorMatrixElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : SVGFEColorMatrixElementBase(aNodeInfo)
  {
  }
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

public:
  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) MOZ_OVERRIDE;
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsSVGString& GetResultImageName() MOZ_OVERRIDE { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources) MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedString> In1();
  already_AddRefed<SVGAnimatedEnumeration> Type();
  already_AddRefed<DOMSVGAnimatedNumberList> Values();

 protected:
  virtual EnumAttributesInfo GetEnumInfo() MOZ_OVERRIDE;
  virtual StringAttributesInfo GetStringInfo() MOZ_OVERRIDE;
  virtual NumberListAttributesInfo GetNumberListInfo() MOZ_OVERRIDE;

  enum { TYPE };
  nsSVGEnum mEnumAttributes[1];
  static nsSVGEnumMapping sTypeMap[];
  static EnumInfo sEnumInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  enum { VALUES };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};

} 
} 

#endif 
