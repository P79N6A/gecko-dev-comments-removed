




#include "HTMLDataListElement.h"
#include "mozilla/dom/HTMLDataListElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(DataList)

namespace mozilla {
namespace dom {

HTMLDataListElement::~HTMLDataListElement()
{
}

JSObject*
HTMLDataListElement::WrapNode(JSContext *aCx)
{
  return HTMLDataListElementBinding::Wrap(aCx, this);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(HTMLDataListElement, nsGenericHTMLElement,
                                   mOptions)

NS_IMPL_ADDREF_INHERITED(HTMLDataListElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLDataListElement, Element)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(HTMLDataListElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)


NS_IMPL_ELEMENT_CLONE(HTMLDataListElement)

bool
HTMLDataListElement::MatchOptions(nsIContent* aContent, int32_t aNamespaceID,
                                  nsIAtom* aAtom, void* aData)
{
  return aContent->NodeInfo()->Equals(nsGkAtoms::option, kNameSpaceID_XHTML) &&
         !aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
}

} 
} 
