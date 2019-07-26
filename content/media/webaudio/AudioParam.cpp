





#include "AudioParam.h"
#include "nsContentUtils.h"
#include "nsIDOMWindow.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AudioParamBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(AudioParam, mContext)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(AudioParam, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(AudioParam, Release)

AudioParam::AudioParam(AudioContext* aContext,
                       float aDefaultValue,
                       float aMinValue,
                       float aMaxValue)
  : AudioParamTimeline(aDefaultValue, aMinValue, aMaxValue)
  , mContext(aContext)
{
  SetIsDOMBinding();
}

AudioParam::~AudioParam()
{
}

JSObject*
AudioParam::WrapObject(JSContext* aCx, JSObject* aScope,
                       bool* aTriedToWrap)
{
  return AudioParamBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

}
}

