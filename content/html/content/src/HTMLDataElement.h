




#ifndef mozilla_dom_HTMLDataElement_h
#define mozilla_dom_HTMLDataElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLDataElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLDataElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  void GetValue(nsAString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::value, aValue);
  }

  void SetValue(const nsAString& aValue, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::value, aValue, aError);
  }

  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

protected:
  virtual ~HTMLDataElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
