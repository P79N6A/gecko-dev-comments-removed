





#include "MessageChannel.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/MessageChannelBinding.h"
#include "mozilla/dom/MessagePort.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MessageChannel, mWindow, mPort1, mPort2)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MessageChannel)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MessageChannel)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MessageChannel)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

namespace {
  bool gPrefInitialized = false;
  bool gPrefEnabled = false;

}

 bool
MessageChannel::Enabled(JSContext* aCx, JSObject* aObj)
{
  if (!gPrefInitialized) {
    Preferences::AddBoolVarCache(&gPrefEnabled, "dom.messageChannel.enabled");
    gPrefInitialized = true;
  }

  
  if (gPrefEnabled) {
    return true;
  }

  
  if (nsContentUtils::ThreadsafeIsCallerChrome()) {
    return true;
  }

  nsCOMPtr<nsIPrincipal> principal = nsContentUtils::SubjectPrincipal();
  MOZ_ASSERT(principal);

  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(principal->GetURI(getter_AddRefs(uri))) || !uri) {
    return false;
  }

  bool isResource = false;
  if (NS_FAILED(uri->SchemeIs("resource", &isResource))) {
    return false;
  }

  return isResource;
}

MessageChannel::MessageChannel(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  MOZ_COUNT_CTOR(MessageChannel);

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
MessageChannel::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MessageChannelBinding::Wrap(aCx, this, aGivenProto);
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
