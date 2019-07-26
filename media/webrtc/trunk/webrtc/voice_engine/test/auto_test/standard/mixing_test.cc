









#include <stdio.h>
#include <string>

#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/after_initialization_fixture.h"

namespace webrtc {
namespace {

const int16_t kLimiterHeadroom = 29204;  
const int16_t kInt16Max = 0x7fff;
const int kSampleRateHz = 16000;
const int kTestDurationMs = 3000;

}  

class MixingTest : public AfterInitializationFixture {
 protected:
  MixingTest()
      : output_filename_(test::OutputPath() + "mixing_test_output.pcm") {
  }
  void SetUp() {
    transport_ = new LoopBackTransport(voe_network_);
  }
  void TearDown() {
    delete transport_;
  }

  
  
  
  
  
  
  
  
  
  
  
  void RunMixingTest(int num_remote_streams,
                     int num_local_streams,
                     int num_remote_streams_using_mono,
                     bool real_audio,
                     int16_t input_value,
                     int16_t max_output_value,
                     int16_t min_output_value) {
    ASSERT_LE(num_remote_streams_using_mono, num_remote_streams);

    if (real_audio) {
      input_filename_ = test::ResourcePath("voice_engine/audio_long16", "pcm");
    } else {
      input_filename_ = test::OutputPath() + "mixing_test_input.pcm";
      GenerateInputFile(input_value);
    }

    std::vector<int> local_streams(num_local_streams);
    for (size_t i = 0; i < local_streams.size(); ++i) {
      local_streams[i] = voe_base_->CreateChannel();
      EXPECT_NE(-1, local_streams[i]);
    }
    StartLocalStreams(local_streams);
    TEST_LOG("Playing %d local streams.\n", num_local_streams);

    std::vector<int> remote_streams(num_remote_streams);
    for (size_t i = 0; i < remote_streams.size(); ++i) {
      remote_streams[i] = voe_base_->CreateChannel();
      EXPECT_NE(-1, remote_streams[i]);
    }
    StartRemoteStreams(remote_streams, num_remote_streams_using_mono);
    TEST_LOG("Playing %d remote streams.\n", num_remote_streams);

    
    SleepMs(1000);

    
    EXPECT_EQ(0, voe_file_->StartRecordingPlayout(-1 ,
        output_filename_.c_str()));
    SleepMs(kTestDurationMs);
    EXPECT_EQ(0, voe_file_->StopRecordingPlayout(-1));

    StopLocalStreams(local_streams);
    StopRemoteStreams(remote_streams);

    if (!real_audio) {
      VerifyMixedOutput(max_output_value, min_output_value);
    }
  }

 private:
  
  
  void GenerateInputFile(int16_t input_value) {
    FILE* input_file = fopen(input_filename_.c_str(), "wb");
    ASSERT_TRUE(input_file != NULL);
    for (int i = 0; i < kSampleRateHz / 1000 * (kTestDurationMs * 2); i++) {
      ASSERT_EQ(1u, fwrite(&input_value, sizeof(input_value), 1, input_file));
    }
    ASSERT_EQ(0, fclose(input_file));
  }

  void VerifyMixedOutput(int16_t max_output_value, int16_t min_output_value) {
    
    FILE* output_file = fopen(output_filename_.c_str(), "rb");
    ASSERT_TRUE(output_file != NULL);
    int16_t output_value = 0;
    int samples_read = 0;
    while (fread(&output_value, sizeof(output_value), 1, output_file) == 1) {
      samples_read++;
      std::ostringstream trace_stream;
      trace_stream << samples_read << " samples read";
      SCOPED_TRACE(trace_stream.str());
      EXPECT_LE(output_value, max_output_value);
      EXPECT_GE(output_value, min_output_value);
    }
    
    
    
    ASSERT_GE((samples_read * 1000.0) / kSampleRateHz, 0.5 * kTestDurationMs);
    
    ASSERT_NE(0, feof(output_file));
    ASSERT_EQ(0, fclose(output_file));
  }

  
  void StartLocalStreams(const std::vector<int>& streams) {
    for (size_t i = 0; i < streams.size(); ++i) {
      EXPECT_EQ(0, voe_base_->StartPlayout(streams[i]));
      EXPECT_EQ(0, voe_file_->StartPlayingFileLocally(streams[i],
          input_filename_.c_str(), true));
    }
  }

