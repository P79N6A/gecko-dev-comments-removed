





#include "MessagePortList.h"
#include "mozilla/dom/MessagePortListBinding.h"
#include "mozilla/dom/MessagePort.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MessagePortList, mOwner, mPorts)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MessagePortList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MessagePortList)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MessagePortList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
MessagePortList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MessagePortListBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
