




































#include "nsIDOMHTMLSourceElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsHTMLMediaElement.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

class nsHTMLSourceElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLSourceElement
{
public:
  nsHTMLSourceElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLSourceElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLSOURCEELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  
  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Source)


nsHTMLSourceElement::nsHTMLSourceElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLSourceElement::~nsHTMLSourceElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLSourceElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLSourceElement, nsGenericElement)



NS_INTERFACE_TABLE_HEAD(nsHTMLSourceElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLSourceElement, nsIDOMHTMLSourceElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLSourceElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLSourceElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLSourceElement)


NS_IMPL_URI_ATTR(nsHTMLSourceElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLSourceElement, Type, type)


PRBool
nsHTMLSourceElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::src) {
      static const char* kWhitespace = " \n\r\t\b";
      aResult.SetTo(nsContentUtils::TrimCharsInSet(kWhitespace, aValue));
      return PR_TRUE;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult
nsHTMLSourceElement::BindToTree(nsIDocument *aDocument,
                                nsIContent *aParent,
                                nsIContent *aBindingParent,
                                PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aParent || !aParent->IsNodeOfType(nsINode::eMEDIA))
    return NS_OK;

  nsHTMLMediaElement* media = static_cast<nsHTMLMediaElement*>(aParent);
  media->NotifyAddedSource();

  return NS_OK;
}

