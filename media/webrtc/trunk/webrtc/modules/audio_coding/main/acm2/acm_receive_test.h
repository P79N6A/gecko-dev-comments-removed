









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVE_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVE_TEST_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
class AudioCoding;
struct CodecInst;

namespace test {
class AudioSink;
class PacketSource;

class AcmReceiveTest {
 public:
  enum NumOutputChannels {
    kArbitraryChannels = 0,
    kMonoOutput = 1,
    kStereoOutput = 2
  };

  AcmReceiveTest(
      PacketSource* packet_source,
      AudioSink* audio_sink,
      int output_freq_hz,
      NumOutputChannels exptected_output_channels);
  virtual ~AcmReceiveTest() {}

  
  void RegisterDefaultCodecs();

  
  
  void RegisterNetEqTestCodecs();

  
  void Run();

 private:
  SimulatedClock clock_;
  scoped_ptr<AudioCoding> acm_;
  PacketSource* packet_source_;
  AudioSink* audio_sink_;
  const int output_freq_hz_;
  NumOutputChannels exptected_output_channels_;

  DISALLOW_COPY_AND_ASSIGN(AcmReceiveTest);
};

}  
}  
#endif
