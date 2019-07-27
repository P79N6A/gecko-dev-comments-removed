





#include "SourceBufferContentManager.h"
#include "TrackBuffer.h"
#include "TrackBuffersManager.h"

namespace mozilla {

already_AddRefed<SourceBufferContentManager>
SourceBufferContentManager::CreateManager(dom::SourceBuffer* aParent,
                                          MediaSourceDecoder* aParentDecoder,
                                          const nsACString &aType)
{
  nsRefPtr<SourceBufferContentManager> manager;
  bool useFormatReader =
    Preferences::GetBool("media.mediasource.format-reader", false);
  if (useFormatReader) {
    manager = new TrackBuffersManager(aParent, aParentDecoder, aType);
  } else {
    manager = new TrackBuffer(aParentDecoder, aType);
  }
  return  manager.forget();
}

}
