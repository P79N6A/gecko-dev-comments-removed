




#include "mozilla/dom/SVGTitleElement.h"
#include "mozilla/dom/SVGTitleElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Title)

namespace mozilla {
namespace dom {

JSObject*
SVGTitleElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGTitleElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED4(SVGTitleElement, SVGTitleElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIMutationObserver)




SVGTitleElement::SVGTitleElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTitleElementBase(aNodeInfo)
{
  AddMutationObserver(this);
}

void
SVGTitleElement::CharacterDataChanged(nsIDocument *aDocument,
                                      nsIContent *aContent,
                                      CharacterDataChangeInfo *aInfo)
{
  SendTitleChangeEvent(false);
}

void
SVGTitleElement::ContentAppended(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aFirstNewContent,
                                 int32_t aNewIndexInContainer)
{
  SendTitleChangeEvent(false);
}

void
SVGTitleElement::ContentInserted(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aChild,
                                 int32_t aIndexInContainer)
{
  SendTitleChangeEvent(false);
}

void
SVGTitleElement::ContentRemoved(nsIDocument *aDocument,
                                nsIContent *aContainer,
                                nsIContent *aChild,
                                int32_t aIndexInContainer,
                                nsIContent *aPreviousSibling)
{
  SendTitleChangeEvent(false);
}

nsresult
SVGTitleElement::BindToTree(nsIDocument *aDocument,
                             nsIContent *aParent,
                             nsIContent *aBindingParent,
                             bool aCompileEventHandlers)
{
  
  nsresult rv = SVGTitleElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  SendTitleChangeEvent(true);

  return NS_OK;
}

void
SVGTitleElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  SendTitleChangeEvent(false);

  
  SVGTitleElementBase::UnbindFromTree(aDeep, aNullParent);
}

void
SVGTitleElement::DoneAddingChildren(bool aHaveNotified)
{
  if (!aHaveNotified) {
    SendTitleChangeEvent(false);
  }
}

void
SVGTitleElement::SendTitleChangeEvent(bool aBound)
{
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->NotifyPossibleTitleChange(aBound);
  }
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGTitleElement)

} 
} 

