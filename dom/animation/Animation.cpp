




#include "mozilla/dom/Animation.h"
#include "mozilla/dom/AnimationBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Animation, mDocument)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(Animation, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(Animation, Release)

JSObject*
Animation::WrapObject(JSContext* aCx)
{
  return AnimationBinding::Wrap(aCx, this);
}

void
Animation::SetParentTime(Nullable<TimeDuration> aParentTime)
{
  mParentTime = aParentTime;
}

} 
} 
