









#include <stdio.h>

#include <algorithm>
#include <queue>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/test/test_utils.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#ifdef WEBRTC_ANDROID_PLATFORM_BUILD
#include "gtest/gtest.h"
#include "external/webrtc/webrtc/modules/audio_processing/test/unittest.pb.h"
#else
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/audio_processing/unittest.pb.h"
#endif

#if (defined(WEBRTC_AUDIOPROC_FIXED_PROFILE)) || \
    (defined(WEBRTC_LINUX) && defined(WEBRTC_ARCH_X86_64) && !defined(NDEBUG))
#  define WEBRTC_AUDIOPROC_BIT_EXACT
#endif

using webrtc::AudioProcessing;
using webrtc::AudioFrame;
using webrtc::Config;
using webrtc::ExperimentalAgc;
using webrtc::GainControl;
using webrtc::NoiseSuppression;
using webrtc::EchoCancellation;
using webrtc::EventWrapper;
using webrtc::scoped_array;
using webrtc::scoped_ptr;
using webrtc::Trace;
using webrtc::LevelEstimator;
using webrtc::EchoCancellation;
using webrtc::EchoControlMobile;
using webrtc::VoiceDetection;

namespace {





#ifdef WEBRTC_AUDIOPROC_BIT_EXACT
bool write_ref_data = false;
const int kChannels[] = {1, 2};
const size_t kChannelsSize = sizeof(kChannels) / sizeof(*kChannels);
#endif

const int kSampleRates[] = {8000, 16000, 32000};
const size_t kSampleRatesSize = sizeof(kSampleRates) / sizeof(*kSampleRates);

#if defined(WEBRTC_AUDIOPROC_FIXED_PROFILE)

const int kProcessSampleRates[] = {8000, 16000};
#elif defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
const int kProcessSampleRates[] = {8000, 16000, 32000};
#endif
const size_t kProcessSampleRatesSize = sizeof(kProcessSampleRates) /
    sizeof(*kProcessSampleRates);

int TruncateToMultipleOf10(int value) {
  return (value / 10) * 10;
}


void MixStereoToMono(const int16_t* stereo,
                     int16_t* mono,
                     int samples_per_channel) {
  for (int i = 0; i < samples_per_channel; i++) {
    int32_t mono_s32 = (static_cast<int32_t>(stereo[i * 2]) +
        static_cast<int32_t>(stereo[i * 2 + 1])) >> 1;
    mono[i] = static_cast<int16_t>(mono_s32);
  }
}

void CopyLeftToRightChannel(int16_t* stereo, int samples_per_channel) {
  for (int i = 0; i < samples_per_channel; i++) {
    stereo[i * 2 + 1] = stereo[i * 2];
  }
}

void VerifyChannelsAreEqual(int16_t* stereo, int samples_per_channel) {
  for (int i = 0; i < samples_per_channel; i++) {
    EXPECT_EQ(stereo[i * 2 + 1], stereo[i * 2]);
  }
}

void SetFrameTo(AudioFrame* frame, int16_t value) {
  for (int i = 0; i < frame->samples_per_channel_ * frame->num_channels_;
      ++i) {
    frame->data_[i] = value;
  }
}

void SetFrameTo(AudioFrame* frame, int16_t left, int16_t right) {
  ASSERT_EQ(2, frame->num_channels_);
  for (int i = 0; i < frame->samples_per_channel_ * 2; i += 2) {
    frame->data_[i] = left;
    frame->data_[i + 1] = right;
  }
}

bool FrameDataAreEqual(const AudioFrame& frame1, const AudioFrame& frame2) {
  if (frame1.samples_per_channel_ !=
      frame2.samples_per_channel_) {
    return false;
  }
  if (frame1.num_channels_ !=
      frame2.num_channels_) {
    return false;
  }
  if (memcmp(frame1.data_, frame2.data_,
             frame1.samples_per_channel_ * frame1.num_channels_ *
               sizeof(int16_t))) {
    return false;
  }
  return true;
}

#ifdef WEBRTC_AUDIOPROC_BIT_EXACT

template <class T>
T AbsValue(T a) {
  return a > 0 ? a: -a;
}

int16_t MaxAudioFrame(const AudioFrame& frame) {
  const int length = frame.samples_per_channel_ * frame.num_channels_;
  int16_t max_data = AbsValue(frame.data_[0]);
  for (int i = 1; i < length; i++) {
    max_data = std::max(max_data, AbsValue(frame.data_[i]));
  }

  return max_data;
}

#if defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
void TestStats(const AudioProcessing::Statistic& test,
               const webrtc::audioproc::Test::Statistic& reference) {
  EXPECT_EQ(reference.instant(), test.instant);
  EXPECT_EQ(reference.average(), test.average);
  EXPECT_EQ(reference.maximum(), test.maximum);
  EXPECT_EQ(reference.minimum(), test.minimum);
}

void WriteStatsMessage(const AudioProcessing::Statistic& output,
                       webrtc::audioproc::Test::Statistic* message) {
  message->set_instant(output.instant);
  message->set_average(output.average);
  message->set_maximum(output.maximum);
  message->set_minimum(output.minimum);
}
#endif

void WriteMessageLiteToFile(const std::string filename,
                            const ::google::protobuf::MessageLite& message) {
  FILE* file = fopen(filename.c_str(), "wb");
  ASSERT_TRUE(file != NULL) << "Could not open " << filename;
  int size = message.ByteSize();
  ASSERT_GT(size, 0);
  unsigned char* array = new unsigned char[size];
  ASSERT_TRUE(message.SerializeToArray(array, size));

  ASSERT_EQ(1u, fwrite(&size, sizeof(int), 1, file));
  ASSERT_EQ(static_cast<size_t>(size),
      fwrite(array, sizeof(unsigned char), size, file));

  delete [] array;
  fclose(file);
}

void ReadMessageLiteFromFile(const std::string filename,
                             ::google::protobuf::MessageLite* message) {
  assert(message != NULL);

  FILE* file = fopen(filename.c_str(), "rb");
  ASSERT_TRUE(file != NULL) << "Could not open " << filename;
  int size = 0;
  ASSERT_EQ(1u, fread(&size, sizeof(int), 1, file));
  ASSERT_GT(size, 0);
  unsigned char* array = new unsigned char[size];
  ASSERT_EQ(static_cast<size_t>(size),
      fread(array, sizeof(unsigned char), size, file));

  ASSERT_TRUE(message->ParseFromArray(array, size));

  delete [] array;
  fclose(file);
}
#endif  

class ApmTest : public ::testing::Test {
 protected:
  ApmTest();
  virtual void SetUp();
  virtual void TearDown();

