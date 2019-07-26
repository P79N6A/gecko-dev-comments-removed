



#ifndef mozilla_dom_HTMLUnknownElement_h
#define mozilla_dom_HTMLUnknownElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLUnknownElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLUnknownElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    if (NodeInfo()->Equals(nsGkAtoms::bdi)) {
      SetHasDirAuto();
    }
  }

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
