









#include "webrtc/modules/audio_processing/audio_processing_impl.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_processing/test/test_utils.h"
#include "webrtc/modules/interface/module_common_types.h"

using ::testing::Invoke;
using ::testing::Return;

namespace webrtc {

class MockInitialize : public AudioProcessingImpl {
 public:
  explicit MockInitialize(const Config& config) : AudioProcessingImpl(config) {
  }

  MOCK_METHOD0(InitializeLocked, int());
  int RealInitializeLocked() { return AudioProcessingImpl::InitializeLocked(); }
};

TEST(AudioProcessingImplTest, AudioParameterChangeTriggersInit) {
  Config config;
  MockInitialize mock(config);
  ON_CALL(mock, InitializeLocked())
      .WillByDefault(Invoke(&mock, &MockInitialize::RealInitializeLocked));

  EXPECT_CALL(mock, InitializeLocked()).Times(1);
  mock.Initialize();

  AudioFrame frame;
  
  frame.num_channels_ = 1;
  SetFrameSampleRate(&frame, 16000);
  EXPECT_CALL(mock, InitializeLocked())
      .Times(0);
  EXPECT_EQ(kNoErr, mock.ProcessStream(&frame));
  EXPECT_EQ(kNoErr, mock.AnalyzeReverseStream(&frame));

  
  SetFrameSampleRate(&frame, 32000);
  EXPECT_CALL(mock, InitializeLocked())
      .Times(1);
  EXPECT_EQ(kNoErr, mock.ProcessStream(&frame));

  
  frame.num_channels_ = 2;
  EXPECT_CALL(mock, InitializeLocked())
      .Times(2);
  EXPECT_EQ(kNoErr, mock.ProcessStream(&frame));
  
  frame.num_channels_ = 2;
  EXPECT_EQ(kNoErr, mock.AnalyzeReverseStream(&frame));

  
  
  SetFrameSampleRate(&frame, 16000);
  EXPECT_CALL(mock, InitializeLocked())
      .Times(0);
  EXPECT_EQ(mock.kBadSampleRateError, mock.AnalyzeReverseStream(&frame));
}

}  
