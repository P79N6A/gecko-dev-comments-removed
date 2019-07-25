




































#if !defined(nsHTMLVideoElement_h__)
#define nsHTMLVideoElement_h__

#include "nsIDOMHTMLVideoElement.h"
#include "nsHTMLMediaElement.h"

class nsHTMLVideoElement : public nsHTMLMediaElement,
                           public nsIDOMHTMLVideoElement
{
public:
  nsHTMLVideoElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLVideoElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(nsHTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLVIDEOELEMENT

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  
  nsIntSize GetVideoSize(nsIntSize defaultSize);

  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsXPCClassInfo* GetClassInfo();
};

#endif