  static void SetUpTestCase() {
    Trace::CreateTrace();
    std::string trace_filename = webrtc::test::OutputPath() +
        "audioproc_trace.txt";
    ASSERT_EQ(0, Trace::SetTraceFile(trace_filename.c_str()));
  }

  static void TearDownTestCase() {
    Trace::ReturnTrace();
  }

  void Init(int sample_rate_hz, int num_reverse_channels,
            int num_input_channels, int num_output_channels,
            bool open_output_file);
  std::string ResourceFilePath(std::string name, int sample_rate_hz);
  std::string OutputFilePath(std::string name,
                             int sample_rate_hz,
                             int num_reverse_channels,
                             int num_input_channels,
                             int num_output_channels);
  void EnableAllComponents();
  bool ReadFrame(FILE* file, AudioFrame* frame);
  void ProcessWithDefaultStreamParameters(AudioFrame* frame);
  void ProcessDelayVerificationTest(int delay_ms, int system_delay_ms,
                                    int delay_min, int delay_max);
  void TestChangingChannels(int num_channels,
                            AudioProcessing::Error expected_return);

  const std::string output_path_;
  const std::string ref_path_;
  const std::string ref_filename_;
  scoped_ptr<webrtc::AudioProcessing> apm_;
  AudioFrame* frame_;
  AudioFrame* revframe_;
  FILE* far_file_;
  FILE* near_file_;
  FILE* out_file_;
};

ApmTest::ApmTest()
    : output_path_(webrtc::test::OutputPath()),
      ref_path_(webrtc::test::ProjectRootPath() +
                "data/audio_processing/"),
#if defined(WEBRTC_AUDIOPROC_FIXED_PROFILE)
      ref_filename_(ref_path_ + "output_data_fixed.pb"),
#elif defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
      ref_filename_(ref_path_ + "output_data_float.pb"),
#endif
      frame_(NULL),
      revframe_(NULL),
      far_file_(NULL),
      near_file_(NULL),
      out_file_(NULL) {
  Config config;
  config.Set<ExperimentalAgc>(new ExperimentalAgc(false));
  apm_.reset(AudioProcessing::Create(config));
}

void ApmTest::SetUp() {
  ASSERT_TRUE(apm_.get() != NULL);

  frame_ = new AudioFrame();
  revframe_ = new AudioFrame();

#if defined(WEBRTC_AUDIOPROC_FIXED_PROFILE)
  Init(16000, 2, 2, 2, false);
#else
  Init(32000, 2, 2, 2, false);
#endif
}

void ApmTest::TearDown() {
  if (frame_) {
    delete frame_;
  }
  frame_ = NULL;

  if (revframe_) {
    delete revframe_;
  }
  revframe_ = NULL;

  if (far_file_) {
    ASSERT_EQ(0, fclose(far_file_));
  }
  far_file_ = NULL;

  if (near_file_) {
    ASSERT_EQ(0, fclose(near_file_));
  }
  near_file_ = NULL;

  if (out_file_) {
    ASSERT_EQ(0, fclose(out_file_));
  }
  out_file_ = NULL;
}

std::string ApmTest::ResourceFilePath(std::string name, int sample_rate_hz) {
  std::ostringstream ss;
  
  ss << name << sample_rate_hz / 1000 << "_stereo";
  return webrtc::test::ResourcePath(ss.str(), "pcm");
}

std::string ApmTest::OutputFilePath(std::string name,
                                    int sample_rate_hz,
                                    int num_reverse_channels,
                                    int num_input_channels,
                                    int num_output_channels) {
  std::ostringstream ss;
  ss << name << sample_rate_hz / 1000 << "_" << num_reverse_channels << "r" <<
      num_input_channels << "i" << "_";
  if (num_output_channels == 1) {
    ss << "mono";
  } else if (num_output_channels == 2) {
    ss << "stereo";
  } else {
    assert(false);
    return "";
  }
  ss << ".pcm";

  return output_path_ + ss.str();
}

void ApmTest::Init(int sample_rate_hz, int num_reverse_channels,
                   int num_input_channels, int num_output_channels,
                   bool open_output_file) {
  
  const int samples_per_channel = kChunkSizeMs * sample_rate_hz / 1000;
  frame_->samples_per_channel_ = samples_per_channel;
  frame_->num_channels_ = num_input_channels;
  frame_->sample_rate_hz_ = sample_rate_hz;
  revframe_->samples_per_channel_ = samples_per_channel;
  revframe_->num_channels_ = num_reverse_channels;
  revframe_->sample_rate_hz_ = sample_rate_hz;

  
  
  int err = apm_->ProcessStream(frame_);
  ASSERT_TRUE(err == kNoErr || err == apm_->kStreamParameterNotSetError);
  ASSERT_EQ(apm_->kNoError, apm_->Initialize());

  if (far_file_) {
    ASSERT_EQ(0, fclose(far_file_));
  }
  std::string filename = ResourceFilePath("far", sample_rate_hz);
  far_file_ = fopen(filename.c_str(), "rb");
  ASSERT_TRUE(far_file_ != NULL) << "Could not open file " <<
      filename << "\n";

  if (near_file_) {
    ASSERT_EQ(0, fclose(near_file_));
  }
  filename = ResourceFilePath("near", sample_rate_hz);
  near_file_ = fopen(filename.c_str(), "rb");
  ASSERT_TRUE(near_file_ != NULL) << "Could not open file " <<
        filename << "\n";

  if (open_output_file) {
    if (out_file_) {
      ASSERT_EQ(0, fclose(out_file_));
    }
    filename = OutputFilePath("out", sample_rate_hz, num_reverse_channels,
                              num_input_channels, num_output_channels);
    out_file_ = fopen(filename.c_str(), "wb");
    ASSERT_TRUE(out_file_ != NULL) << "Could not open file " <<
          filename << "\n";
  }
}

void ApmTest::EnableAllComponents() {
#if defined(WEBRTC_AUDIOPROC_FIXED_PROFILE)
  EXPECT_EQ(apm_->kNoError, apm_->echo_control_mobile()->Enable(true));

  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_mode(GainControl::kAdaptiveDigital));
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(true));
#elif defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(true));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_metrics(true));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_delay_logging(true));
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));

  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_mode(GainControl::kAdaptiveAnalog));
  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_analog_level_limits(0, 255));
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(true));
#endif

  EXPECT_EQ(apm_->kNoError,
            apm_->high_pass_filter()->Enable(true));

  EXPECT_EQ(apm_->kNoError,
            apm_->level_estimator()->Enable(true));

  EXPECT_EQ(apm_->kNoError,
            apm_->noise_suppression()->Enable(true));

  EXPECT_EQ(apm_->kNoError,
            apm_->voice_detection()->Enable(true));
}

