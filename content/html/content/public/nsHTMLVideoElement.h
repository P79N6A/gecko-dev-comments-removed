




#if !defined(nsHTMLVideoElement_h__)
#define nsHTMLVideoElement_h__

#include "nsIDOMHTMLVideoElement.h"
#include "mozilla/dom/HTMLMediaElement.h"

class nsHTMLVideoElement : public mozilla::dom::HTMLMediaElement,
                           public nsIDOMHTMLVideoElement
{
public:
  nsHTMLVideoElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLVideoElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLVideoElement, video)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  using mozilla::dom::HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(mozilla::dom::HTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLVIDEOELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  
  nsresult GetVideoSize(nsIntSize* size);

  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

#endif
