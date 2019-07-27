





#ifndef mozilla_dom_HTMLPictureElement_h
#define mozilla_dom_HTMLPictureElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLPictureElement.h"
#include "nsGenericHTMLElement.h"

#include "mozilla/dom/HTMLUnknownElement.h"

namespace mozilla {
namespace dom {

class HTMLPictureElement final : public nsGenericHTMLElement,
                                 public nsIDOMHTMLPictureElement
{
public:
  explicit HTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLPICTUREELEMENT

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const override;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;

  static bool IsPictureEnabled();

protected:
  virtual ~HTMLPictureElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
};

} 
} 

#endif 
