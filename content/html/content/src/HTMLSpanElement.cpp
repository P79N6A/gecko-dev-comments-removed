




#include "mozilla/dom/HTMLSpanElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Span)
DOMCI_NODE_DATA(HTMLSpanElement, mozilla::dom::HTMLSpanElement)

namespace mozilla {
namespace dom {

HTMLSpanElement::~HTMLSpanElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLSpanElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSpanElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLSpanElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(HTMLSpanElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLSpanElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLSpanElement)


NS_IMPL_ELEMENT_CLONE(HTMLSpanElement)

} 
} 
