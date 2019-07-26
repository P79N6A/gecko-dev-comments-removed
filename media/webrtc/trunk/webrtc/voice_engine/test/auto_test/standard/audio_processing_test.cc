









#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"
#include "webrtc/voice_engine/test/auto_test/voe_standard_test.h"

class RxCallback : public webrtc::VoERxVadCallback {
 public:
  RxCallback() :
    vad_decision(-1) {
  }

  virtual void OnRxVad(int, int vadDecision) {
    char msg[128];
    sprintf(msg, "RX VAD detected decision %d \n", vadDecision);
    TEST_LOG("%s", msg);
    vad_decision = vadDecision;
  }

  int vad_decision;
};

class AudioProcessingTest : public AfterStreamingFixture {
 protected:
  
  
  void TryEnablingAgcWithMode(webrtc::AgcModes agc_mode_to_set) {
    EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, agc_mode_to_set));

    bool agc_enabled = false;
    webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

    EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
    EXPECT_TRUE(agc_enabled);
    EXPECT_EQ(agc_mode_to_set, agc_mode);
  }

  void TryEnablingRxAgcWithMode(webrtc::AgcModes agc_mode_to_set) {
    EXPECT_EQ(0, voe_apm_->SetRxAgcStatus(channel_, true, agc_mode_to_set));

    bool rx_agc_enabled = false;
    webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

    EXPECT_EQ(0, voe_apm_->GetRxAgcStatus(channel_, rx_agc_enabled, agc_mode));
    EXPECT_TRUE(rx_agc_enabled);
    EXPECT_EQ(agc_mode_to_set, agc_mode);
  }

  
  
  void TryEnablingEcWithMode(webrtc::EcModes ec_mode_to_set,
                             webrtc::EcModes expected_mode) {
    EXPECT_EQ(0, voe_apm_->SetEcStatus(true, ec_mode_to_set));

    bool ec_enabled = true;
    webrtc::EcModes ec_mode = webrtc::kEcDefault;

    EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));

    EXPECT_EQ(expected_mode, ec_mode);
  }

  
  void TryEnablingAecmWithMode(webrtc::AecmModes aecm_mode_to_set,
                               bool cng_enabled_to_set) {
    EXPECT_EQ(0, voe_apm_->SetAecmMode(aecm_mode_to_set, cng_enabled_to_set));

    bool cng_enabled = false;
    webrtc::AecmModes aecm_mode = webrtc::kAecmEarpiece;

    voe_apm_->GetAecmMode(aecm_mode, cng_enabled);

    EXPECT_EQ(cng_enabled_to_set, cng_enabled);
    EXPECT_EQ(aecm_mode_to_set, aecm_mode);
  }

  void TryEnablingNsWithMode(webrtc::NsModes ns_mode_to_set,
                             webrtc::NsModes expected_ns_mode) {
    EXPECT_EQ(0, voe_apm_->SetNsStatus(true, ns_mode_to_set));

    bool ns_status = true;
    webrtc::NsModes ns_mode = webrtc::kNsDefault;
    EXPECT_EQ(0, voe_apm_->GetNsStatus(ns_status, ns_mode));

    EXPECT_TRUE(ns_status);
    EXPECT_EQ(expected_ns_mode, ns_mode);
  }

  void TryEnablingRxNsWithMode(webrtc::NsModes ns_mode_to_set,
                               webrtc::NsModes expected_ns_mode) {
    EXPECT_EQ(0, voe_apm_->SetRxNsStatus(channel_, true, ns_mode_to_set));

    bool ns_status = true;
    webrtc::NsModes ns_mode = webrtc::kNsDefault;
    EXPECT_EQ(0, voe_apm_->GetRxNsStatus(channel_, ns_status, ns_mode));

    EXPECT_TRUE(ns_status);
    EXPECT_EQ(expected_ns_mode, ns_mode);
  }

  void TryDetectingSilence() {
    
    EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, true));
    EXPECT_EQ(0, voe_volume_control_->SetInputMute(channel_, true));
    EXPECT_EQ(0, voe_file_->StopPlayingFileAsMicrophone(channel_));

    
    Sleep(50);
    for (int i = 0; i < 25; i++) {
      EXPECT_EQ(0, voe_apm_->VoiceActivityIndicator(channel_));
      Sleep(10);
    }
  }

  void TryDetectingSpeechAfterSilence() {
    
    RestartFakeMicrophone();
    EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, false));
    EXPECT_EQ(0, voe_volume_control_->SetInputMute(channel_, false));

    
    for (int i = 0; i < 50; i++) {
      if (voe_apm_->VoiceActivityIndicator(channel_) == 1) {
        return;
      }
      Sleep(10);
    }

    ADD_FAILURE() << "Failed to detect speech within 500 ms.";
  }
};

