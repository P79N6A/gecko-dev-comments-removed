




#ifndef mozilla_dom_HTMLModElement_h
#define mozilla_dom_HTMLModElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLModElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLModElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  void GetCite(nsString& aCite)
  {
    GetHTMLURIAttr(nsGkAtoms::cite, aCite);
  }
  void SetCite(const nsAString& aCite, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::cite, aCite, aRv);
  }
  void GetDateTime(nsAString& aDateTime)
  {
    GetHTMLAttr(nsGkAtoms::datetime, aDateTime);
  }
  void SetDateTime(const nsAString& aDateTime, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::datetime, aDateTime, aRv);
  }

protected:
  virtual ~HTMLModElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
