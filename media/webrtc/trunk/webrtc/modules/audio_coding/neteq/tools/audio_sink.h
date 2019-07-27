









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_SINK_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_AUDIO_SINK_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {



class AudioSink {
 public:
  AudioSink() {}
  virtual ~AudioSink() {}

  
  
  virtual bool WriteArray(const int16_t* audio, size_t num_samples) = 0;

  
  
  bool WriteAudioFrame(const AudioFrame& audio_frame) {
    return WriteArray(
        audio_frame.data_,
        audio_frame.samples_per_channel_ * audio_frame.num_channels_);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioSink);
};


class AudioSinkFork : public AudioSink {
 public:
  AudioSinkFork(AudioSink* left, AudioSink* right)
      : left_sink_(left), right_sink_(right) {}

  virtual bool WriteArray(const int16_t* audio, size_t num_samples) OVERRIDE {
    return left_sink_->WriteArray(audio, num_samples) &&
           right_sink_->WriteArray(audio, num_samples);
  }

 private:
  AudioSink* left_sink_;
  AudioSink* right_sink_;

  DISALLOW_COPY_AND_ASSIGN(AudioSinkFork);
};
}  
}  
#endif  
