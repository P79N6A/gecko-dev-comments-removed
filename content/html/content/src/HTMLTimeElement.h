




#ifndef mozilla_dom_HTMLTimeElement_h
#define mozilla_dom_HTMLTimeElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLTimeElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLTimeElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual ~HTMLTimeElement();

  
  void GetDateTime(nsAString& aDateTime)
  {
    GetHTMLAttr(nsGkAtoms::datetime, aDateTime);
  }

  void SetDateTime(const nsAString& aDateTime, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::datetime, aDateTime, aError);
  }

  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

protected:
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