bool ApmTest::ReadFrame(FILE* file, AudioFrame* frame) {
  
  size_t frame_size = frame->samples_per_channel_ * 2;
  size_t read_count = fread(frame->data_,
                            sizeof(int16_t),
                            frame_size,
                            file);
  if (read_count != frame_size) {
    
    EXPECT_NE(0, feof(file));
    return false;  
  }

  if (frame->num_channels_ == 1) {
    MixStereoToMono(frame->data_, frame->data_,
                    frame->samples_per_channel_);
  }

  return true;
}

void ApmTest::ProcessWithDefaultStreamParameters(AudioFrame* frame) {
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError,
      apm_->gain_control()->set_stream_analog_level(127));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame));
}

void ApmTest::ProcessDelayVerificationTest(int delay_ms, int system_delay_ms,
                                           int delay_min, int delay_max) {
  
  
  webrtc::AudioFrame tmp_frame;
  std::queue<webrtc::AudioFrame*> frame_queue;
  bool causal = true;

  tmp_frame.CopyFrom(*revframe_);
  SetFrameTo(&tmp_frame, 0);

  EXPECT_EQ(apm_->kNoError, apm_->Initialize());
  
  int frame_delay = delay_ms / 10;
  while (frame_delay < 0) {
    webrtc::AudioFrame* frame = new AudioFrame();
    frame->CopyFrom(tmp_frame);
    frame_queue.push(frame);
    frame_delay++;
    causal = false;
  }
  while (frame_delay > 0) {
    webrtc::AudioFrame* frame = new AudioFrame();
    frame->CopyFrom(tmp_frame);
    frame_queue.push(frame);
    frame_delay--;
  }
  
  
  
  
  for (int frame_count = 0; frame_count < 450; ++frame_count) {
    webrtc::AudioFrame* frame = new AudioFrame();
    frame->CopyFrom(tmp_frame);
    
    ASSERT_TRUE(ReadFrame(near_file_, frame));
    frame_queue.push(frame);
    webrtc::AudioFrame* reverse_frame = frame;
    webrtc::AudioFrame* process_frame = frame_queue.front();
    if (!causal) {
      reverse_frame = frame_queue.front();
      
      
      
      process_frame = &tmp_frame;
      process_frame->CopyFrom(*frame);
    }
    EXPECT_EQ(apm_->kNoError, apm_->AnalyzeReverseStream(reverse_frame));
    EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(system_delay_ms));
    EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(process_frame));
    frame = frame_queue.front();
    frame_queue.pop();
    delete frame;

    if (frame_count == 250) {
      int median;
      int std;
      
      EXPECT_EQ(apm_->kNoError,
                apm_->echo_cancellation()->GetDelayMetrics(&median, &std));
    }
  }

  rewind(near_file_);
  while (!frame_queue.empty()) {
    webrtc::AudioFrame* frame = frame_queue.front();
    frame_queue.pop();
    delete frame;
  }
  
  
  const int samples_per_ms = std::min(16, frame_->samples_per_channel_ / 10);
  int expected_median = std::min(std::max(delay_ms - system_delay_ms,
                                          delay_min), delay_max);
  int expected_median_high = std::min(std::max(
      expected_median + 96 / samples_per_ms, delay_min), delay_max);
  int expected_median_low = std::min(std::max(
      expected_median - 96 / samples_per_ms, delay_min), delay_max);
  
  int median;
  int std;
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->GetDelayMetrics(&median, &std));
  EXPECT_GE(expected_median_high, median);
  EXPECT_LE(expected_median_low, median);
}

TEST_F(ApmTest, StreamParameters) {
  
  EXPECT_EQ(apm_->kNoError,
            apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(true));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_stream_analog_level(127));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(true));
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kStreamParameterNotSetError,
            apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(false));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(false));

  
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(true));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(true));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_stream_analog_level(127));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(false));

  
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_stream_analog_level(127));
  EXPECT_EQ(apm_->kStreamParameterNotSetError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError,
            apm_->AnalyzeReverseStream(revframe_));
  EXPECT_EQ(apm_->kStreamParameterNotSetError,
            apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError,
            apm_->gain_control()->set_stream_analog_level(127));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
}

TEST_F(ApmTest, DefaultDelayOffsetIsZero) {
  EXPECT_EQ(0, apm_->delay_offset_ms());
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(50));
  EXPECT_EQ(50, apm_->stream_delay_ms());
}

TEST_F(ApmTest, DelayOffsetWithLimitsIsSetProperly) {
  
  apm_->set_delay_offset_ms(100);
  EXPECT_EQ(100, apm_->delay_offset_ms());
  EXPECT_EQ(apm_->kBadStreamParameterWarning, apm_->set_stream_delay_ms(450));
  EXPECT_EQ(500, apm_->stream_delay_ms());
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  EXPECT_EQ(200, apm_->stream_delay_ms());

  
  apm_->set_delay_offset_ms(-50);
  EXPECT_EQ(-50, apm_->delay_offset_ms());
  EXPECT_EQ(apm_->kBadStreamParameterWarning, apm_->set_stream_delay_ms(20));
  EXPECT_EQ(0, apm_->stream_delay_ms());
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(100));
  EXPECT_EQ(50, apm_->stream_delay_ms());
}

