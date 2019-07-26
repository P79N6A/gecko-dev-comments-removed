




#include "mozilla/dom/HTMLTemplateElement.h"
#include "mozilla/dom/HTMLTemplateElementBinding.h"

#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

using namespace mozilla::dom;

nsGenericHTMLElement*
NS_NewHTMLTemplateElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                          FromParser aFromParser)
{
  HTMLTemplateElement* it = new HTMLTemplateElement(aNodeInfo);
  nsresult rv = it->Init();
  if (NS_FAILED(rv)) {
    delete it;
    return nullptr;
  }

  return it;
}

namespace mozilla {
namespace dom {

HTMLTemplateElement::HTMLTemplateElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

nsresult
HTMLTemplateElement::Init()
{
  nsIDocument* doc = OwnerDoc();
  nsIDocument* contentsOwner = doc;

  
  nsCOMPtr<nsISupports> container = doc->GetContainer();
  if (container) {
    
    contentsOwner = doc->GetTemplateContentsOwner();
    NS_ENSURE_TRUE(contentsOwner, NS_ERROR_UNEXPECTED);
  }

  ErrorResult rv;
  mContent = contentsOwner->CreateDocumentFragment(rv);
  if (rv.Failed()) {
    return rv.ErrorCode();
  }
  mContent->SetHost(this);

  return NS_OK;
}

HTMLTemplateElement::~HTMLTemplateElement()
{
  if (mContent) {
    mContent->SetHost(nullptr);
  }
}

NS_IMPL_ADDREF_INHERITED(HTMLTemplateElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTemplateElement, Element)

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(HTMLTemplateElement,
                                     nsGenericHTMLElement,
                                     mContent)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLTemplateElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(HTMLTemplateElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTemplateElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE_WITH_INIT(HTMLTemplateElement)

JSObject*
HTMLTemplateElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return HTMLTemplateElementBinding::Wrap(aCx, aScope, this);
}

} 
} 

