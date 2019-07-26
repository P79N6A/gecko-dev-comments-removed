




#ifndef mozilla_dom_HTMLDataElement_h
#define mozilla_dom_HTMLDataElement_h

#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLDataElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLElement
{
public:
  HTMLDataElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLDataElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  void GetValue(nsAString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::value, aValue);
  }

  void SetValue(const nsAString& aValue, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::value, aValue, aError);
  }

  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;
};

} 
} 

#endif 
