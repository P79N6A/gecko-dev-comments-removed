





#include "mozilla/dom/HTMLSourceElement.h"
#include "mozilla/dom/HTMLSourceElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Source)

namespace mozilla {
namespace dom {

HTMLSourceElement::HTMLSourceElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLSourceElement::~HTMLSourceElement()
{
}


NS_IMPL_ADDREF_INHERITED(HTMLSourceElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSourceElement, Element)




NS_INTERFACE_TABLE_HEAD(HTMLSourceElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLSourceElement, nsIDOMHTMLSourceElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLSourceElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLSourceElement)


NS_IMPL_URI_ATTR(HTMLSourceElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Type, type)
NS_IMPL_STRING_ATTR(HTMLSourceElement, Media, media)

void
HTMLSourceElement::GetItemValueText(nsAString& aValue)
{
  GetSrc(aValue);
}

void
HTMLSourceElement::SetItemValueText(const nsAString& aValue)
{
  SetSrc(aValue);
}

nsresult
HTMLSourceElement::BindToTree(nsIDocument *aDocument,
                              nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aParent || !aParent->IsNodeOfType(nsINode::eMEDIA))
    return NS_OK;

  nsHTMLMediaElement* media = static_cast<nsHTMLMediaElement*>(aParent);
  media->NotifyAddedSource();

  return NS_OK;
}

JSObject*
HTMLSourceElement::WrapNode(JSContext* aCx, JSObject* aScope,
                            bool* aTriedToWrap)
{
  return HTMLSourceElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

} 
} 
