











#include <stdio.h>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/channel_transport/include/channel_transport.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"

int main(int argc, char** argv) {
  webrtc::VoiceEngine* voe = webrtc::VoiceEngine::Create();
  if (voe == NULL) {
    fprintf(stderr, "Failed to initialize voice engine.\n");
    return 1;
  }

  webrtc::VoEBase* base = webrtc::VoEBase::GetInterface(voe);
  webrtc::VoEVolumeControl* volume_control =
      webrtc::VoEVolumeControl::GetInterface(voe);

  if (base->Init() != 0) {
    fprintf(stderr, "Failed to initialize voice engine base.\n");
    return 1;
  }
  
  if (volume_control->SetMicVolume(0) != 0) {
    fprintf(stderr, "Failed set volume to 0.\n");
    return 1;
  }
  if (volume_control->SetMicVolume(255) != 0) {
    fprintf(stderr, "Failed set volume to 255.\n");
    return 1;
  }

  return 0;
}
