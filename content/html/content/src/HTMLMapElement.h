




#ifndef mozilla_dom_HTMLMapElement_h
#define mozilla_dom_HTMLMapElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsAutoPtr.h"
#include "nsGkAtoms.h"

class nsContentList;

namespace mozilla {
namespace dom {

class HTMLMapElement MOZ_FINAL : public nsGenericHTMLElement,
                                 public nsIDOMHTMLMapElement
{
public:
  HTMLMapElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLMAPELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLMapElement,
                                                     nsGenericHTMLElement)

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  
  void SetName(const nsAString& aName, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aError);
  }
  nsIHTMLCollection* Areas();

  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

protected:
  nsRefPtr<nsContentList> mAreas;
};

} 
} 

#endif 