void ApmTest::TestChangingChannels(int num_channels,
                                   AudioProcessing::Error expected_return) {
  frame_->num_channels_ = num_channels;
  EXPECT_EQ(expected_return, apm_->ProcessStream(frame_));
  EXPECT_EQ(expected_return, apm_->AnalyzeReverseStream(frame_));
}

TEST_F(ApmTest, Channels) {
  
  TestChangingChannels(0, apm_->kBadNumberChannelsError);
  TestChangingChannels(3, apm_->kBadNumberChannelsError);
  
  for (int i = 1; i < 3; i++) {
    TestChangingChannels(i, kNoErr);
    EXPECT_EQ(i, apm_->num_input_channels());
    EXPECT_EQ(i, apm_->num_reverse_channels());
  }
}

TEST_F(ApmTest, SampleRates) {
  
  SetFrameSampleRate(frame_, 10000);
  EXPECT_EQ(apm_->kBadSampleRateError, apm_->ProcessStream(frame_));
  
  int fs[] = {8000, 16000, 32000};
  for (size_t i = 0; i < sizeof(fs) / sizeof(*fs); i++) {
    SetFrameSampleRate(frame_, fs[i]);
    EXPECT_EQ(kNoErr, apm_->ProcessStream(frame_));
    EXPECT_EQ(fs[i], apm_->sample_rate_hz());
  }
}

TEST_F(ApmTest, EchoCancellation) {
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(true));
  EXPECT_TRUE(apm_->echo_cancellation()->is_drift_compensation_enabled());
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(false));
  EXPECT_FALSE(apm_->echo_cancellation()->is_drift_compensation_enabled());

  EXPECT_EQ(apm_->kBadParameterError,
      apm_->echo_cancellation()->set_device_sample_rate_hz(4000));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->echo_cancellation()->set_device_sample_rate_hz(100000));

  int rate[] = {16000, 44100, 48000};
  for (size_t i = 0; i < sizeof(rate)/sizeof(*rate); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->echo_cancellation()->set_device_sample_rate_hz(rate[i]));
    EXPECT_EQ(rate[i],
        apm_->echo_cancellation()->device_sample_rate_hz());
  }

  EchoCancellation::SuppressionLevel level[] = {
    EchoCancellation::kLowSuppression,
    EchoCancellation::kModerateSuppression,
    EchoCancellation::kHighSuppression,
  };
  for (size_t i = 0; i < sizeof(level)/sizeof(*level); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->echo_cancellation()->set_suppression_level(level[i]));
    EXPECT_EQ(level[i],
        apm_->echo_cancellation()->suppression_level());
  }

  EchoCancellation::Metrics metrics;
  EXPECT_EQ(apm_->kNotEnabledError,
            apm_->echo_cancellation()->GetMetrics(&metrics));

  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_metrics(true));
  EXPECT_TRUE(apm_->echo_cancellation()->are_metrics_enabled());
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_metrics(false));
  EXPECT_FALSE(apm_->echo_cancellation()->are_metrics_enabled());

  int median = 0;
  int std = 0;
  EXPECT_EQ(apm_->kNotEnabledError,
            apm_->echo_cancellation()->GetDelayMetrics(&median, &std));

  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_delay_logging(true));
  EXPECT_TRUE(apm_->echo_cancellation()->is_delay_logging_enabled());
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_delay_logging(false));
  EXPECT_FALSE(apm_->echo_cancellation()->is_delay_logging_enabled());

  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));
  EXPECT_TRUE(apm_->echo_cancellation()->is_enabled());
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(false));
  EXPECT_FALSE(apm_->echo_cancellation()->is_enabled());

  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));
  EXPECT_TRUE(apm_->echo_cancellation()->is_enabled());
  EXPECT_TRUE(apm_->echo_cancellation()->aec_core() != NULL);
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(false));
  EXPECT_FALSE(apm_->echo_cancellation()->is_enabled());
  EXPECT_FALSE(apm_->echo_cancellation()->aec_core() != NULL);
}

TEST_F(ApmTest, EchoCancellationReportsCorrectDelays) {
  
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_drift_compensation(false));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_metrics(false));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_cancellation()->enable_delay_logging(true));
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));

  
  
  const int kLookaheadBlocks = 15;
  const int kMaxDelayBlocks = 60;
  
  
  
  
  
  
  
  
  
  
  const int kSystemDelayMs = 66;
  
  
  
  for (size_t i = 0; i < kProcessSampleRatesSize; i++) {
    Init(kProcessSampleRates[i], 2, 2, 2, false);
    
    const int num_ms_per_block = std::max(4,
                                          640 / frame_->samples_per_channel_);
    const int delay_min_ms = -kLookaheadBlocks * num_ms_per_block;
    const int delay_max_ms = (kMaxDelayBlocks - 1) * num_ms_per_block;

    
    int delay_ms = TruncateToMultipleOf10(kSystemDelayMs + delay_min_ms);
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    
    
    delay_ms -= 20;
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    
    
    delay_ms = TruncateToMultipleOf10(kSystemDelayMs - 10);
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    delay_ms = TruncateToMultipleOf10(kSystemDelayMs);
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    delay_ms = TruncateToMultipleOf10(kSystemDelayMs + 10);
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    
    delay_ms = TruncateToMultipleOf10(kSystemDelayMs + delay_max_ms);
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
    
    
    delay_ms += 20;
    ProcessDelayVerificationTest(delay_ms, kSystemDelayMs, delay_min_ms,
                                 delay_max_ms);
  }
}

