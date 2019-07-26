




#include "HTMLDataListElement.h"
#include "mozilla/dom/HTMLDataListElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(DataList)

namespace mozilla {
namespace dom {

HTMLDataListElement::~HTMLDataListElement()
{
}

JSObject*
HTMLDataListElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return HTMLDataListElementBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLDataListElement,
                                                nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLDataListElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLDataListElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLDataListElement, Element)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLDataListElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLDataListElement,
                                   nsIDOMHTMLDataListElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLDataListElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLDataListElement)

bool
HTMLDataListElement::MatchOptions(nsIContent* aContent, int32_t aNamespaceID,
                                  nsIAtom* aAtom, void* aData)
{
  return aContent->NodeInfo()->Equals(nsGkAtoms::option, kNameSpaceID_XHTML) &&
         !aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
}

NS_IMETHODIMP
HTMLDataListElement::GetOptions(nsIDOMHTMLCollection** aOptions)
{
  NS_ADDREF(*aOptions = Options());

  return NS_OK;
}

} 
} 
