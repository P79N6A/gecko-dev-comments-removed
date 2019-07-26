





#include "WaveTable.h"
#include "AudioContext.h"
#include "mozilla/dom/WaveTableBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(WaveTable, mContext)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WaveTable, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WaveTable, Release)

WaveTable::WaveTable(AudioContext* aContext,
                     const float* aRealData,
                     uint32_t aRealDataLength,
                     const float* aImagData,
                     uint32_t aImagDataLength)
  : mContext(aContext)
{
  MOZ_ASSERT(aContext);
  SetIsDOMBinding();
}

JSObject*
WaveTable::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return WaveTableBinding::Wrap(aCx, aScope, this);
}

}
}

