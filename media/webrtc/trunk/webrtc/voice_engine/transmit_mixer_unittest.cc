









#include "voice_engine/transmit_mixer.h"

#include "gtest/gtest.h"
#include "voice_engine/include/voe_external_media.h"

namespace webrtc {
namespace voe {
namespace {

class MediaCallback : public VoEMediaProcess {
 public:
  virtual void Process(const int channel, const ProcessingTypes type,
                       int16_t audio[], const int samples_per_channel,
                       const int sample_rate_hz, const bool is_stereo) {
  }
};



TEST(TransmitMixerTest, RegisterExternalMediaCallback) {
  TransmitMixer* tm = NULL;
  ASSERT_EQ(0, TransmitMixer::Create(tm, 0));
  ASSERT_TRUE(tm != NULL);
  MediaCallback callback;
  EXPECT_EQ(-1, tm->RegisterExternalMediaProcessing(NULL,
                                                    kRecordingPreprocessing));
  EXPECT_EQ(-1, tm->RegisterExternalMediaProcessing(&callback,
                                                    kPlaybackPerChannel));
  EXPECT_EQ(-1, tm->RegisterExternalMediaProcessing(&callback,
                                                    kPlaybackAllChannelsMixed));
  EXPECT_EQ(-1, tm->RegisterExternalMediaProcessing(&callback,
                                                    kRecordingPerChannel));
  EXPECT_EQ(0, tm->RegisterExternalMediaProcessing(&callback,
                                                   kRecordingAllChannelsMixed));
  EXPECT_EQ(0, tm->RegisterExternalMediaProcessing(&callback,
                                                   kRecordingPreprocessing));
  EXPECT_EQ(-1, tm->DeRegisterExternalMediaProcessing(kPlaybackPerChannel));
  EXPECT_EQ(-1, tm->DeRegisterExternalMediaProcessing(
                    kPlaybackAllChannelsMixed));
  EXPECT_EQ(-1, tm->DeRegisterExternalMediaProcessing(kRecordingPerChannel));
  EXPECT_EQ(0, tm->DeRegisterExternalMediaProcessing(
                   kRecordingAllChannelsMixed));
  EXPECT_EQ(0, tm->DeRegisterExternalMediaProcessing(kRecordingPreprocessing));
  TransmitMixer::Destroy(tm);
}

}  
}  
}  
