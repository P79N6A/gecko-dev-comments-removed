




#include "mozilla/dom/HTMLMapElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsContentList.h"
#include "nsCOMPtr.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Map)

DOMCI_NODE_DATA(HTMLMapElement, mozilla::dom::HTMLMapElement)

namespace mozilla {
namespace dom {

HTMLMapElement::HTMLMapElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLMapElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAreas)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLMapElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLMapElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLMapElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLMapElement, nsIDOMHTMLMapElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLMapElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLMapElement)

NS_IMPL_ELEMENT_CLONE(HTMLMapElement)


NS_IMETHODIMP
HTMLMapElement::GetAreas(nsIDOMHTMLCollection** aAreas)
{
  NS_ENSURE_ARG_POINTER(aAreas);

  if (!mAreas) {
    
    mAreas = new nsContentList(this,
                               kNameSpaceID_XHTML,
                               nsGkAtoms::area,
                               nsGkAtoms::area,
                               false);
  }

  NS_ADDREF(*aAreas = mAreas);
  return NS_OK;
}


NS_IMPL_STRING_ATTR(HTMLMapElement, Name, name)

} 
} 
