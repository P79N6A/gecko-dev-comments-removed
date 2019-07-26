




#ifndef mozilla_dom_HTMLTimeElement_h
#define mozilla_dom_HTMLTimeElement_h

#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLTimeElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLElement
{
public:
  HTMLTimeElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLTimeElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  void GetDateTime(nsAString& aDateTime)
  {
    GetHTMLAttr(nsGkAtoms::datetime, aDateTime);
  }

  void SetDateTime(const nsAString& aDateTime, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::datetime, aDateTime, aError);
  }

  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
