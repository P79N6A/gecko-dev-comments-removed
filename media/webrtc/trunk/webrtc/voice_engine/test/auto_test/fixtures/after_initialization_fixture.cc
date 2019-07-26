









#include "webrtc/voice_engine/test/auto_test/fixtures/after_initialization_fixture.h"

class TestErrorObserver : public webrtc::VoiceEngineObserver {
 public:
  TestErrorObserver() {}
  virtual ~TestErrorObserver() {}
  void CallbackOnError(int channel, int error_code) {
    ADD_FAILURE() << "Unexpected error on channel " << channel <<
        ": error code " << error_code;
  }
};

AfterInitializationFixture::AfterInitializationFixture()
    : error_observer_(new TestErrorObserver()) {
  EXPECT_EQ(0, voe_base_->Init());

#if defined(WEBRTC_ANDROID)
  EXPECT_EQ(0, voe_hardware_->SetLoudspeakerStatus(false));
#endif

  EXPECT_EQ(0, voe_base_->RegisterVoiceEngineObserver(*error_observer_));
}

AfterInitializationFixture::~AfterInitializationFixture() {
  EXPECT_EQ(0, voe_base_->DeRegisterVoiceEngineObserver());
}
