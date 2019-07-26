









#ifndef SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_H_
#define SRC_VOICE_ENGINE_MAIN_TEST_AUTO_TEST_STANDARD_TEST_BASE_H_

#include <assert.h>

#include "common_types.h"
#include "engine_configurations.h"
#include "voe_audio_processing.h"
#include "voe_base.h"
#include "voe_call_report.h"
#include "voe_codec.h"
#include "voe_dtmf.h"
#include "voe_encryption.h"
#include "voe_errors.h"
#include "voe_external_media.h"
#include "voe_file.h"
#include "voe_hardware.h"
#include "voe_neteq_stats.h"
#include "voe_network.h"
#include "voe_rtp_rtcp.h"
#include "voe_test_defines.h"
#include "voe_video_sync.h"
#include "voe_volume_control.h"


#undef TEST
#undef ASSERT_TRUE
#undef ASSERT_FALSE
#include "gtest/gtest.h"
#include "gmock/gmock.h"












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
};

#endif  
