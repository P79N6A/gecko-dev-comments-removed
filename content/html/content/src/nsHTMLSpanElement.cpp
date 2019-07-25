



































#include "nsIDOMHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

class nsHTMLSpanElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLElement
{
public:
  nsHTMLSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLSpanElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Span)


nsHTMLSpanElement::nsHTMLSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLSpanElement::~nsHTMLSpanElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLSpanElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLSpanElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLSpanElement, nsHTMLSpanElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLSpanElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(nsHTMLSpanElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLSpanElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLSpanElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLSpanElement)




class nsHTMLUnknownElement : public nsHTMLSpanElement
{
public:
  nsHTMLUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};

DOMCI_NODE_DATA(HTMLUnknownElement, nsHTMLUnknownElement)

NS_INTERFACE_MAP_BEGIN(nsHTMLUnknownElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLUnknownElement)
NS_INTERFACE_MAP_END_INHERITING(nsHTMLSpanElement)

nsHTMLUnknownElement::nsHTMLUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsHTMLSpanElement(aNodeInfo)
{
}


NS_IMPL_NS_NEW_HTML_ELEMENT(Unknown)


NS_IMPL_ELEMENT_CLONE(nsHTMLUnknownElement)
