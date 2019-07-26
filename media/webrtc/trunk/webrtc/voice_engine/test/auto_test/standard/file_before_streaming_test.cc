









#include "after_initialization_fixture.h"
#include "test/testsupport/fileutils.h"

namespace {

const int kSampleRateHz = 16000;
const int kTestDurationMs = 1000;
const int kSkipOutputMs = 50;
const int16_t kInputValue = 15000;
const int16_t kSilenceValue = 0;

}  

class FileBeforeStreamingTest : public AfterInitializationFixture {
 protected:
  FileBeforeStreamingTest()
      : input_filename_(webrtc::test::OutputPath() + "file_test_input.pcm"),
        output_filename_(webrtc::test::OutputPath() + "file_test_output.pcm") {
  }

  void SetUp() {
    channel_ = voe_base_->CreateChannel();
  }

  void TearDown() {
    voe_base_->DeleteChannel(channel_);
  }

  

  
  
  void GenerateInputFile() {
    FILE* input_file = fopen(input_filename_.c_str(), "wb");
    ASSERT_TRUE(input_file != NULL);
    for (int i = 0; i < kSampleRateHz / 1000 * (kTestDurationMs + 1000); i++) {
      ASSERT_EQ(1u, fwrite(&kInputValue, sizeof(kInputValue), 1, input_file));
    }
    ASSERT_EQ(0, fclose(input_file));
  }

  void RecordOutput() {
    
    EXPECT_EQ(0, voe_file_->StartRecordingPlayout(-1,
        output_filename_.c_str()));
    Sleep(kTestDurationMs);
    EXPECT_EQ(0, voe_file_->StopRecordingPlayout(-1));
  }

  void VerifyOutput(int16_t target_value) {
    FILE* output_file = fopen(output_filename_.c_str(), "rb");
    ASSERT_TRUE(output_file != NULL);
    int16_t output_value = 0;
    int samples_read = 0;

    
    EXPECT_EQ(0, fseek(output_file, sizeof(output_value) *
                       kSampleRateHz / 1000 * kSkipOutputMs, SEEK_SET));
    while (fread(&output_value, sizeof(output_value), 1, output_file) == 1) {
      samples_read++;
      EXPECT_EQ(output_value, target_value);
    }

    
    
    ASSERT_GE((samples_read * 1000.0) / kSampleRateHz, 0.4 * kTestDurationMs);

    
    ASSERT_NE(0, feof(output_file));
    ASSERT_EQ(0, fclose(output_file));
  }

void VerifyEmptyOutput() {
  FILE* output_file = fopen(output_filename_.c_str(), "rb");
  ASSERT_TRUE(output_file != NULL);
  ASSERT_EQ(0, fseek(output_file, 0, SEEK_END));
  EXPECT_EQ(0, ftell(output_file));
  ASSERT_EQ(0, fclose(output_file));
}

  int channel_;
  const std::string input_filename_;
  const std::string output_filename_;
};







TEST_F(FileBeforeStreamingTest, TestStartPlayingFileLocallyWithStartPlayout) {
  GenerateInputFile();

  TEST_LOG("Playout is not started. File will not be played out.\n");
  EXPECT_EQ(0, voe_file_->StartPlayingFileLocally(
      channel_, input_filename_.c_str(), true));
  EXPECT_EQ(1, voe_file_->IsPlayingFileLocally(channel_));
  RecordOutput();
  VerifyEmptyOutput();

  TEST_LOG("Playout is now started. File will be played out.\n");
  EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
  RecordOutput();
  VerifyOutput(kInputValue);

  TEST_LOG("Stop playing file. Only silence will be played out.\n");
  EXPECT_EQ(0, voe_file_->StopPlayingFileLocally(channel_));
  EXPECT_EQ(0, voe_file_->IsPlayingFileLocally(channel_));
  RecordOutput();
  VerifyOutput(kSilenceValue);

  TEST_LOG("Start playing file again. File will be played out.\n");
  EXPECT_EQ(0, voe_file_->StartPlayingFileLocally(
      channel_, input_filename_.c_str(), true));
  EXPECT_EQ(1, voe_file_->IsPlayingFileLocally(channel_));
  RecordOutput();
  VerifyOutput(kInputValue);

  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));
  EXPECT_EQ(0, voe_file_->StopPlayingFileLocally(channel_));
}
