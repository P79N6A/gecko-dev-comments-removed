




#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "mozilla/Attributes.h"
#include "nsImageLoadingContent.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"
#include "nsSVGElement.h"
#include "nsSVGNumber2.h"
#include "nsSVGNumberPair.h"
#include "FilterSupport.h"

class nsSVGFilterInstance;
class nsSVGNumberPair;

struct nsSVGStringInfo {
  nsSVGStringInfo(const nsSVGString* aString,
                  nsSVGElement *aElement) :
    mString(aString), mElement(aElement) {}

  const nsSVGString* mString;
  nsSVGElement* mElement;
};

typedef nsSVGElement nsSVGFEBase;

#define NS_SVG_FE_CID \
{ 0x60483958, 0xd229, 0x4a77, \
  { 0x96, 0xb2, 0x62, 0x3e, 0x69, 0x95, 0x1e, 0x0e } }






class nsSVGFE : public nsSVGFEBase
{
  friend class nsSVGFilterInstance;

protected:
  typedef mozilla::gfx::SourceSurface SourceSurface;
  typedef mozilla::gfx::Size Size;
  typedef mozilla::gfx::IntRect IntRect;
  typedef mozilla::gfx::ColorSpace ColorSpace;
  typedef mozilla::gfx::FilterPrimitiveDescription FilterPrimitiveDescription;

  explicit nsSVGFE(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsSVGFEBase(aNodeInfo) {}
  virtual ~nsSVGFE() {}

public:
  typedef mozilla::gfx::AttributeMap AttributeMap;

  ColorSpace
  GetInputColorSpace(int32_t aInputIndex, ColorSpace aUnchangedInputColorSpace) {
    return OperatesOnSRGB(aInputIndex, aUnchangedInputColorSpace == ColorSpace::SRGB) ?
             ColorSpace::SRGB : ColorSpace::LinearRGB;
  }

  
  
  ColorSpace
  GetOutputColorSpace() {
    return ProducesSRGB() ? ColorSpace::SRGB : ColorSpace::LinearRGB;
  }

  
  virtual bool SubregionIsUnionOfRegions() { return true; }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVG_FE_CID)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override = 0;

  virtual bool HasValidDimensions() const override;

  bool IsNodeOfType(uint32_t aFlags) const override
    { return !(aFlags & ~(eCONTENT | eFILTER)); }

  virtual nsSVGString& GetResultImageName() = 0;
  
  
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources);

  virtual FilterPrimitiveDescription
    GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                            const IntRect& aFilterSubregion,
                            const nsTArray<bool>& aInputsAreTainted,
                            nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages) = 0;

  
  
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const;

  
  
  
  
  virtual bool OutputIsTainted(const nsTArray<bool>& aInputsAreTainted,
                               nsIPrincipal* aReferencePrincipal);

  static nsIntRect GetMaxRect() {
    
    
    return nsIntRect(INT32_MIN/2, INT32_MIN/2, INT32_MAX, INT32_MAX);
  }

  operator nsISupports*() { return static_cast<nsIContent*>(this); }

  
  already_AddRefed<mozilla::dom::SVGAnimatedLength> X();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Y();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Width();
  already_AddRefed<mozilla::dom::SVGAnimatedLength> Height();
  already_AddRefed<mozilla::dom::SVGAnimatedString> Result();

protected:
  virtual bool OperatesOnSRGB(int32_t aInputIndex, bool aInputIsAlreadySRGB) {
    return StyleIsSetToSRGB();
  }

  
  virtual bool ProducesSRGB() { return StyleIsSetToSRGB(); }

  bool StyleIsSetToSRGB();

  
  virtual LengthAttributesInfo GetLengthInfo() override;

  Size GetKernelUnitLength(nsSVGFilterInstance* aInstance,
                          nsSVGNumberPair *aKernelUnitLength);

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGFE, NS_SVG_FE_CID)

typedef nsSVGElement SVGFEUnstyledElementBase;

class SVGFEUnstyledElement : public SVGFEUnstyledElementBase
{
protected:
  explicit SVGFEUnstyledElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFEUnstyledElementBase(aNodeInfo) {}

public:
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override = 0;

  
  
  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const = 0;
};



typedef nsSVGFE nsSVGFELightingElementBase;

class nsSVGFELightingElement : public nsSVGFELightingElementBase
{
protected:
  explicit nsSVGFELightingElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsSVGFELightingElementBase(aNodeInfo) {}

  virtual ~nsSVGFELightingElement() {}

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual bool AttributeAffectsRendering(
          int32_t aNameSpaceID, nsIAtom* aAttribute) const override;
  virtual nsSVGString& GetResultImageName() override { return mStringAttributes[RESULT]; }
  virtual void GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources) override;
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFELightingElementBase::)

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;

protected:
  virtual bool OperatesOnSRGB(int32_t aInputIndex,
                              bool aInputIsAlreadySRGB) override { return true; }

  virtual NumberAttributesInfo GetNumberInfo() override;
  virtual NumberPairAttributesInfo GetNumberPairInfo() override;
  virtual StringAttributesInfo GetStringInfo() override;

  AttributeMap ComputeLightAttributes(nsSVGFilterInstance* aInstance);

  FilterPrimitiveDescription
    AddLightingAttributes(FilterPrimitiveDescription aDescription,
                          nsSVGFilterInstance* aInstance);

  enum { SURFACE_SCALE, DIFFUSE_CONSTANT, SPECULAR_CONSTANT, SPECULAR_EXPONENT };
  nsSVGNumber2 mNumberAttributes[4];
  static NumberInfo sNumberInfo[4];

  enum { KERNEL_UNIT_LENGTH };
  nsSVGNumberPair mNumberPairAttributes[1];
  static NumberPairInfo sNumberPairInfo[1];

  enum { RESULT, IN1 };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};

namespace mozilla {
namespace dom {

typedef SVGFEUnstyledElement SVGFELightElementBase;

class SVGFELightElement : public SVGFELightElementBase
{
protected:
  explicit SVGFELightElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : SVGFELightElementBase(aNodeInfo) {}

public:
  typedef gfx::AttributeMap AttributeMap;

  virtual AttributeMap
    ComputeLightAttributes(nsSVGFilterInstance* aInstance) = 0;
};

}
}

#endif
