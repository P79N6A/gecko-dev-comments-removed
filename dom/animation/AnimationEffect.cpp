




#include "mozilla/dom/AnimationEffect.h"
#include "mozilla/dom/AnimationEffectBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AnimationEffect, mAnimation)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AnimationEffect, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AnimationEffect, Release)

JSObject*
AnimationEffect::WrapObject(JSContext* aCx)
{
  return AnimationEffectBinding::Wrap(aCx, this);
}

} 
} 
