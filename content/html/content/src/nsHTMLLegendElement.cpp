




































#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIEventStateManager.h"
#include "nsIFocusController.h"
#include "nsIDocument.h"
#include "nsPIDOMWindow.h"


class nsHTMLLegendElement : public nsGenericHTMLFormElement,
                            public nsIDOMHTMLLegendElement
{
public:
  nsHTMLLegendElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLLegendElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLLEGENDELEMENT

  
  NS_IMETHOD_(PRInt32) GetType() const { return NS_FORM_LEGEND; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual void SetFocus(nsPresContext* aPresContext);
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Legend)


nsHTMLLegendElement::nsHTMLLegendElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
{
}

nsHTMLLegendElement::~nsHTMLLegendElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLLegendElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLLegendElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLLegendElement,
                                    nsGenericHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLLegendElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLLegendElement)
NS_HTML_CONTENT_INTERFACE_MAP_END





NS_IMPL_ELEMENT_CLONE(nsHTMLLegendElement)


NS_IMETHODIMP
nsHTMLLegendElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
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

PRBool
nsHTMLLegendElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::align && aNamespaceID == kNameSpaceID_None) {
    return aResult.ParseEnumValue(aValue, kAlignTable);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsChangeHint
nsHTMLLegendElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
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
    RegUnRegAccessKey(PR_FALSE);
  }

  nsresult rv = nsGenericHTMLFormElement::SetAttr(aNameSpaceID, aAttribute,
                                                  aPrefix, aValue, aNotify);

  if (accesskey && !aValue.IsEmpty()) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

nsresult
nsHTMLLegendElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                               PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::accesskey &&
      aNameSpaceID == kNameSpaceID_None) {
    RegUnRegAccessKey(PR_FALSE);
  }

  return nsGenericHTMLFormElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsHTMLLegendElement::Reset()
{
  return NS_OK;
}

nsresult
nsHTMLLegendElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

void
nsHTMLLegendElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    RegUnRegAccessKey(PR_FALSE);
  }

  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);
}

void
nsHTMLLegendElement::SetFocus(nsPresContext* aPresContext)
{
  nsIDocument *document = GetCurrentDoc();
  if (!aPresContext || !document) {
    return;
  }

  if (IsFocusable()) {
    nsGenericHTMLFormElement::SetFocus(aPresContext);
  } else {
    
    
    nsCOMPtr<nsPIDOMWindow> ourWindow = document->GetWindow();
    if (ourWindow) {
      nsIFocusController* focusController =
        ourWindow->GetRootFocusController();
      nsCOMPtr<nsIDOMElement> domElement =
        do_QueryInterface(NS_STATIC_CAST(nsIContent *, this));
      if (focusController && domElement) {
        focusController->MoveFocus(PR_TRUE, domElement);
      }
    }
  }
}

NS_IMETHODIMP
nsHTMLLegendElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                       nsIContent* aSubmitElement)
{
  return NS_OK;
}
