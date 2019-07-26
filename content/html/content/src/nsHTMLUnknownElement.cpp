




#include "nsHTMLUnknownElement.h"

using namespace mozilla::dom;

NS_IMPL_ADDREF_INHERITED(nsHTMLUnknownElement, Element)
NS_IMPL_RELEASE_INHERITED(nsHTMLUnknownElement, Element)

NS_IMPL_NS_NEW_HTML_ELEMENT(Unknown)

DOMCI_NODE_DATA(HTMLUnknownElement, nsHTMLUnknownElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLUnknownElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLUnknownElement,
                                   nsIDOMHTMLUnknownElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLUnknownElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLUnknownElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLUnknownElement)
