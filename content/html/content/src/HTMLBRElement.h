




#ifndef mozilla_dom_HTMLBRElement_h
#define mozilla_dom_HTMLBRElement_h

#include "nsIDOMHTMLBRElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLBRElement MOZ_FINAL : public nsGenericHTMLElement,
                                public nsIDOMHTMLBRElement
{
public:
  HTMLBRElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLBRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLBRELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  bool Clear()
  {
    return GetBoolAttr(nsGkAtoms::clear);
  }
  void SetClear(const nsAString& aClear, ErrorResult& aError)
  {
    return SetHTMLAttr(nsGkAtoms::clear, aClear, aError);
  }

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope,
                             bool *aTriedToWrap);
};

} 
} 

#endif

