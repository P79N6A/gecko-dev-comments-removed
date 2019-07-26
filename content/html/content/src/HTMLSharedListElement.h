




#ifndef mozilla_dom_HTMLSharedListElement_h
#define mozilla_dom_HTMLSharedListElement_h
#include "mozilla/Util.h"

#include "nsIDOMHTMLOListElement.h"
#include "nsIDOMHTMLDListElement.h"
#include "nsIDOMHTMLUListElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLSharedListElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLOListElement,
                              public nsIDOMHTMLDListElement,
                              public nsIDOMHTMLUListElement
{
public:
  HTMLSharedListElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLSharedListElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLOLISTELEMENT

  
  

  
  

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode()
  {
    return static_cast<nsIDOMHTMLOListElement*>(this);
  }

  bool Reversed() const
  {
    return GetBoolAttr(nsGkAtoms::reversed);
  }
  void SetReversed(bool aReversed, mozilla::ErrorResult& rv)
  {
    SetHTMLBoolAttr(nsGkAtoms::reversed, aReversed, rv);
  }
  int32_t Start() const
  {
    return GetIntAttr(nsGkAtoms::start, 1);
  }
  void SetStart(int32_t aStart, mozilla::ErrorResult& rv)
  {
    SetHTMLIntAttr(nsGkAtoms::start, aStart, rv);
  }
  void GetType(nsString& aType)
  {
    GetHTMLAttr(nsGkAtoms::type, aType);
  }
  void SetType(const nsAString& aType, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, rv);
  }
  bool Compact() const
  {
    return GetBoolAttr(nsGkAtoms::compact);
  }
  void SetCompact(bool aCompact, mozilla::ErrorResult& rv)
  {
    SetHTMLBoolAttr(nsGkAtoms::compact, aCompact, rv);
  }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
