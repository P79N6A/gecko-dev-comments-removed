





#include "MediaUtils.h"

#include "mozilla/MediaManager.h"

namespace mozilla {
namespace media {

template<typename ValueType>
Pledge<ValueType>::Pledge()
  : mDone(false)
  , mResult(NS_OK)
{
  MOZ_ASSERT(MediaManager::IsInMediaThread());
}

}
}
