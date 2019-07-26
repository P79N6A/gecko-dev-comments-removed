




#include "MessageChannel.h"
#include "mozilla/dom/MessageChannelBinding.h"
#include "mozilla/dom/MessagePort.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_3(MessageChannel, mWindow, mPort1, mPort2)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MessageChannel)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MessageChannel)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MessageChannel)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MessageChannel::MessageChannel(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  MOZ_COUNT_CTOR(MessageChannel);
  SetIsDOMBinding();

  mPort1 = new MessagePort(mWindow);
  mPort2 = new MessagePort(mWindow);

  mPort1->Entangle(mPort2);
  mPort2->Entangle(mPort1);
}

MessageChannel::~MessageChannel()
{
  MOZ_COUNT_DTOR(MessageChannel);
}

JSObject*
MessageChannel::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MessageChannelBinding::Wrap(aCx, aScope, this);
}

 already_AddRefed<MessageChannel>
MessageChannel::Constructor(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<MessageChannel> channel = new MessageChannel(window);
  return channel.forget();
}

} 
} 
