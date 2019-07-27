











#include <string>
#include <list>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq/mock/mock_external_decoder_pcm16b.h"
#include "webrtc/modules/audio_coding/neteq/tools/input_audio_file.h"
#include "webrtc/modules/audio_coding/neteq/tools/rtp_generator.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {

using ::testing::_;
using ::testing::Return;






class NetEqExternalDecoderTest : public ::testing::Test {
 protected:
  static const int kTimeStepMs = 10;
  static const int kMaxBlockSize = 480;  
  static const uint8_t kPayloadType = 95;
  static const int kSampleRateHz = 32000;

  NetEqExternalDecoderTest()
      : sample_rate_hz_(kSampleRateHz),
        samples_per_ms_(sample_rate_hz_ / 1000),
        frame_size_ms_(10),
        frame_size_samples_(frame_size_ms_ * samples_per_ms_),
        output_size_samples_(frame_size_ms_ * samples_per_ms_),
        external_decoder_(new MockExternalPcm16B),
        rtp_generator_(new test::RtpGenerator(samples_per_ms_)),
        payload_size_bytes_(0),
        last_send_time_(0),
        last_arrival_time_(0) {
    config_.sample_rate_hz = sample_rate_hz_;
    neteq_external_ = NetEq::Create(config_);
    neteq_ = NetEq::Create(config_);
    input_ = new int16_t[frame_size_samples_];
    encoded_ = new uint8_t[2 * frame_size_samples_];
  }

  ~NetEqExternalDecoderTest() {
    delete neteq_external_;
    delete neteq_;
    
    EXPECT_CALL(*external_decoder_, Die()).Times(1);
    delete [] input_;
    delete [] encoded_;
  }

  virtual void SetUp() {
    const std::string file_name =
        webrtc::test::ResourcePath("audio_coding/testfile32kHz", "pcm");
    input_file_.reset(new test::InputAudioFile(file_name));
    assert(sample_rate_hz_ == 32000);
    NetEqDecoder decoder = kDecoderPCM16Bswb32kHz;
    EXPECT_CALL(*external_decoder_, Init());
    
    EXPECT_CALL(*external_decoder_, Die()).Times(0);
    ASSERT_EQ(NetEq::kOK,
              neteq_external_->RegisterExternalDecoder(
                  external_decoder_.get(), decoder, kPayloadType));
    ASSERT_EQ(NetEq::kOK,
              neteq_->RegisterPayloadType(decoder, kPayloadType));
  }

  virtual void TearDown() {}

  int GetNewPackets() {
    if (!input_file_->Read(frame_size_samples_, input_)) {
      return -1;
    }
    payload_size_bytes_ = WebRtcPcm16b_Encode(input_, frame_size_samples_,
                                             encoded_);
    if (frame_size_samples_ * 2 != payload_size_bytes_) {
      return -1;
    }
    int next_send_time = rtp_generator_->GetRtpHeader(
        kPayloadType, frame_size_samples_, &rtp_header_);
    return next_send_time;
  }

  virtual void VerifyOutput(size_t num_samples) const {
    for (size_t i = 0; i < num_samples; ++i) {
      ASSERT_EQ(output_[i], output_external_[i]) <<
          "Diff in sample " << i << ".";
    }
  }

  virtual int GetArrivalTime(int send_time) {
    int arrival_time = last_arrival_time_ + (send_time - last_send_time_);
    last_send_time_ = send_time;
    last_arrival_time_ = arrival_time;
    return arrival_time;
  }

  virtual bool Lost() { return false; }

  virtual void InsertPackets(int next_arrival_time) {
    
    ASSERT_EQ(
        NetEq::kOK,
        neteq_->InsertPacket(
            rtp_header_, encoded_, payload_size_bytes_, next_arrival_time));
    
    EXPECT_CALL(*external_decoder_,
                IncomingPacket(_,
                               payload_size_bytes_,
                               rtp_header_.header.sequenceNumber,
                               rtp_header_.header.timestamp,
                               next_arrival_time));
    ASSERT_EQ(
        NetEq::kOK,
        neteq_external_->InsertPacket(
            rtp_header_, encoded_, payload_size_bytes_, next_arrival_time));
  }

