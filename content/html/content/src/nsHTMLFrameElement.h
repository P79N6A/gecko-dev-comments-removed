




































#ifndef nsHTMLFrameElement_h__
#define nsHTMLFrameElement_h__

#include "nsIDOMHTMLFrameElement.h"
#include "nsGenericHTMLElement.h"

class nsHTMLFrameElement : public nsGenericHTMLFrameElement,
                           public nsIDOMHTMLFrameElement
{
public:
  nsHTMLFrameElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFrameElement::)

  
  NS_DECL_NSIDOMHTMLFRAMEELEMENT

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

#endif
