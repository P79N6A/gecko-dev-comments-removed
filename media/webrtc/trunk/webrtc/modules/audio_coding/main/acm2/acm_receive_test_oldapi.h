









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVE_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVE_TEST_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
class AudioCodingModule;
struct CodecInst;

namespace test {
class AudioSink;
class PacketSource;

class AcmReceiveTestOldApi {
 public:
  enum NumOutputChannels {
    kArbitraryChannels = 0,
    kMonoOutput = 1,
    kStereoOutput = 2
  };

  AcmReceiveTestOldApi(PacketSource* packet_source,
                       AudioSink* audio_sink,
                       int output_freq_hz,
                       NumOutputChannels exptected_output_channels);
  virtual ~AcmReceiveTestOldApi() {}

  
  void RegisterDefaultCodecs();

  
  
  void RegisterNetEqTestCodecs();

  
  void Run();

 protected:
  
  virtual void AfterGetAudio() {}

  SimulatedClock clock_;
  scoped_ptr<AudioCodingModule> acm_;
  PacketSource* packet_source_;
  AudioSink* audio_sink_;
  int output_freq_hz_;
  NumOutputChannels exptected_output_channels_;

  DISALLOW_COPY_AND_ASSIGN(AcmReceiveTestOldApi);
};




class AcmReceiveTestToggleOutputFreqOldApi : public AcmReceiveTestOldApi {
 public:
  AcmReceiveTestToggleOutputFreqOldApi(
      PacketSource* packet_source,
      AudioSink* audio_sink,
      int output_freq_hz_1,
      int output_freq_hz_2,
      int toggle_period_ms,
      NumOutputChannels exptected_output_channels);

 protected:
  void AfterGetAudio() OVERRIDE;

  const int output_freq_hz_1_;
  const int output_freq_hz_2_;
  const int toggle_period_ms_;
  int64_t last_toggle_time_ms_;
};

}  
}  
#endif
