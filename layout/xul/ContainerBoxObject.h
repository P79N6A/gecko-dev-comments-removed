





#ifndef mozilla_dom_ContainerBoxObject_h
#define mozilla_dom_ContainerBoxObject_h

#include "mozilla/dom/BoxObject.h"

namespace mozilla {
namespace dom {

class ContainerBoxObject MOZ_FINAL : public BoxObject
{
public:
  ContainerBoxObject();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  already_AddRefed<nsIDocShell> GetDocShell();

private:
  ~ContainerBoxObject();
};

} 
} 

#endif 