TEST_F(ApmTest, EchoControlMobile) {
  
  SetFrameSampleRate(frame_, 32000);
  EXPECT_EQ(kNoErr, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kBadSampleRateError,
            apm_->echo_control_mobile()->Enable(true));
  SetFrameSampleRate(frame_, 16000);
  EXPECT_EQ(kNoErr, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_control_mobile()->Enable(true));
  SetFrameSampleRate(frame_, 32000);
  EXPECT_EQ(apm_->kUnsupportedComponentError, apm_->ProcessStream(frame_));

  
  Init(16000, 2, 2, 2, false);
  EXPECT_EQ(apm_->kNoError, apm_->echo_control_mobile()->Enable(true));
  EXPECT_TRUE(apm_->echo_control_mobile()->is_enabled());

  
  EchoControlMobile::RoutingMode mode[] = {
      EchoControlMobile::kQuietEarpieceOrHeadset,
      EchoControlMobile::kEarpiece,
      EchoControlMobile::kLoudEarpiece,
      EchoControlMobile::kSpeakerphone,
      EchoControlMobile::kLoudSpeakerphone,
  };
  for (size_t i = 0; i < sizeof(mode)/sizeof(*mode); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->echo_control_mobile()->set_routing_mode(mode[i]));
    EXPECT_EQ(mode[i],
        apm_->echo_control_mobile()->routing_mode());
  }
  
  EXPECT_EQ(apm_->kNoError,
      apm_->echo_control_mobile()->enable_comfort_noise(false));
  EXPECT_FALSE(apm_->echo_control_mobile()->is_comfort_noise_enabled());
  EXPECT_EQ(apm_->kNoError,
      apm_->echo_control_mobile()->enable_comfort_noise(true));
  EXPECT_TRUE(apm_->echo_control_mobile()->is_comfort_noise_enabled());
  
  const size_t echo_path_size =
      apm_->echo_control_mobile()->echo_path_size_bytes();
  scoped_array<char> echo_path_in(new char[echo_path_size]);
  scoped_array<char> echo_path_out(new char[echo_path_size]);
  EXPECT_EQ(apm_->kNullPointerError,
            apm_->echo_control_mobile()->SetEchoPath(NULL, echo_path_size));
  EXPECT_EQ(apm_->kNullPointerError,
            apm_->echo_control_mobile()->GetEchoPath(NULL, echo_path_size));
  EXPECT_EQ(apm_->kBadParameterError,
            apm_->echo_control_mobile()->GetEchoPath(echo_path_out.get(), 1));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_control_mobile()->GetEchoPath(echo_path_out.get(),
                                                     echo_path_size));
  for (size_t i = 0; i < echo_path_size; i++) {
    echo_path_in[i] = echo_path_out[i] + 1;
  }
  EXPECT_EQ(apm_->kBadParameterError,
            apm_->echo_control_mobile()->SetEchoPath(echo_path_in.get(), 1));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_control_mobile()->SetEchoPath(echo_path_in.get(),
                                                     echo_path_size));
  EXPECT_EQ(apm_->kNoError,
            apm_->echo_control_mobile()->GetEchoPath(echo_path_out.get(),
                                                     echo_path_size));
  for (size_t i = 0; i < echo_path_size; i++) {
    EXPECT_EQ(echo_path_in[i], echo_path_out[i]);
  }

  
  
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));

  
  EXPECT_EQ(apm_->kNoError, apm_->echo_control_mobile()->Enable(false));
  EXPECT_FALSE(apm_->echo_control_mobile()->is_enabled());
}

TEST_F(ApmTest, GainControl) {
  
  EXPECT_EQ(apm_->kNoError,
      apm_->gain_control()->set_mode(
      apm_->gain_control()->mode()));

  GainControl::Mode mode[] = {
    GainControl::kAdaptiveAnalog,
    GainControl::kAdaptiveDigital,
    GainControl::kFixedDigital
  };
  for (size_t i = 0; i < sizeof(mode)/sizeof(*mode); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->gain_control()->set_mode(mode[i]));
    EXPECT_EQ(mode[i], apm_->gain_control()->mode());
  }
  
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_target_level_dbfs(-3));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_target_level_dbfs(-40));
  
  EXPECT_EQ(apm_->kNoError,
      apm_->gain_control()->set_target_level_dbfs(
      apm_->gain_control()->target_level_dbfs()));

  int level_dbfs[] = {0, 6, 31};
  for (size_t i = 0; i < sizeof(level_dbfs)/sizeof(*level_dbfs); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->gain_control()->set_target_level_dbfs(level_dbfs[i]));
    EXPECT_EQ(level_dbfs[i], apm_->gain_control()->target_level_dbfs());
  }

  
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_compression_gain_db(-1));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_compression_gain_db(100));

  
  EXPECT_EQ(apm_->kNoError,
      apm_->gain_control()->set_compression_gain_db(
      apm_->gain_control()->compression_gain_db()));

  int gain_db[] = {0, 10, 90};
  for (size_t i = 0; i < sizeof(gain_db)/sizeof(*gain_db); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->gain_control()->set_compression_gain_db(gain_db[i]));
    EXPECT_EQ(gain_db[i], apm_->gain_control()->compression_gain_db());
  }

  
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->enable_limiter(false));
  EXPECT_FALSE(apm_->gain_control()->is_limiter_enabled());
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->enable_limiter(true));
  EXPECT_TRUE(apm_->gain_control()->is_limiter_enabled());

  
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_analog_level_limits(-1, 512));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_analog_level_limits(100000, 512));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_analog_level_limits(512, -1));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_analog_level_limits(512, 100000));
  EXPECT_EQ(apm_->kBadParameterError,
      apm_->gain_control()->set_analog_level_limits(512, 255));

  
  EXPECT_EQ(apm_->kNoError,
      apm_->gain_control()->set_analog_level_limits(
      apm_->gain_control()->analog_level_minimum(),
      apm_->gain_control()->analog_level_maximum()));

  int min_level[] = {0, 255, 1024};
  for (size_t i = 0; i < sizeof(min_level)/sizeof(*min_level); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->gain_control()->set_analog_level_limits(min_level[i], 1024));
    EXPECT_EQ(min_level[i], apm_->gain_control()->analog_level_minimum());
  }

  int max_level[] = {0, 1024, 65535};
  for (size_t i = 0; i < sizeof(min_level)/sizeof(*min_level); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->gain_control()->set_analog_level_limits(0, max_level[i]));
    EXPECT_EQ(max_level[i], apm_->gain_control()->analog_level_maximum());
  }

  

  
  EXPECT_EQ(apm_->kNoError, apm_->gain_control()->Enable(false));
  EXPECT_FALSE(apm_->gain_control()->is_enabled());
}

