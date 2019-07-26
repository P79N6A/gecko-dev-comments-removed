









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_H_

#include <assert.h>

#include "webrtc/common.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_call_report.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_dtmf.h"
#include "webrtc/voice_engine/include/voe_encryption.h"
#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/voice_engine/include/voe_external_media.h"
#include "webrtc/voice_engine/include/voe_file.h"
#include "webrtc/voice_engine/include/voe_hardware.h"
#include "webrtc/voice_engine/include/voe_neteq_stats.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/include/voe_rtp_rtcp.h"
#include "webrtc/voice_engine/include/voe_video_sync.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"
#include "webrtc/voice_engine/test/auto_test/voe_test_common.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"












class BeforeInitializationFixture : public testing::Test {
 public:
  BeforeInitializationFixture();
  virtual ~BeforeInitializationFixture();

 protected:
  
  void Sleep(long milliseconds);

  webrtc::VoiceEngine*        voice_engine_;
  webrtc::VoEBase*            voe_base_;
  webrtc::VoECodec*           voe_codec_;
  webrtc::VoEVolumeControl*   voe_volume_control_;
  webrtc::VoEDtmf*            voe_dtmf_;
  webrtc::VoERTP_RTCP*        voe_rtp_rtcp_;
  webrtc::VoEAudioProcessing* voe_apm_;
  webrtc::VoENetwork*         voe_network_;
  webrtc::VoEFile*            voe_file_;
  webrtc::VoEVideoSync*       voe_vsync_;
  webrtc::VoEEncryption*      voe_encrypt_;
  webrtc::VoEHardware*        voe_hardware_;
  webrtc::VoEExternalMedia*   voe_xmedia_;
  webrtc::VoECallReport*      voe_call_report_;
  webrtc::VoENetEqStats*      voe_neteq_stats_;
  webrtc::Config              config_;
};

#endif  
