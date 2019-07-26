



#include "mozilla/dom/GamepadButton.h"
#include "mozilla/dom/GamepadBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(GamepadButton)
NS_IMPL_CYCLE_COLLECTING_RELEASE(GamepadButton)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(GamepadButton)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(GamepadButton, mParent)

 JSObject*
GamepadButton::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return GamepadButtonBinding::Wrap(aCx, aScope, this);
}

} 
} 
