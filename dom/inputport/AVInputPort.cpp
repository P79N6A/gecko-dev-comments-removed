





#include "mozilla/dom/AVInputPort.h"
#include "mozilla/dom/AVInputPortBinding.h"

namespace mozilla {
namespace dom {

AVInputPort::AVInputPort(nsPIDOMWindow* aWindow)
  : InputPort(aWindow)
{
}

AVInputPort::~AVInputPort()
{
}

 already_AddRefed<AVInputPort>
AVInputPort::Create(nsPIDOMWindow* aWindow,
                    nsIInputPortListener* aListener,
                    nsIInputPortData* aData,
                    ErrorResult& aRv)
{
  nsRefPtr<AVInputPort> inputport = new AVInputPort(aWindow);
  inputport->Init(aData, aListener, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  return inputport.forget();
}

JSObject*
AVInputPort::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AVInputPortBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
