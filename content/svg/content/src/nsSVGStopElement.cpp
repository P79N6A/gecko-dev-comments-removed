





































#include "nsSVGStylableElement.h"
#include "nsIDOMSVGStopElement.h"
#include "nsSVGAnimatedNumberList.h"
#include "nsSVGNumber2.h"
#include "nsGenericHTMLElement.h"
#include "prdtoa.h"

typedef nsSVGStylableElement nsSVGStopElementBase;

class nsSVGStopElement : public nsSVGStopElementBase,
                         public nsIDOMSVGStopElement
{
protected:
  friend nsresult NS_NewSVGStopElement(nsIContent **aResult,
                                       nsINodeInfo *aNodeInfo);
  nsSVGStopElement(nsINodeInfo* aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTOPELEMENT

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGStopElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGStopElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGStopElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual NumberAttributesInfo GetNumberInfo();
  
  nsSVGNumber2 mOffset;
  static NumberInfo sNumberInfo;
};

nsSVGElement::NumberInfo nsSVGStopElement::sNumberInfo = { &nsGkAtoms::offset, 
                                                           0 };
NS_IMPL_NS_NEW_SVG_ELEMENT(Stop)




NS_IMPL_ADDREF_INHERITED(nsSVGStopElement,nsSVGStopElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStopElement,nsSVGStopElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGStopElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGStopElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGStopElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStopElementBase)




nsSVGStopElement::nsSVGStopElement(nsINodeInfo* aNodeInfo)
  : nsSVGStopElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGStopElement)





NS_IMETHODIMP nsSVGStopElement::GetOffset(nsIDOMSVGAnimatedNumber * *aOffset)
{
  return mOffset.ToDOMAnimatedNumber(aOffset,this);
}




nsSVGElement::NumberAttributesInfo
nsSVGStopElement::GetNumberInfo()
{
  return NumberAttributesInfo(&mOffset, &sNumberInfo, 1);
}

PRBool
nsSVGStopElement::ParseAttribute(PRInt32 aNamespaceID,
								 nsIAtom* aAttribute,
								 const nsAString& aValue,
								 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::offset) {
      NS_ConvertUTF16toUTF8 value(aValue);
      const char *str = value.get();

      char *rest;
      float offset = static_cast<float>(PR_strtod(str, &rest));
      if (str != rest) {
        if (*rest == '%') {
          offset /= 100;
          ++rest;
        }
        if (*rest == '\0') {
          mOffset.SetBaseValue(offset, this, PR_FALSE);
          aResult.SetTo(aValue);
          return PR_TRUE;
        }
      }
    }
  }
  return nsSVGElement::ParseAttribute(aNamespaceID, aAttribute,
                                      aValue, aResult);
}




NS_IMETHODIMP_(PRBool)
nsSVGStopElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sGradientStopMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGStopElementBase::IsAttributeMapped(name);
}


