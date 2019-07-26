




#ifndef mozilla_dom_HTMLPreElement_h
#define mozilla_dom_HTMLPreElement_h

#include "mozilla/Util.h"

#include "nsIDOMHTMLPreElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLPreElement : public nsGenericHTMLElement,
                       public nsIDOMHTMLPreElement
{
public:
  HTMLPreElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLPreElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_IMETHOD GetWidth(int32_t* aWidth);
  NS_IMETHOD SetWidth(int32_t aWidth);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  int32_t Width() const
  {
    return GetIntAttr(nsGkAtoms::width, 0);
  }
  void SetWidth(int32_t aWidth, mozilla::ErrorResult& rv)
  {
    rv = SetIntAttr(nsGkAtoms::width, aWidth);
  }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
