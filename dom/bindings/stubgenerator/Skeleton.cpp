





#include "Skeleton.h"
#include "mozilla/dom/SkeletonBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Skeleton)
NS_IMPL_CYCLE_COLLECTING_ADDREF(Skeleton)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Skeleton)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Skeleton)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

Skeleton::Skeleton()
{
  SetIsDOMBinding();
}

Skeleton::~Skeleton()
{
}

JSObject*
Skeleton::WrapObject(JSContext* aCx, JSObject* aScope,
                         bool* aTriedToWrap)
{
  return SkeletonBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

