




#include "mozilla/dom/AnimationEffectReadOnly.h"
#include "mozilla/dom/AnimationEffectReadOnlyBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationEffectReadOnly, mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(AnimationEffectReadOnly)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AnimationEffectReadOnly)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AnimationEffectReadOnly)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

} 
} 
