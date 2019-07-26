




#ifndef mozilla_dom_SVGDocument_h
#define mozilla_dom_SVGDocument_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/XMLDocument.h"

class nsSVGElement;

namespace mozilla {
namespace dom {

class SVGDocument : public XMLDocument
{
public:
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  void GetDomain(nsAString& aDomain, ErrorResult& aRv);
  nsSVGElement* GetRootElement(ErrorResult& aRv);

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