TEST_F(ApmTest, NoiseSuppression) {
  
  NoiseSuppression::Level level[] = {
    NoiseSuppression::kLow,
    NoiseSuppression::kModerate,
    NoiseSuppression::kHigh,
    NoiseSuppression::kVeryHigh
  };
  for (size_t i = 0; i < sizeof(level)/sizeof(*level); i++) {
    EXPECT_EQ(apm_->kNoError,
        apm_->noise_suppression()->set_level(level[i]));
    EXPECT_EQ(level[i], apm_->noise_suppression()->level());
  }

  
  EXPECT_EQ(apm_->kNoError, apm_->noise_suppression()->Enable(true));
  EXPECT_TRUE(apm_->noise_suppression()->is_enabled());
  EXPECT_EQ(apm_->kNoError, apm_->noise_suppression()->Enable(false));
  EXPECT_FALSE(apm_->noise_suppression()->is_enabled());
}

TEST_F(ApmTest, HighPassFilter) {
  
  EXPECT_EQ(apm_->kNoError, apm_->high_pass_filter()->Enable(true));
  EXPECT_TRUE(apm_->high_pass_filter()->is_enabled());
  EXPECT_EQ(apm_->kNoError, apm_->high_pass_filter()->Enable(false));
  EXPECT_FALSE(apm_->high_pass_filter()->is_enabled());
}

TEST_F(ApmTest, LevelEstimator) {
  
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(false));
  EXPECT_FALSE(apm_->level_estimator()->is_enabled());

  EXPECT_EQ(apm_->kNotEnabledError, apm_->level_estimator()->RMS());

  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(true));
  EXPECT_TRUE(apm_->level_estimator()->is_enabled());

  
  
  frame_->samples_per_channel_ = 160;
  frame_->num_channels_ = 2;
  frame_->sample_rate_hz_ = 16000;

  
  EXPECT_EQ(127, apm_->level_estimator()->RMS());

  
  SetFrameTo(frame_, 0);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(127, apm_->level_estimator()->RMS());

  
  
  SetFrameTo(frame_, 32767);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(0, apm_->level_estimator()->RMS());

  SetFrameTo(frame_, 30000);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(1, apm_->level_estimator()->RMS());

  SetFrameTo(frame_, 10000);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(10, apm_->level_estimator()->RMS());

  SetFrameTo(frame_, 10);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(70, apm_->level_estimator()->RMS());

  
  SetFrameTo(frame_, 10000);
  uint32_t energy = frame_->energy_;  
  frame_->energy_ = 0;
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(127, apm_->level_estimator()->RMS());
  frame_->energy_ = energy;

  
  SetFrameTo(frame_, 32767);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(false));
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(true));
  SetFrameTo(frame_, 1);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(90, apm_->level_estimator()->RMS());

  
  SetFrameTo(frame_, 32767);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->Initialize());
  SetFrameTo(frame_, 1);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(90, apm_->level_estimator()->RMS());
}

TEST_F(ApmTest, VoiceDetection) {
  
  EXPECT_EQ(apm_->kNoError,
            apm_->voice_detection()->set_stream_has_voice(true));
  EXPECT_TRUE(apm_->voice_detection()->stream_has_voice());
  EXPECT_EQ(apm_->kNoError,
            apm_->voice_detection()->set_stream_has_voice(false));
  EXPECT_FALSE(apm_->voice_detection()->stream_has_voice());

  
  VoiceDetection::Likelihood likelihood[] = {
      VoiceDetection::kVeryLowLikelihood,
      VoiceDetection::kLowLikelihood,
      VoiceDetection::kModerateLikelihood,
      VoiceDetection::kHighLikelihood
  };
  for (size_t i = 0; i < sizeof(likelihood)/sizeof(*likelihood); i++) {
    EXPECT_EQ(apm_->kNoError,
              apm_->voice_detection()->set_likelihood(likelihood[i]));
    EXPECT_EQ(likelihood[i], apm_->voice_detection()->likelihood());
  }

  












  
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(true));
  EXPECT_TRUE(apm_->voice_detection()->is_enabled());
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(false));
  EXPECT_FALSE(apm_->voice_detection()->is_enabled());

  
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(false));
  AudioFrame::VADActivity activity[] = {
      AudioFrame::kVadActive,
      AudioFrame::kVadPassive,
      AudioFrame::kVadUnknown
  };
  for (size_t i = 0; i < sizeof(activity)/sizeof(*activity); i++) {
    frame_->vad_activity_ = activity[i];
    EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
    EXPECT_EQ(activity[i], frame_->vad_activity_);
  }

  
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(true));
  frame_->vad_activity_ = AudioFrame::kVadUnknown;
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_NE(AudioFrame::kVadUnknown, frame_->vad_activity_);

  
}

TEST_F(ApmTest, AllProcessingDisabledByDefault) {
  EXPECT_FALSE(apm_->echo_cancellation()->is_enabled());
  EXPECT_FALSE(apm_->echo_control_mobile()->is_enabled());
  EXPECT_FALSE(apm_->gain_control()->is_enabled());
  EXPECT_FALSE(apm_->high_pass_filter()->is_enabled());
  EXPECT_FALSE(apm_->level_estimator()->is_enabled());
  EXPECT_FALSE(apm_->noise_suppression()->is_enabled());
  EXPECT_FALSE(apm_->voice_detection()->is_enabled());
}

TEST_F(ApmTest, NoProcessingWhenAllComponentsDisabled) {
  for (size_t i = 0; i < kSampleRatesSize; i++) {
    Init(kSampleRates[i], 2, 2, 2, false);
    SetFrameTo(frame_, 1000, 2000);
    AudioFrame frame_copy;
    frame_copy.CopyFrom(*frame_);
    for (int j = 0; j < 1000; j++) {
      EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
      EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));
    }
  }
}

