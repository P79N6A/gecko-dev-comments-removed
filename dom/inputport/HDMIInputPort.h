





#ifndef mozilla_dom_HDMIInputPort_h
#define mozilla_dom_HDMIInputPort_h

#include "mozilla/dom/InputPort.h"

namespace mozilla {
namespace dom {

class HDMIInputPort final : public InputPort
{
public:
  static already_AddRefed<HDMIInputPort> Create(nsPIDOMWindow* aWindow,
                                                nsIInputPortListener* aListener,
                                                nsIInputPortData* aData,
                                                ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  explicit HDMIInputPort(nsPIDOMWindow* aWindow);

  ~HDMIInputPort();
};

} 
} 

#endif 
