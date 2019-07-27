





#ifndef mozilla_dom_AVInputPort_h
#define mozilla_dom_AVInputPort_h

#include "mozilla/dom/InputPort.h"

namespace mozilla {
namespace dom {

class AVInputPort final : public InputPort
{
public:
  static already_AddRefed<AVInputPort> Create(nsPIDOMWindow* aWindow,
                                              nsIInputPortListener* aListener,
                                              nsIInputPortData* aData,
                                              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  explicit AVInputPort(nsPIDOMWindow* aWindow);

  ~AVInputPort();
};

} 
} 

#endif 