TEST_F(ApmTest, IdenticalInputChannelsResultInIdenticalOutputChannels) {
  EnableAllComponents();

  for (size_t i = 0; i < kProcessSampleRatesSize; i++) {
    Init(kProcessSampleRates[i], 2, 2, 2, false);
    int analog_level = 127;
    EXPECT_EQ(0, feof(far_file_));
    EXPECT_EQ(0, feof(near_file_));
    while (1) {
      if (!ReadFrame(far_file_, revframe_)) break;
      CopyLeftToRightChannel(revframe_->data_, revframe_->samples_per_channel_);

      EXPECT_EQ(apm_->kNoError, apm_->AnalyzeReverseStream(revframe_));

      if (!ReadFrame(near_file_, frame_)) break;
      CopyLeftToRightChannel(frame_->data_, frame_->samples_per_channel_);
      frame_->vad_activity_ = AudioFrame::kVadUnknown;

      EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
      apm_->echo_cancellation()->set_stream_drift_samples(0);
      EXPECT_EQ(apm_->kNoError,
          apm_->gain_control()->set_stream_analog_level(analog_level));
      EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
      analog_level = apm_->gain_control()->stream_analog_level();

      VerifyChannelsAreEqual(frame_->data_, frame_->samples_per_channel_);
    }
    rewind(far_file_);
    rewind(near_file_);
  }
}

TEST_F(ApmTest, SplittingFilter) {
  
  
  SetFrameTo(frame_, 1000);
  AudioFrame frame_copy;
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));

  
  SetFrameTo(frame_, 1000);
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(false));

  
  SetFrameTo(frame_, 1000);
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(false));

  
  SetFrameTo(frame_, 1000);
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(true));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));
  EXPECT_EQ(apm_->kNoError, apm_->level_estimator()->Enable(false));
  EXPECT_EQ(apm_->kNoError, apm_->voice_detection()->Enable(false));

  
  frame_->samples_per_channel_ = 160;
  frame_->num_channels_ = 2;
  frame_->sample_rate_hz_ = 16000;
  
  
  
  
  EXPECT_EQ(apm_->kNoError, apm_->echo_cancellation()->Enable(true));
  SetFrameTo(frame_, 1000);
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_TRUE(FrameDataAreEqual(*frame_, frame_copy));

  
  
  frame_->samples_per_channel_ = 320;
  frame_->num_channels_ = 2;
  frame_->sample_rate_hz_ = 32000;
  SetFrameTo(frame_, 1000);
  frame_copy.CopyFrom(*frame_);
  EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
  apm_->echo_cancellation()->set_stream_drift_samples(0);
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_FALSE(FrameDataAreEqual(*frame_, frame_copy));
}


TEST_F(ApmTest, DebugDump) {
  const std::string filename = webrtc::test::OutputPath() + "debug.aec";
  EXPECT_EQ(apm_->kNullPointerError,
            apm_->StartDebugRecording(static_cast<const char*>(NULL)));

#ifdef WEBRTC_AUDIOPROC_DEBUG_DUMP
  
  EXPECT_EQ(apm_->kNoError, apm_->StopDebugRecording());

  EXPECT_EQ(apm_->kNoError, apm_->StartDebugRecording(filename.c_str()));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->AnalyzeReverseStream(revframe_));
  EXPECT_EQ(apm_->kNoError, apm_->StopDebugRecording());

  
  FILE* fid = fopen(filename.c_str(), "r");
  ASSERT_TRUE(fid != NULL);

  
  ASSERT_EQ(0, fclose(fid));
  ASSERT_EQ(0, remove(filename.c_str()));
#else
  EXPECT_EQ(apm_->kUnsupportedFunctionError,
            apm_->StartDebugRecording(filename.c_str()));
  EXPECT_EQ(apm_->kUnsupportedFunctionError, apm_->StopDebugRecording());

  
  ASSERT_TRUE(fopen(filename.c_str(), "r") == NULL);
#endif  
}


TEST_F(ApmTest, DebugDumpFromFileHandle) {
  FILE* fid = NULL;
  EXPECT_EQ(apm_->kNullPointerError, apm_->StartDebugRecording(fid));
  const std::string filename = webrtc::test::OutputPath() + "debug.aec";
  fid = fopen(filename.c_str(), "w");
  ASSERT_TRUE(fid);

#ifdef WEBRTC_AUDIOPROC_DEBUG_DUMP
  
  EXPECT_EQ(apm_->kNoError, apm_->StopDebugRecording());

  EXPECT_EQ(apm_->kNoError, apm_->StartDebugRecording(fid));
  EXPECT_EQ(apm_->kNoError, apm_->AnalyzeReverseStream(revframe_));
  EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
  EXPECT_EQ(apm_->kNoError, apm_->StopDebugRecording());

  
  fid = fopen(filename.c_str(), "r");
  ASSERT_TRUE(fid != NULL);

  
  ASSERT_EQ(0, fclose(fid));
  ASSERT_EQ(0, remove(filename.c_str()));
#else
  EXPECT_EQ(apm_->kUnsupportedFunctionError,
            apm_->StartDebugRecording(fid));
  EXPECT_EQ(apm_->kUnsupportedFunctionError, apm_->StopDebugRecording());

  ASSERT_EQ(0, fclose(fid));
#endif  
}






