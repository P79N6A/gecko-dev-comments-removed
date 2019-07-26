








#ifndef VOICE_ENGINE_MAIN_TEST_AUTO_TEST_FAKE_MEDIA_PROCESS_H_
#define VOICE_ENGINE_MAIN_TEST_AUTO_TEST_FAKE_MEDIA_PROCESS_H_

#include <cmath>

class FakeMediaProcess : public webrtc::VoEMediaProcess {
 public:
  virtual void Process(const int channel,
                       const webrtc::ProcessingTypes type,
                       WebRtc_Word16 audio_10ms[],
                       const int length,
                       const int sampling_freq_hz,
                       const bool stereo) {
    for (int i = 0; i < length; i++) {
      if (!stereo) {
        audio_10ms[i] = static_cast<WebRtc_Word16>(audio_10ms[i] *
            sin(2.0 * 3.14 * frequency * 400.0 / sampling_freq_hz));
      } else {
        
        audio_10ms[2 * i] = static_cast<WebRtc_Word16> (
            audio_10ms[2 * i] * sin(2.0 * 3.14 *
                frequency * 400.0 / sampling_freq_hz));
        audio_10ms[2 * i + 1] = static_cast<WebRtc_Word16> (
            audio_10ms[2 * i + 1] * sin(2.0 * 3.14 *
                frequency * 400.0 / sampling_freq_hz));
      }
      frequency++;
    }
  }

 private:
  int frequency;
};

#endif  
