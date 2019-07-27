





#include "SourceBufferContentManager.h"
#include "TrackBuffer.h"
#include "TrackBuffersManager.h"

namespace mozilla {

#if defined(MOZ_GONK_MEDIACODEC) || defined(XP_WIN) || defined(MOZ_APPLEMEDIA) || defined(MOZ_FFMPEG)
#define MP4_READER_DORMANT_HEURISTIC
#else
#undef MP4_READER_DORMANT_HEURISTIC
#endif

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

  
#if defined(MP4_READER_DORMANT_HEURISTIC)
  if (aType.LowerCaseEqualsLiteral("video/mp4") ||
      aType.LowerCaseEqualsLiteral("audio/mp4") ||
      useFormatReader)
  {
    aParentDecoder->NotifyDormantSupported(Preferences::GetBool("media.decoder.heuristic.dormant.enabled", false));
  }
#endif


  return  manager.forget();
}

}
