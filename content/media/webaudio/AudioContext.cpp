





#include "AudioContext.h"
#include "nsContentUtils.h"
#include "nsIDOMWindow.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AudioContextBinding.h"

namespace mozilla {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(AudioContext, mWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(AudioContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AudioContext)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AudioContext)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

AudioContext::AudioContext(nsIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  SetIsDOMBinding();
}

AudioContext::~AudioContext()
{
}

JSObject*
AudioContext::WrapObject(JSContext* aCx, JSObject* aScope,
                         bool* aTriedToWrap)
{
  return dom::mozAudioContextBinding::Wrap(aCx, aScope, this,
                                           aTriedToWrap);
}

 already_AddRefed<AudioContext>
AudioContext::Constructor(nsISupports* aGlobal, ErrorResult& aRv)
{
  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(aGlobal);
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  AudioContext* object = new AudioContext(window);
  NS_ADDREF(object);
  return object;
}

}