  void StopLocalStreams(const std::vector<int>& streams) {
    for (size_t i = 0; i < streams.size(); ++i) {
      EXPECT_EQ(0, voe_base_->StopPlayout(streams[i]));
      EXPECT_EQ(0, voe_base_->DeleteChannel(streams[i]));
    }
  }

  
  void StartRemoteStreams(const std::vector<int>& streams,
                          int num_remote_streams_using_mono) {
    
    
    CodecInst codec_inst;
    strcpy(codec_inst.plname, "L16");
    codec_inst.channels = 1;
    codec_inst.plfreq = kSampleRateHz;
    codec_inst.pltype = 105;
    codec_inst.pacsize = codec_inst.plfreq / 100;
    codec_inst.rate = codec_inst.plfreq * sizeof(int16_t) * 8;  

    for (int i = 0; i < num_remote_streams_using_mono; ++i) {
      
      
      
      SleepMs(50);
      StartRemoteStream(streams[i], codec_inst, 1234 + 2 * i);
    }

    
    codec_inst.channels = 2;
    codec_inst.pltype++;
    for (size_t i = num_remote_streams_using_mono; i < streams.size(); ++i) {
      StartRemoteStream(streams[i], codec_inst, 1234 + 2 * i);
    }
  }

  
  void StartRemoteStream(int stream, const CodecInst& codec_inst, int port) {
    EXPECT_EQ(0, voe_codec_->SetRecPayloadType(stream, codec_inst));
    EXPECT_EQ(0, voe_network_->RegisterExternalTransport(stream, *transport_));
    EXPECT_EQ(0, voe_base_->StartReceive(stream));
    EXPECT_EQ(0, voe_base_->StartPlayout(stream));
    EXPECT_EQ(0, voe_codec_->SetSendCodec(stream, codec_inst));
    EXPECT_EQ(0, voe_base_->StartSend(stream));
    EXPECT_EQ(0, voe_file_->StartPlayingFileAsMicrophone(stream,
        input_filename_.c_str(), true));
  }

  void StopRemoteStreams(const std::vector<int>& streams) {
    for (size_t i = 0; i < streams.size(); ++i) {
      EXPECT_EQ(0, voe_base_->StopSend(streams[i]));
      EXPECT_EQ(0, voe_base_->StopPlayout(streams[i]));
      EXPECT_EQ(0, voe_base_->StopReceive(streams[i]));
      EXPECT_EQ(0, voe_network_->DeRegisterExternalTransport(streams[i]));
      EXPECT_EQ(0, voe_base_->DeleteChannel(streams[i]));
    }
  }

  std::string input_filename_;
  const std::string output_filename_;
  LoopBackTransport* transport_;
};




TEST_F(MixingTest, MixManyChannelsForStress) {
  RunMixingTest(10, 0, 10, true, 0, 0, 0);
}




TEST_F(MixingTest, FourChannelsWithOnlyThreeMixed) {
  const int16_t kInputValue = 1000;
  const int16_t kExpectedOutput = kInputValue * 3;
  RunMixingTest(4, 0, 4, false, kInputValue, 1.1 * kExpectedOutput,
                0.9 * kExpectedOutput);
}




TEST_F(MixingTest, VerifySaturationProtection) {
  const int16_t kInputValue = 20000;
  const int16_t kExpectedOutput = kLimiterHeadroom;
  
  ASSERT_GT(kInputValue * 3, kInt16Max);
  ASSERT_LT(1.1 * kExpectedOutput, kInt16Max);
  RunMixingTest(3, 0, 3, false, kInputValue, 1.1 * kExpectedOutput,
               0.9 * kExpectedOutput);
}

TEST_F(MixingTest, SaturationProtectionHasNoEffectOnOneChannel) {
  const int16_t kInputValue = kInt16Max;
  const int16_t kExpectedOutput = kInt16Max;
  
  ASSERT_GT(0.95 * kExpectedOutput, kLimiterHeadroom);
  
  RunMixingTest(1, 0, 1, false, kInputValue, kExpectedOutput,
                0.95 * kExpectedOutput);
}

TEST_F(MixingTest, VerifyAnonymousAndNormalParticipantMixing) {
  const int16_t kInputValue = 1000;
  const int16_t kExpectedOutput = kInputValue * 2;
  RunMixingTest(1, 1, 1, false, kInputValue, 1.1 * kExpectedOutput,
                0.9 * kExpectedOutput);
}

TEST_F(MixingTest, AnonymousParticipantsAreAlwaysMixed) {
  const int16_t kInputValue = 1000;
  const int16_t kExpectedOutput = kInputValue * 4;
  RunMixingTest(3, 1, 3, false, kInputValue, 1.1 * kExpectedOutput,
                0.9 * kExpectedOutput);
}

TEST_F(MixingTest, VerifyStereoAndMonoMixing) {
  const int16_t kInputValue = 1000;
  const int16_t kExpectedOutput = kInputValue * 2;
  RunMixingTest(2, 0, 1, false, kInputValue, 1.1 * kExpectedOutput,
                
                0.8 * kExpectedOutput);
}

}  
