




































#include "nsHTMLLegendElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIEventStateManager.h"
#include "nsIDocument.h"
#include "nsPIDOMWindow.h"
#include "nsFocusManager.h"
#include "nsIFrame.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Legend)


nsHTMLLegendElement::nsHTMLLegendElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLLegendElement::~nsHTMLLegendElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLLegendElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLLegendElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLLegendElement, nsHTMLLegendElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLLegendElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLLegendElement, nsIDOMHTMLLegendElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLLegendElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLLegendElement)





NS_IMPL_ELEMENT_CLONE(nsHTMLLegendElement)


NS_IMETHODIMP
nsHTMLLegendElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  Element *form = GetFormElement();

  return form ? CallQueryInterface(form, aForm) : NS_OK;
}


NS_IMPL_STRING_ATTR(nsHTMLLegendElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLLegendElement, Align, align)


static const nsAttrValue::EnumTable kAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { "bottom", NS_STYLE_VERTICAL_ALIGN_BOTTOM },
  { "top", NS_STYLE_VERTICAL_ALIGN_TOP },
  { 0 }
};

nsIContent*
nsHTMLLegendElement::GetFieldSet()
{
  nsIContent* parent = GetParent();

  if (parent && parent->IsHTML() && parent->Tag() == nsGkAtoms::fieldset) {
    return parent;
  }

  return nsnull;
}

PRBool
nsHTMLLegendElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::align && aNamespaceID == kNameSpaceID_None) {
    return aResult.ParseEnumValue(aValue, kAlignTable, PR_FALSE);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsChangeHint
nsHTMLLegendElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::align) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  }
  return retval;
}

nsresult
nsHTMLLegendElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  PRBool accesskey = (aAttribute == nsGkAtoms::accesskey &&
                      aNameSpaceID == kNameSpaceID_None);
  if (accesskey) {
    UnregAccessKey();
  }

  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aAttribute,
                                              aPrefix, aValue, aNotify);

  if (accesskey && !aValue.IsEmpty()) {
    SetFlags(NODE_HAS_ACCESSKEY);
    RegAccessKey();
  }

  return rv;
}

nsresult
nsHTMLLegendElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                               PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::accesskey &&
      aNameSpaceID == kNameSpaceID_None) {
    
    UnregAccessKey();
    UnsetFlags(NODE_HAS_ACCESSKEY);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsHTMLLegendElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    RegAccessKey();
  }

  return rv;
}

void
nsHTMLLegendElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    UnregAccessKey();
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

NS_IMETHODIMP
nsHTMLLegendElement::Focus()
{
  nsIFrame* frame = GetPrimaryFrame();
  if (!frame)
    return NS_OK;

  PRInt32 tabIndex;
  if (frame->IsFocusable(&tabIndex, PR_FALSE))
    return nsGenericHTMLElement::Focus();

  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return NS_OK;

  nsCOMPtr<nsIDOMElement> result;
  return fm->MoveFocus(nsnull, this, nsIFocusManager::MOVEFOCUS_FORWARD, 0,
                       getter_AddRefs(result));
}

void
nsHTMLLegendElement::PerformAccesskey(PRBool aKeyCausesActivation,
                                      PRBool aIsTrustedEvent)
{
  
  Focus();
}

