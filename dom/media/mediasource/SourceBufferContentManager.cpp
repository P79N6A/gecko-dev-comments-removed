





#include "SourceBufferContentManager.h"

namespace mozilla {

already_AddRefed<SourceBufferContentManager>
SourceBufferContentManager::CreateManager(MediaSourceDecoder* aParentDecoder,
                                          const nsACString &aType)
{
  nsRefPtr<SourceBufferContentManager> manager;
  manager = new TrackBuffer(aParentDecoder, aType);
  return  manager.forget();
}

}