  virtual void GetOutputAudio() {
    NetEqOutputType output_type;
    
    int samples_per_channel;
    int num_channels;
    EXPECT_EQ(NetEq::kOK,
              neteq_->GetAudio(kMaxBlockSize,
                               output_,
                               &samples_per_channel,
                               &num_channels,
                               &output_type));
    EXPECT_EQ(1, num_channels);
    EXPECT_EQ(output_size_samples_, samples_per_channel);
    
    ASSERT_EQ(NetEq::kOK,
              neteq_external_->GetAudio(kMaxBlockSize,
                                        output_external_,
                                        &samples_per_channel,
                                        &num_channels,
                                        &output_type));
    EXPECT_EQ(1, num_channels);
    EXPECT_EQ(output_size_samples_, samples_per_channel);
  }

  virtual int NumExpectedDecodeCalls(int num_loops) const { return num_loops; }

  void RunTest(int num_loops) {
    
    int next_send_time;
    int next_arrival_time;
    do {
      next_send_time = GetNewPackets();
      ASSERT_NE(-1, next_send_time);
      next_arrival_time = GetArrivalTime(next_send_time);
    } while (Lost());  

    EXPECT_CALL(*external_decoder_, Decode(_, payload_size_bytes_, _, _))
        .Times(NumExpectedDecodeCalls(num_loops));

    int time_now = 0;
    for (int k = 0; k < num_loops; ++k) {
      while (time_now >= next_arrival_time) {
        InsertPackets(next_arrival_time);

        
        do {
          next_send_time = GetNewPackets();
          ASSERT_NE(-1, next_send_time);
          next_arrival_time = GetArrivalTime(next_send_time);
        } while (Lost());  
      }

      GetOutputAudio();

      std::ostringstream ss;
      ss << "Lap number " << k << ".";
      SCOPED_TRACE(ss.str());  
      
      ASSERT_NO_FATAL_FAILURE(VerifyOutput(output_size_samples_));

      time_now += kTimeStepMs;
    }
  }

  NetEq::Config config_;
  int sample_rate_hz_;
  int samples_per_ms_;
  const int frame_size_ms_;
  int frame_size_samples_;
  int output_size_samples_;
  NetEq* neteq_external_;
  NetEq* neteq_;
  scoped_ptr<MockExternalPcm16B> external_decoder_;
  scoped_ptr<test::RtpGenerator> rtp_generator_;
  int16_t* input_;
  uint8_t* encoded_;
  int16_t output_[kMaxBlockSize];
  int16_t output_external_[kMaxBlockSize];
  WebRtcRTPHeader rtp_header_;
  int payload_size_bytes_;
  int last_send_time_;
  int last_arrival_time_;
  scoped_ptr<test::InputAudioFile> input_file_;
};

TEST_F(NetEqExternalDecoderTest, RunTest) {
  RunTest(100);  
}

class LargeTimestampJumpTest : public NetEqExternalDecoderTest {
 protected:
  enum TestStates {
    kInitialPhase,
    kNormalPhase,
    kExpandPhase,
    kFadedExpandPhase,
    kRecovered
  };

  LargeTimestampJumpTest()
      : NetEqExternalDecoderTest(), test_state_(kInitialPhase) {
    sample_rate_hz_ = 8000;
    samples_per_ms_ = sample_rate_hz_ / 1000;
    frame_size_samples_ = frame_size_ms_ * samples_per_ms_;
    output_size_samples_ = frame_size_ms_ * samples_per_ms_;
    EXPECT_CALL(*external_decoder_, Die()).Times(1);
    external_decoder_.reset(new MockExternalPcm16B);
  }