#if !defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID)

TEST_F(AudioProcessingTest, DISABLED_AgcIsOnByDefault) {
  bool agc_enabled = false;
  webrtc::AgcModes agc_mode = webrtc::kAgcAdaptiveAnalog;

  EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
  EXPECT_TRUE(agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveAnalog, agc_mode);
}

TEST_F(AudioProcessingTest, DISABLED_CanEnableAgcWithAllModes) {
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveDigital);
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveAnalog);
  TryEnablingAgcWithMode(webrtc::kAgcFixedDigital);
}

TEST_F(AudioProcessingTest, EcIsDisabledAndAecIsDefaultEcMode) {
  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;

  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));
  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAec, ec_mode);
}

TEST_F(AudioProcessingTest, EnablingEcAecShouldEnableEcAec) {
  TryEnablingEcWithMode(webrtc::kEcAec, webrtc::kEcAec);
}

TEST_F(AudioProcessingTest, EnablingEcConferenceShouldEnableEcAec) {
  TryEnablingEcWithMode(webrtc::kEcConference, webrtc::kEcAec);
}

TEST_F(AudioProcessingTest, EcModeIsPreservedWhenEcIsTurnedOff) {
  TryEnablingEcWithMode(webrtc::kEcConference, webrtc::kEcAec);

  EXPECT_EQ(0, voe_apm_->SetEcStatus(false));

  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;
  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));

  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAec, ec_mode);
}

TEST_F(AudioProcessingTest, CanEnableAndDisableEcModeSeveralTimesInARow) {
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(0, voe_apm_->SetEcStatus(true));
    EXPECT_EQ(0, voe_apm_->SetEcStatus(false));
  }

  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;
  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));

  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAec, ec_mode);
}


TEST_F(AudioProcessingTest, DISABLED_TestVoiceActivityDetectionWithObserver) {
  RxCallback rx_callback;
  EXPECT_EQ(0, voe_apm_->RegisterRxVadObserver(channel_, rx_callback));

  
  
  TryDetectingSilence();
  Sleep(100);

  EXPECT_EQ(0, rx_callback.vad_decision);

  TryDetectingSpeechAfterSilence();
  Sleep(100);

  EXPECT_EQ(1, rx_callback.vad_decision);

  EXPECT_EQ(0, voe_apm_->DeRegisterRxVadObserver(channel_));
}

#endif   

TEST_F(AudioProcessingTest, EnablingEcAecmShouldEnableEcAecm) {
  
  TryEnablingEcWithMode(webrtc::kEcAecm, webrtc::kEcAecm);
}

TEST_F(AudioProcessingTest, EcAecmModeIsEnabledAndSpeakerphoneByDefault) {
  bool cng_enabled = false;
  webrtc::AecmModes aecm_mode = webrtc::kAecmEarpiece;

  voe_apm_->GetAecmMode(aecm_mode, cng_enabled);

  EXPECT_TRUE(cng_enabled);
  EXPECT_EQ(webrtc::kAecmSpeakerphone, aecm_mode);
}

