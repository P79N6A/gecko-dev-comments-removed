




#include "mozilla/dom/DOMStringList.h"
#include "mozilla/dom/DOMStringListBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(DOMStringList)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMStringList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMStringList)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMStringList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

DOMStringList::~DOMStringList()
{
}

JSObject*
DOMStringList::WrapObject(JSContext* aCx)
{
  return DOMStringListBinding::Wrap(aCx, this);
}

} 
} 
