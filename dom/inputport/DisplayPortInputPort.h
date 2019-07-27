





#ifndef mozilla_dom_DisplayPortInputPort_h
#define mozilla_dom_DisplayPortInputPort_h

#include "mozilla/dom/InputPort.h"

namespace mozilla {
namespace dom {

class DisplayPortInputPort final : public InputPort
{
public:
  static already_AddRefed<DisplayPortInputPort> Create(nsPIDOMWindow* aWindow,
                                                       nsIInputPortListener* aListener,
                                                       nsIInputPortData* aData,
                                                       ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  explicit DisplayPortInputPort(nsPIDOMWindow* aWindow);

  ~DisplayPortInputPort();
};

} 
} 

#endif 