TEST_F(AudioProcessingTest, CanSetAecmMode) {
  EXPECT_EQ(0, voe_apm_->SetEcStatus(true, webrtc::kEcAecm));

  
  TryEnablingAecmWithMode(webrtc::kAecmEarpiece, true);
  TryEnablingAecmWithMode(webrtc::kAecmEarpiece, false);
  TryEnablingAecmWithMode(webrtc::kAecmLoudEarpiece, true);
  TryEnablingAecmWithMode(webrtc::kAecmLoudSpeakerphone, false);
  TryEnablingAecmWithMode(webrtc::kAecmQuietEarpieceOrHeadset, true);
  TryEnablingAecmWithMode(webrtc::kAecmSpeakerphone, false);
}

TEST_F(AudioProcessingTest, DISABLED_RxAgcShouldBeOffByDefault) {
  bool rx_agc_enabled = true;
  webrtc::AgcModes agc_mode = webrtc::kAgcDefault;

  EXPECT_EQ(0, voe_apm_->GetRxAgcStatus(channel_, rx_agc_enabled, agc_mode));
  EXPECT_FALSE(rx_agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveDigital, agc_mode);
}

TEST_F(AudioProcessingTest, DISABLED_CanTurnOnDigitalRxAcg) {
  TryEnablingRxAgcWithMode(webrtc::kAgcAdaptiveDigital);
  TryEnablingRxAgcWithMode(webrtc::kAgcFixedDigital);
}

TEST_F(AudioProcessingTest, DISABLED_CannotTurnOnAdaptiveAnalogRxAgc) {
  EXPECT_EQ(-1, voe_apm_->SetRxAgcStatus(
      channel_, true, webrtc::kAgcAdaptiveAnalog));
}

TEST_F(AudioProcessingTest, NsIsOffWithModerateSuppressionByDefault) {
  bool ns_status = true;
  webrtc::NsModes ns_mode = webrtc::kNsDefault;
  EXPECT_EQ(0, voe_apm_->GetNsStatus(ns_status, ns_mode));

  EXPECT_FALSE(ns_status);
  EXPECT_EQ(webrtc::kNsModerateSuppression, ns_mode);
}

TEST_F(AudioProcessingTest, CanSetNsMode) {
  
  TryEnablingNsWithMode(webrtc::kNsHighSuppression,
                        webrtc::kNsHighSuppression);
  TryEnablingNsWithMode(webrtc::kNsLowSuppression,
                        webrtc::kNsLowSuppression);
  TryEnablingNsWithMode(webrtc::kNsModerateSuppression,
                        webrtc::kNsModerateSuppression);
  TryEnablingNsWithMode(webrtc::kNsVeryHighSuppression,
                        webrtc::kNsVeryHighSuppression);

  
  TryEnablingNsWithMode(webrtc::kNsConference,
                        webrtc::kNsHighSuppression);
  TryEnablingNsWithMode(webrtc::kNsDefault,
                        webrtc::kNsModerateSuppression);
}

TEST_F(AudioProcessingTest, RxNsIsOffWithModerateSuppressionByDefault) {
  bool ns_status = true;
  webrtc::NsModes ns_mode = webrtc::kNsDefault;
  EXPECT_EQ(0, voe_apm_->GetRxNsStatus(channel_, ns_status, ns_mode));

  EXPECT_FALSE(ns_status);
  EXPECT_EQ(webrtc::kNsModerateSuppression, ns_mode);
}

TEST_F(AudioProcessingTest, CanSetRxNsMode) {
  EXPECT_EQ(0, voe_apm_->SetRxNsStatus(channel_, true));

  
  TryEnablingRxNsWithMode(webrtc::kNsHighSuppression,
                          webrtc::kNsHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsLowSuppression,
                          webrtc::kNsLowSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsModerateSuppression,
                          webrtc::kNsModerateSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsVeryHighSuppression,
                          webrtc::kNsVeryHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsConference,
                          webrtc::kNsHighSuppression);
  TryEnablingRxNsWithMode(webrtc::kNsDefault,
                          webrtc::kNsModerateSuppression);
}

TEST_F(AudioProcessingTest, VadIsDisabledByDefault) {
  bool vad_enabled;
  bool disabled_dtx;
  webrtc::VadModes vad_mode;

  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, disabled_dtx));

  EXPECT_FALSE(vad_enabled);
}

