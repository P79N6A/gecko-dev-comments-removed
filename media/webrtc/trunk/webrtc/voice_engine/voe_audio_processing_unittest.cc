









#include "voice_engine/include/voe_audio_processing.h"

#include "gtest/gtest.h"
#include "voice_engine/include/voe_base.h"

namespace webrtc {
namespace voe {
namespace {

class VoEAudioProcessingTest : public ::testing::Test {
 protected:
  VoEAudioProcessingTest()
      : voe_(VoiceEngine::Create()),
        base_(VoEBase::GetInterface(voe_)),
        audioproc_(VoEAudioProcessing::GetInterface(voe_)) {
  }

  virtual ~VoEAudioProcessingTest() {
    base_->Terminate();
    audioproc_->Release();
    base_->Release();
    VoiceEngine::Delete(voe_);
  }

  VoiceEngine* voe_;
  VoEBase* base_;
  VoEAudioProcessing* audioproc_;
};

TEST_F(VoEAudioProcessingTest, FailureIfNotInitialized) {
  EXPECT_EQ(-1, audioproc_->EnableDriftCompensation(true));
  EXPECT_EQ(-1, audioproc_->EnableDriftCompensation(false));
  EXPECT_FALSE(audioproc_->DriftCompensationEnabled());
}



TEST_F(VoEAudioProcessingTest, DISABLED_DriftCompensationIsEnabledIfSupported) {
  ASSERT_EQ(0, base_->Init());
  
  bool supported = VoEAudioProcessing::DriftCompensationSupported();
  if (supported) {
    EXPECT_EQ(0, audioproc_->EnableDriftCompensation(true));
    EXPECT_TRUE(audioproc_->DriftCompensationEnabled());
    EXPECT_EQ(0, audioproc_->EnableDriftCompensation(false));
    EXPECT_FALSE(audioproc_->DriftCompensationEnabled());
  } else {
    EXPECT_EQ(-1, audioproc_->EnableDriftCompensation(true));
    EXPECT_FALSE(audioproc_->DriftCompensationEnabled());
    EXPECT_EQ(-1, audioproc_->EnableDriftCompensation(false));
    EXPECT_FALSE(audioproc_->DriftCompensationEnabled());
  }
}

}  
}  
}  