  void SetUp() OVERRIDE {
    const std::string file_name =
        webrtc::test::ResourcePath("audio_coding/testfile32kHz", "pcm");
    input_file_.reset(new test::InputAudioFile(file_name));
    assert(sample_rate_hz_ == 8000);
    NetEqDecoder decoder = kDecoderPCM16B;
    EXPECT_CALL(*external_decoder_, Init());
    EXPECT_CALL(*external_decoder_, HasDecodePlc())
        .WillRepeatedly(Return(false));
    
    EXPECT_CALL(*external_decoder_, Die()).Times(0);
    ASSERT_EQ(NetEq::kOK,
              neteq_external_->RegisterExternalDecoder(
                  external_decoder_.get(), decoder, kPayloadType));
    ASSERT_EQ(NetEq::kOK, neteq_->RegisterPayloadType(decoder, kPayloadType));
  }

  void InsertPackets(int next_arrival_time) OVERRIDE {
    
    EXPECT_CALL(*external_decoder_,
                IncomingPacket(_,
                               payload_size_bytes_,
                               rtp_header_.header.sequenceNumber,
                               rtp_header_.header.timestamp,
                               next_arrival_time));
    ASSERT_EQ(
        NetEq::kOK,
        neteq_external_->InsertPacket(
            rtp_header_, encoded_, payload_size_bytes_, next_arrival_time));
  }

  void GetOutputAudio() OVERRIDE {
    NetEqOutputType output_type;
    int samples_per_channel;
    int num_channels;
    
    ASSERT_EQ(NetEq::kOK,
              neteq_external_->GetAudio(kMaxBlockSize,
                                        output_external_,
                                        &samples_per_channel,
                                        &num_channels,
                                        &output_type));
    EXPECT_EQ(1, num_channels);
    EXPECT_EQ(output_size_samples_, samples_per_channel);
    UpdateState(output_type);
  }

  virtual void UpdateState(NetEqOutputType output_type) {
    switch (test_state_) {
      case kInitialPhase: {
        if (output_type == kOutputNormal) {
          test_state_ = kNormalPhase;
        }
        break;
      }
      case kNormalPhase: {
        if (output_type == kOutputPLC) {
          test_state_ = kExpandPhase;
        }
        break;
      }
      case kExpandPhase: {
        if (output_type == kOutputPLCtoCNG) {
          test_state_ = kFadedExpandPhase;
        } else if (output_type == kOutputNormal) {
          test_state_ = kRecovered;
        }
        break;
      }
      case kFadedExpandPhase: {
        if (output_type == kOutputNormal) {
          test_state_ = kRecovered;
        }
        break;
      }
      case kRecovered: {
        break;
      }
    }
  }

  void VerifyOutput(size_t num_samples) const OVERRIDE {
    if (test_state_ == kExpandPhase || test_state_ == kFadedExpandPhase) {
      
      return;
    }
    for (size_t i = 0; i < num_samples; ++i) {
      if (output_external_[i] != 0)
        return;
    }
    EXPECT_TRUE(false)
        << "Expected at least one non-zero sample in each output block.";
  }

  int NumExpectedDecodeCalls(int num_loops) const OVERRIDE {
    
    
    
    
    
    
    
    return num_loops - 2;
  }

  TestStates test_state_;
};

TEST_F(LargeTimestampJumpTest, JumpLongerThanHalfRange) {
  
  
  
  static const uint16_t kStartSeqeunceNumber = 42076;
  static const uint32_t kStartTimestamp = 2880;
  static const uint32_t kJumpFromTimestamp = 7200;
  static const uint32_t kJumpToTimestamp = 2869342376;
  COMPILE_ASSERT(kJumpFromTimestamp < kJumpToTimestamp,
                 timestamp_jump_should_not_result_in_wrap);
  COMPILE_ASSERT(
      static_cast<uint32_t>(kJumpToTimestamp - kJumpFromTimestamp) > 0x7FFFFFFF,
      jump_should_be_larger_than_half_range);
  
  rtp_generator_.reset(new test::TimestampJumpRtpGenerator(samples_per_ms_,
                                                           kStartSeqeunceNumber,
                                                           kStartTimestamp,
                                                           kJumpFromTimestamp,
                                                           kJumpToTimestamp));

  RunTest(130);  
  EXPECT_EQ(kRecovered, test_state_);
}

