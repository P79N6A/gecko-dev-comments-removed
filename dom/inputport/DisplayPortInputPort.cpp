





#include "mozilla/dom/DisplayPortInputPort.h"
#include "mozilla/dom/DisplayPortInputPortBinding.h"

namespace mozilla {
namespace dom {

DisplayPortInputPort::DisplayPortInputPort(nsPIDOMWindow* aWindow)
  : InputPort(aWindow)
{
}

DisplayPortInputPort::~DisplayPortInputPort()
{
}

 already_AddRefed<DisplayPortInputPort>
DisplayPortInputPort::Create(nsPIDOMWindow* aWindow,
                             nsIInputPortListener* aListener,
                             nsIInputPortData* aData,
                             ErrorResult& aRv)
{
  nsRefPtr<DisplayPortInputPort> inputport = new DisplayPortInputPort(aWindow);
  inputport->Init(aData, aListener, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  return inputport.forget();
}

JSObject*
DisplayPortInputPort::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return DisplayPortInputPortBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
