





#include "mozilla/dom/HDMIInputPort.h"
#include "mozilla/dom/HDMIInputPortBinding.h"

namespace mozilla {
namespace dom {

HDMIInputPort::HDMIInputPort(nsPIDOMWindow* aWindow)
  : InputPort(aWindow)
{
}

HDMIInputPort::~HDMIInputPort()
{
}

 already_AddRefed<HDMIInputPort>
HDMIInputPort::Create(nsPIDOMWindow* aWindow,
                      nsIInputPortListener* aListener,
                      nsIInputPortData* aData,
                      ErrorResult& aRv)
{
  nsRefPtr<HDMIInputPort> inputport = new HDMIInputPort(aWindow);
  inputport->Init(aData, aListener, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  return inputport.forget();
}


JSObject*
HDMIInputPort::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HDMIInputPortBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
