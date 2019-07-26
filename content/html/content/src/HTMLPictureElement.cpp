





#include "mozilla/dom/HTMLPictureElement.h"
#include "mozilla/dom/HTMLPictureElementBinding.h"
#include "mozilla/dom/HTMLImageElement.h"

#include "mozilla/Preferences.h"
static const char *kPrefPictureEnabled = "dom.image.picture.enabled";


nsGenericHTMLElement*
NS_NewHTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                         mozilla::dom::FromParser aFromParser)
{
  if (!mozilla::dom::HTMLPictureElement::IsPictureEnabled()) {
    return new mozilla::dom::HTMLUnknownElement(aNodeInfo);
  }

  return new mozilla::dom::HTMLPictureElement(aNodeInfo);
}

namespace mozilla {
namespace dom {

HTMLPictureElement::HTMLPictureElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLPictureElement::~HTMLPictureElement()
{
}

NS_IMPL_ISUPPORTS_INHERITED(HTMLPictureElement, nsGenericHTMLElement,
                            nsIDOMHTMLPictureElement)

NS_IMPL_ELEMENT_CLONE(HTMLPictureElement)

bool
HTMLPictureElement::IsPictureEnabled()
{
  return HTMLImageElement::IsSrcsetEnabled() &&
         Preferences::GetBool(kPrefPictureEnabled, false);
}

JSObject*
HTMLPictureElement::WrapNode(JSContext* aCx)
{
  return HTMLPictureElementBinding::Wrap(aCx, this);
}

} 
} 
