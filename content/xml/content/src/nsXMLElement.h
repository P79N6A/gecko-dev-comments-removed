




#ifndef nsXMLElement_h___
#define nsXMLElement_h___

#include "nsIDOMElement.h"
#include "mozilla/dom/Element.h"

class nsXMLElement : public mozilla::dom::Element,
                     public nsIDOMElement
{
public:
  nsXMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : mozilla::dom::Element(aNodeInfo)
  {
    SetIsDOMBinding();
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual nsIAtom *GetIDAttributeName() const;
  virtual nsIAtom* DoGetID() const;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  
  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo);

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

#endif 
