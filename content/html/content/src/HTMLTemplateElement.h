




#ifndef mozilla_dom_HTMLTemplateElement_h
#define mozilla_dom_HTMLTemplateElement_h

#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/DocumentFragment.h"

namespace mozilla {
namespace dom {

class HTMLTemplateElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLElement
{
public:
  HTMLTemplateElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLTemplateElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTemplateElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  nsresult Init();

  DocumentFragment* Content()
  {
    return mContent;
  }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  nsRefPtr<DocumentFragment> mContent;
};

} 
} 

#endif 

