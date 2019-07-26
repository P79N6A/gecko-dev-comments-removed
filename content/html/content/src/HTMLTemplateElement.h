




#ifndef mozilla_dom_HTMLTemplateElement_h
#define mozilla_dom_HTMLTemplateElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/DocumentFragment.h"

namespace mozilla {
namespace dom {

class HTMLTemplateElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLTemplateElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual ~HTMLTemplateElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTemplateElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  DocumentFragment* Content()
  {
    return mContent;
  }

protected:
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  nsRefPtr<DocumentFragment> mContent;
};

} 
} 

#endif 

