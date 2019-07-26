




#ifndef nsXMLElement_h___
#define nsXMLElement_h___

#include "mozilla/Attributes.h"
#include "nsIDOMElement.h"
#include "mozilla/dom/ElementInlines.h"
#include "mozilla/dom/DOMRect.h"

class nsXMLElement : public mozilla::dom::Element,
                     public nsIDOMElement
{
public:
  nsXMLElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
    : mozilla::dom::Element(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  
  virtual nsIAtom *GetIDAttributeName() const MOZ_OVERRIDE;
  virtual nsIAtom* DoGetID() const MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;

  
  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo) MOZ_OVERRIDE;

protected:
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;
};

#endif 
