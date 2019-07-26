









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CANCELLATION_IMPL_WRAPPER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CANCELLATION_IMPL_WRAPPER_H_

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

namespace webrtc {

class AudioProcessingImpl;
class AudioBuffer;

class EchoCancellationImplWrapper : public virtual EchoCancellation,
                                    public virtual ProcessingComponent {
 public:
  static EchoCancellationImplWrapper* Create(
      const AudioProcessingImpl* audioproc);
  virtual ~EchoCancellationImplWrapper() {}

  virtual int ProcessRenderAudio(const AudioBuffer* audio) = 0;
  virtual int ProcessCaptureAudio(AudioBuffer* audio) = 0;
};

}  

#endif  