TEST_F(AudioProcessingTest, VoiceActivityIndicatorReturns1WithSpeechOn) {
  
  
  Sleep(500);
  EXPECT_EQ(1, voe_apm_->VoiceActivityIndicator(channel_));
}

TEST_F(AudioProcessingTest, CanSetDelayOffset) {
  voe_apm_->SetDelayOffsetMs(50);
  EXPECT_EQ(50, voe_apm_->DelayOffsetMs());
  voe_apm_->SetDelayOffsetMs(-50);
  EXPECT_EQ(-50, voe_apm_->DelayOffsetMs());
}

TEST_F(AudioProcessingTest, HighPassFilterIsOnByDefault) {
  EXPECT_TRUE(voe_apm_->IsHighPassFilterEnabled());
}

TEST_F(AudioProcessingTest, CanSetHighPassFilter) {
  EXPECT_EQ(0, voe_apm_->EnableHighPassFilter(true));
  EXPECT_TRUE(voe_apm_->IsHighPassFilterEnabled());
  EXPECT_EQ(0, voe_apm_->EnableHighPassFilter(false));
  EXPECT_FALSE(voe_apm_->IsHighPassFilterEnabled());
}

TEST_F(AudioProcessingTest, StereoChannelSwappingIsOffByDefault) {
  EXPECT_FALSE(voe_apm_->IsStereoChannelSwappingEnabled());
}

TEST_F(AudioProcessingTest, CanSetStereoChannelSwapping) {
  voe_apm_->EnableStereoChannelSwapping(true);
  EXPECT_TRUE(voe_apm_->IsStereoChannelSwappingEnabled());
  voe_apm_->EnableStereoChannelSwapping(false);
  EXPECT_FALSE(voe_apm_->IsStereoChannelSwappingEnabled());
}

TEST_F(AudioProcessingTest, CanStartAndStopDebugRecording) {
  std::string output_path = webrtc::test::OutputPath();
  std::string output_file = output_path + "apm_debug.txt";

  EXPECT_EQ(0, voe_apm_->StartDebugRecording(output_file.c_str()));
  Sleep(1000);
  EXPECT_EQ(0, voe_apm_->StopDebugRecording());
}

#if defined(WEBRTC_IOS) || defined(WEBRTC_ANDROID)

TEST_F(AudioProcessingTest, AgcIsOffByDefaultAndDigital) {
  bool agc_enabled = true;
  webrtc::AgcModes agc_mode = webrtc::kAgcAdaptiveAnalog;

  EXPECT_EQ(0, voe_apm_->GetAgcStatus(agc_enabled, agc_mode));
  EXPECT_FALSE(agc_enabled);
  EXPECT_EQ(webrtc::kAgcAdaptiveDigital, agc_mode);
}

TEST_F(AudioProcessingTest, CanEnableAgcInAdaptiveDigitalMode) {
  TryEnablingAgcWithMode(webrtc::kAgcAdaptiveDigital);
}

TEST_F(AudioProcessingTest, AgcIsPossibleExceptInAdaptiveAnalogMode) {
  EXPECT_EQ(-1, voe_apm_->SetAgcStatus(true, webrtc::kAgcAdaptiveAnalog));
  EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, webrtc::kAgcFixedDigital));
  EXPECT_EQ(0, voe_apm_->SetAgcStatus(true, webrtc::kAgcAdaptiveDigital));
}

TEST_F(AudioProcessingTest, EcIsDisabledAndAecmIsDefaultEcMode) {
  bool ec_enabled = true;
  webrtc::EcModes ec_mode = webrtc::kEcDefault;

  EXPECT_EQ(0, voe_apm_->GetEcStatus(ec_enabled, ec_mode));
  EXPECT_FALSE(ec_enabled);
  EXPECT_EQ(webrtc::kEcAecm, ec_mode);
}

TEST_F(AudioProcessingTest, TestVoiceActivityDetection) {
  TryDetectingSilence();
  TryDetectingSpeechAfterSilence();
}

#endif  