TEST_F(LargeTimestampJumpTest, JumpLongerThanHalfRangeAndWrap) {
  
  
  static const uint16_t kStartSeqeunceNumber = 42076;
  
  static const uint32_t kStartTimestamp = 3221223116;
  static const uint32_t kJumpFromTimestamp = 3221223216;
  static const uint32_t kJumpToTimestamp = 1073744278;
  COMPILE_ASSERT(kJumpToTimestamp < kJumpFromTimestamp,
                 timestamp_jump_should_result_in_wrap);
  COMPILE_ASSERT(
      static_cast<uint32_t>(kJumpToTimestamp - kJumpFromTimestamp) > 0x7FFFFFFF,
      jump_should_be_larger_than_half_range);
  
  rtp_generator_.reset(new test::TimestampJumpRtpGenerator(samples_per_ms_,
                                                           kStartSeqeunceNumber,
                                                           kStartTimestamp,
                                                           kJumpFromTimestamp,
                                                           kJumpToTimestamp));

  RunTest(130);  
  EXPECT_EQ(kRecovered, test_state_);
}

class ShortTimestampJumpTest : public LargeTimestampJumpTest {
 protected:
  void UpdateState(NetEqOutputType output_type) OVERRIDE {
    switch (test_state_) {
      case kInitialPhase: {
        if (output_type == kOutputNormal) {
          test_state_ = kNormalPhase;
        }
        break;
      }
      case kNormalPhase: {
        if (output_type == kOutputPLC) {
          test_state_ = kExpandPhase;
        }
        break;
      }
      case kExpandPhase: {
        if (output_type == kOutputNormal) {
          test_state_ = kRecovered;
        }
        break;
      }
      case kRecovered: {
        break;
      }
      default: { FAIL(); }
    }
  }

  int NumExpectedDecodeCalls(int num_loops) const OVERRIDE {
    
    return num_loops - 2;
  }
};

TEST_F(ShortTimestampJumpTest, JumpShorterThanHalfRange) {
  
  
  static const uint16_t kStartSeqeunceNumber = 42076;
  
  static const uint32_t kStartTimestamp = 4711;
  static const uint32_t kJumpFromTimestamp = 4811;
  static const uint32_t kJumpToTimestamp = 2147483747;
  COMPILE_ASSERT(kJumpFromTimestamp < kJumpToTimestamp,
                 timestamp_jump_should_not_result_in_wrap);
  COMPILE_ASSERT(
      static_cast<uint32_t>(kJumpToTimestamp - kJumpFromTimestamp) < 0x7FFFFFFF,
      jump_should_be_smaller_than_half_range);
  
  rtp_generator_.reset(new test::TimestampJumpRtpGenerator(samples_per_ms_,
                                                           kStartSeqeunceNumber,
                                                           kStartTimestamp,
                                                           kJumpFromTimestamp,
                                                           kJumpToTimestamp));

  RunTest(130);  
  EXPECT_EQ(kRecovered, test_state_);
}

TEST_F(ShortTimestampJumpTest, JumpShorterThanHalfRangeAndWrap) {
  
  
  static const uint16_t kStartSeqeunceNumber = 42076;
  
  static const uint32_t kStartTimestamp = 3221227827;
  static const uint32_t kJumpFromTimestamp = 3221227927;
  static const uint32_t kJumpToTimestamp = 1073739567;
  COMPILE_ASSERT(kJumpToTimestamp < kJumpFromTimestamp,
                 timestamp_jump_should_result_in_wrap);
  COMPILE_ASSERT(
      static_cast<uint32_t>(kJumpToTimestamp - kJumpFromTimestamp) < 0x7FFFFFFF,
      jump_should_be_smaller_than_half_range);
  
  rtp_generator_.reset(new test::TimestampJumpRtpGenerator(samples_per_ms_,
                                                           kStartSeqeunceNumber,
                                                           kStartTimestamp,
                                                           kJumpFromTimestamp,
                                                           kJumpToTimestamp));

  RunTest(130);  
  EXPECT_EQ(kRecovered, test_state_);
}

}  
