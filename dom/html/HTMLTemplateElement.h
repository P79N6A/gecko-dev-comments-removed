




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
  explicit HTMLTemplateElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTemplateElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  DocumentFragment* Content()
  {
    return mContent;
  }

protected:
  virtual ~HTMLTemplateElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  nsRefPtr<DocumentFragment> mContent;
};

} 
} 

#endif 

