





#ifndef mozilla_dom_HTMLPictureElement_h
#define mozilla_dom_HTMLPictureElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLPictureElement.h"
#include "nsGenericHTMLElement.h"

#include "mozilla/dom/HTMLUnknownElement.h"

namespace mozilla {
namespace dom {

class HTMLPictureElement MOZ_FINAL : public nsGenericHTMLElement,
                                    public nsIDOMHTMLPictureElement
{
public:
  HTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual ~HTMLPictureElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLPICTUREELEMENT

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  static bool IsPictureEnabled();

protected:
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
