



#ifndef mozilla_dom_HTMLTableRowElement_h
#define mozilla_dom_HTMLTableRowElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLTableRowElement.h"

class nsIDOMHTMLTableElement;
class nsIDOMHTMLTableSectionElement;

namespace mozilla {
namespace dom {

class HTMLTableRowElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLTableRowElement
{
public:
  HTMLTableRowElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTABLEROWELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLTableRowElement,
                                                     nsGenericHTMLElement)

protected:
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetSection() const;
  already_AddRefed<nsIDOMHTMLTableElement> GetTable() const;
  nsRefPtr<nsContentList> mCells;
};

} 
} 

#endif 
