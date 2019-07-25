



































#include "nsIDOMHTMLMetaElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPLDOMEvent.h"
#include "nsContentUtils.h"


class nsHTMLMetaElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLMetaElement
{
public:
  nsHTMLMetaElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLMetaElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLMETAELEMENT

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  void CreateAndDispatchEvent(nsIDocument* aDoc, const nsAString& aEventName);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Meta)


nsHTMLMetaElement::nsHTMLMetaElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLMetaElement::~nsHTMLMetaElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLMetaElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLMetaElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLMetaElement, nsHTMLMetaElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLMetaElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLMetaElement, nsIDOMHTMLMetaElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLMetaElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLMetaElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLMetaElement)


NS_IMPL_STRING_ATTR(nsHTMLMetaElement, Content, content)
NS_IMPL_STRING_ATTR(nsHTMLMetaElement, HttpEquiv, httpEquiv)
NS_IMPL_STRING_ATTR(nsHTMLMetaElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLMetaElement, Scheme, scheme)

nsresult
nsHTMLMetaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aDocument &&
      AttrValueIs(kNameSpaceID_None, nsGkAtoms::name, nsGkAtoms::viewport, eIgnoreCase)) {
    nsAutoString content;
    rv = GetContent(content);
    NS_ENSURE_SUCCESS(rv, rv);
    nsContentUtils::ProcessViewportInfo(aDocument, content);  
  }
  CreateAndDispatchEvent(aDocument, NS_LITERAL_STRING("DOMMetaAdded"));
  return rv;
}

void
nsHTMLMetaElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();
  CreateAndDispatchEvent(oldDoc, NS_LITERAL_STRING("DOMMetaRemoved"));
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

void
nsHTMLMetaElement::CreateAndDispatchEvent(nsIDocument* aDoc,
                                          const nsAString& aEventName)
{
  if (!aDoc)
    return;

  nsRefPtr<nsPLDOMEvent> event = new nsPLDOMEvent(this, aEventName, true,
                                                  true);
  if (event) {
    event->PostDOMEvent();
  }
}
