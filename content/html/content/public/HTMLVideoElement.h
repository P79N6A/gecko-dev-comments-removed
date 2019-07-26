





#ifndef mozilla_dom_HTMLVideoElement_h
#define mozilla_dom_HTMLVideoElement_h

#include "nsIDOMHTMLVideoElement.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

class HTMLVideoElement : public HTMLMediaElement,
                         public nsIDOMHTMLVideoElement
{
public:
  HTMLVideoElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLVideoElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLVideoElement, video)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  using HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(HTMLMediaElement::)

  
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

} 
} 

#endif 