#ifdef WEBRTC_AUDIOPROC_BIT_EXACT
TEST_F(ApmTest, DISABLED_ON_ANDROID(Process)) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  webrtc::audioproc::OutputData ref_data;

  if (!write_ref_data) {
    ReadMessageLiteFromFile(ref_filename_, &ref_data);
  } else {
    
    for (size_t i = 0; i < kChannelsSize; i++) {
      for (size_t j = 0; j < kChannelsSize; j++) {
        for (size_t l = 0; l < kProcessSampleRatesSize; l++) {
          webrtc::audioproc::Test* test = ref_data.add_test();
          test->set_num_reverse_channels(kChannels[i]);
          test->set_num_input_channels(kChannels[j]);
          test->set_num_output_channels(kChannels[j]);
          test->set_sample_rate(kProcessSampleRates[l]);
        }
      }
    }
  }

  EnableAllComponents();

  for (int i = 0; i < ref_data.test_size(); i++) {
    printf("Running test %d of %d...\n", i + 1, ref_data.test_size());

    webrtc::audioproc::Test* test = ref_data.mutable_test(i);
    
    
    if (test->num_input_channels() != test->num_output_channels())
      continue;

    Init(test->sample_rate(), test->num_reverse_channels(),
         test->num_input_channels(), test->num_output_channels(), true);

    int frame_count = 0;
    int has_echo_count = 0;
    int has_voice_count = 0;
    int is_saturated_count = 0;
    int analog_level = 127;
    int analog_level_average = 0;
    int max_output_average = 0;
    float ns_speech_prob_average = 0.0f;

    while (1) {
      if (!ReadFrame(far_file_, revframe_)) break;
      EXPECT_EQ(apm_->kNoError, apm_->AnalyzeReverseStream(revframe_));

      if (!ReadFrame(near_file_, frame_)) break;
      frame_->vad_activity_ = AudioFrame::kVadUnknown;

      EXPECT_EQ(apm_->kNoError, apm_->set_stream_delay_ms(0));
      apm_->echo_cancellation()->set_stream_drift_samples(0);
      EXPECT_EQ(apm_->kNoError,
          apm_->gain_control()->set_stream_analog_level(analog_level));

      EXPECT_EQ(apm_->kNoError, apm_->ProcessStream(frame_));
      
      EXPECT_EQ(test->num_output_channels(), frame_->num_channels_);

      max_output_average += MaxAudioFrame(*frame_);

      if (apm_->echo_cancellation()->stream_has_echo()) {
        has_echo_count++;
      }

      analog_level = apm_->gain_control()->stream_analog_level();
      analog_level_average += analog_level;
      if (apm_->gain_control()->stream_is_saturated()) {
        is_saturated_count++;
      }
      if (apm_->voice_detection()->stream_has_voice()) {
        has_voice_count++;
        EXPECT_EQ(AudioFrame::kVadActive, frame_->vad_activity_);
      } else {
        EXPECT_EQ(AudioFrame::kVadPassive, frame_->vad_activity_);
      }

      ns_speech_prob_average += apm_->noise_suppression()->speech_probability();

      size_t frame_size = frame_->samples_per_channel_ * frame_->num_channels_;
      size_t write_count = fwrite(frame_->data_,
                                  sizeof(int16_t),
                                  frame_size,
                                  out_file_);
      ASSERT_EQ(frame_size, write_count);

      
      frame_->num_channels_ = test->num_input_channels();
      frame_count++;
    }
    max_output_average /= frame_count;
    analog_level_average /= frame_count;
    ns_speech_prob_average /= frame_count;

#if defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
    EchoCancellation::Metrics echo_metrics;
    EXPECT_EQ(apm_->kNoError,
              apm_->echo_cancellation()->GetMetrics(&echo_metrics));
    int median = 0;
    int std = 0;
    EXPECT_EQ(apm_->kNoError,
              apm_->echo_cancellation()->GetDelayMetrics(&median, &std));

    int rms_level = apm_->level_estimator()->RMS();
    EXPECT_LE(0, rms_level);
    EXPECT_GE(127, rms_level);
#endif

    if (!write_ref_data) {
      EXPECT_EQ(test->has_echo_count(), has_echo_count);
      EXPECT_EQ(test->has_voice_count(), has_voice_count);
      EXPECT_EQ(test->is_saturated_count(), is_saturated_count);

      EXPECT_EQ(test->analog_level_average(), analog_level_average);
      EXPECT_EQ(test->max_output_average(), max_output_average);

#if defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
      webrtc::audioproc::Test::EchoMetrics reference =
          test->echo_metrics();
      TestStats(echo_metrics.residual_echo_return_loss,
                reference.residual_echo_return_loss());
      TestStats(echo_metrics.echo_return_loss,
                reference.echo_return_loss());
      TestStats(echo_metrics.echo_return_loss_enhancement,
                reference.echo_return_loss_enhancement());
      TestStats(echo_metrics.a_nlp,
                reference.a_nlp());

      webrtc::audioproc::Test::DelayMetrics reference_delay =
          test->delay_metrics();
      EXPECT_EQ(reference_delay.median(), median);
      EXPECT_EQ(reference_delay.std(), std);

      EXPECT_EQ(test->rms_level(), rms_level);

      EXPECT_FLOAT_EQ(test->ns_speech_probability_average(),
                      ns_speech_prob_average);
#endif
    } else {
      test->set_has_echo_count(has_echo_count);
      test->set_has_voice_count(has_voice_count);
      test->set_is_saturated_count(is_saturated_count);

      test->set_analog_level_average(analog_level_average);
      test->set_max_output_average(max_output_average);

#if defined(WEBRTC_AUDIOPROC_FLOAT_PROFILE)
      webrtc::audioproc::Test::EchoMetrics* message =
          test->mutable_echo_metrics();
      WriteStatsMessage(echo_metrics.residual_echo_return_loss,
                        message->mutable_residual_echo_return_loss());
      WriteStatsMessage(echo_metrics.echo_return_loss,
                        message->mutable_echo_return_loss());
      WriteStatsMessage(echo_metrics.echo_return_loss_enhancement,
                        message->mutable_echo_return_loss_enhancement());
      WriteStatsMessage(echo_metrics.a_nlp,
                        message->mutable_a_nlp());

      webrtc::audioproc::Test::DelayMetrics* message_delay =
          test->mutable_delay_metrics();
      message_delay->set_median(median);
      message_delay->set_std(std);

      test->set_rms_level(rms_level);

      EXPECT_LE(0.0f, ns_speech_prob_average);
      EXPECT_GE(1.0f, ns_speech_prob_average);
      test->set_ns_speech_probability_average(ns_speech_prob_average);
#endif
    }

    rewind(far_file_);
    rewind(near_file_);
  }

  if (write_ref_data) {
    WriteMessageLiteToFile(ref_filename_, ref_data);
  }
}
#endif  





}  
