






#include "MP3Decoder.h"
#include "MediaDecoderStateMachine.h"
#include "MediaFormatReader.h"
#include "MP3Demuxer.h"
#include "mozilla/Preferences.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#endif

namespace mozilla {

MediaDecoder*
MP3Decoder::Clone() {
  if (!IsEnabled()) {
    return nullptr;
  }
  return new MP3Decoder();
}

MediaDecoderStateMachine*
MP3Decoder::CreateStateMachine() {
  nsRefPtr<MediaDecoderReader> reader =
      new MediaFormatReader(this, new mp3::MP3Demuxer(GetResource()));
  return new MediaDecoderStateMachine(this, reader);
}

bool
MP3Decoder::IsEnabled() {
#ifdef MOZ_WIDGET_ANDROID
  
  return AndroidBridge::Bridge()->GetAPIVersion() >= 16;
#else
  return Preferences::GetBool("media.mp3.enabled");
#endif
}

} 
