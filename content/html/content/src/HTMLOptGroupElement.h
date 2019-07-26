




#ifndef mozilla_dom_HTMLOptGroupElement_h
#define mozilla_dom_HTMLOptGroupElement_h

#include "nsIDOMHTMLOptGroupElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLOptGroupElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLOptGroupElement
{
public:
  HTMLOptGroupElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLOptGroupElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLOPTGROUPELEMENT

  
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual nsEventStates IntrinsicState() const;
 
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual bool IsDisabled() const {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
  }

  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aValue, ErrorResult& aError)
  {
     SetHTMLBoolAttr(nsGkAtoms::disabled, aValue, aError);
  }

  
  void SetLabel(const nsAString& aLabel, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::label, aLabel, aError);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;

protected:

  



  nsIContent* GetSelect();
};

} 
} 

#endif 
