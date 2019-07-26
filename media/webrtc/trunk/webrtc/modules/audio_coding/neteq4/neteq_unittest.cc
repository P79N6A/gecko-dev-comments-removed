













#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"

#include <stdlib.h>
#include <string.h>  

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq4/test/NETEQTEST_RTPpacket.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RefFiles {
 public:
  RefFiles(const std::string& input_file, const std::string& output_file);
  ~RefFiles();
  template<class T> void ProcessReference(const T& test_results);
  template<typename T, size_t n> void ProcessReference(
      const T (&test_results)[n],
      size_t length);
  template<typename T, size_t n> void WriteToFile(
      const T (&test_results)[n],
      size_t length);
  template<typename T, size_t n> void ReadFromFileAndCompare(
      const T (&test_results)[n],
      size_t length);
  void WriteToFile(const NetEqNetworkStatistics& stats);
  void ReadFromFileAndCompare(const NetEqNetworkStatistics& stats);
  void WriteToFile(const RtcpStatistics& stats);
  void ReadFromFileAndCompare(const RtcpStatistics& stats);

  FILE* input_fp_;
  FILE* output_fp_;
};

RefFiles::RefFiles(const std::string &input_file,
                   const std::string &output_file)
    : input_fp_(NULL),
      output_fp_(NULL) {
  if (!input_file.empty()) {
    input_fp_ = fopen(input_file.c_str(), "rb");
    EXPECT_TRUE(input_fp_ != NULL);
  }
  if (!output_file.empty()) {
    output_fp_ = fopen(output_file.c_str(), "wb");
    EXPECT_TRUE(output_fp_ != NULL);
  }
}

RefFiles::~RefFiles() {
  if (input_fp_) {
    EXPECT_EQ(EOF, fgetc(input_fp_));  
    fclose(input_fp_);
  }
  if (output_fp_) fclose(output_fp_);
}

template<class T>
void RefFiles::ProcessReference(const T& test_results) {
  WriteToFile(test_results);
  ReadFromFileAndCompare(test_results);
}

template<typename T, size_t n>
void RefFiles::ProcessReference(const T (&test_results)[n], size_t length) {
  WriteToFile(test_results, length);
  ReadFromFileAndCompare(test_results, length);
}

template<typename T, size_t n>
void RefFiles::WriteToFile(const T (&test_results)[n], size_t length) {
  if (output_fp_) {
    ASSERT_EQ(length, fwrite(&test_results, sizeof(T), length, output_fp_));
  }
}

template<typename T, size_t n>
void RefFiles::ReadFromFileAndCompare(const T (&test_results)[n],
                                      size_t length) {
  if (input_fp_) {
    
    T* ref = new T[length];
    ASSERT_EQ(length, fread(ref, sizeof(T), length, input_fp_));
    
    ASSERT_EQ(0, memcmp(&test_results, ref, sizeof(T) * length));
    delete [] ref;
  }
}

void RefFiles::WriteToFile(const NetEqNetworkStatistics& stats) {
  if (output_fp_) {
    ASSERT_EQ(1u, fwrite(&stats, sizeof(NetEqNetworkStatistics), 1,
                         output_fp_));
  }
}

void RefFiles::ReadFromFileAndCompare(
    const NetEqNetworkStatistics& stats) {
  if (input_fp_) {
    
    size_t stat_size = sizeof(NetEqNetworkStatistics);
    NetEqNetworkStatistics ref_stats;
    ASSERT_EQ(1u, fread(&ref_stats, stat_size, 1, input_fp_));
    
    EXPECT_EQ(0, memcmp(&stats, &ref_stats, stat_size));
  }
}

void RefFiles::WriteToFile(const RtcpStatistics& stats) {
  if (output_fp_) {
    ASSERT_EQ(1u, fwrite(&(stats.fraction_lost), sizeof(stats.fraction_lost), 1,
                         output_fp_));
    ASSERT_EQ(1u, fwrite(&(stats.cumulative_lost),
                         sizeof(stats.cumulative_lost), 1, output_fp_));
    ASSERT_EQ(1u, fwrite(&(stats.extended_max), sizeof(stats.extended_max), 1,
                         output_fp_));
    ASSERT_EQ(1u, fwrite(&(stats.jitter), sizeof(stats.jitter), 1,
                         output_fp_));
  }
}

void RefFiles::ReadFromFileAndCompare(
    const RtcpStatistics& stats) {
  if (input_fp_) {
    
    RtcpStatistics ref_stats;
    ASSERT_EQ(1u, fread(&(ref_stats.fraction_lost),
                        sizeof(ref_stats.fraction_lost), 1, input_fp_));
    ASSERT_EQ(1u, fread(&(ref_stats.cumulative_lost),
                        sizeof(ref_stats.cumulative_lost), 1, input_fp_));
    ASSERT_EQ(1u, fread(&(ref_stats.extended_max),
                        sizeof(ref_stats.extended_max), 1, input_fp_));
    ASSERT_EQ(1u, fread(&(ref_stats.jitter), sizeof(ref_stats.jitter), 1,
                        input_fp_));
    
    EXPECT_EQ(ref_stats.fraction_lost, stats.fraction_lost);
    EXPECT_EQ(ref_stats.cumulative_lost, stats.cumulative_lost);
    EXPECT_EQ(ref_stats.extended_max, stats.extended_max);
    EXPECT_EQ(ref_stats.jitter, stats.jitter);
  }
}

class NetEqDecodingTest : public ::testing::Test {
 protected:
  
  
  static const int kTimeStepMs = 10;
  static const int kBlockSize8kHz = kTimeStepMs * 8;
  static const int kBlockSize16kHz = kTimeStepMs * 16;
  static const int kBlockSize32kHz = kTimeStepMs * 32;
  static const int kMaxBlockSize = kBlockSize32kHz;
  static const int kInitSampleRateHz = 8000;

  NetEqDecodingTest();
  virtual void SetUp();
  virtual void TearDown();
  void SelectDecoders(NetEqDecoder* used_codec);
  void LoadDecoders();
  void OpenInputFile(const std::string &rtp_file);
  void Process(NETEQTEST_RTPpacket* rtp_ptr, int* out_len);
  void DecodeAndCompare(const std::string &rtp_file,
                        const std::string &ref_file);
  void DecodeAndCheckStats(const std::string &rtp_file,
                           const std::string &stat_ref_file,
                           const std::string &rtcp_ref_file);
  static void PopulateRtpInfo(int frame_index,
                              int timestamp,
                              WebRtcRTPHeader* rtp_info);
  static void PopulateCng(int frame_index,
                          int timestamp,
                          WebRtcRTPHeader* rtp_info,
                          uint8_t* payload,
                          int* payload_len);

  NetEq* neteq_;
  FILE* rtp_fp_;
  unsigned int sim_clock_;
  int16_t out_data_[kMaxBlockSize];
  int output_sample_rate_;
};


const int NetEqDecodingTest::kTimeStepMs;
const int NetEqDecodingTest::kBlockSize8kHz;
const int NetEqDecodingTest::kBlockSize16kHz;
const int NetEqDecodingTest::kBlockSize32kHz;
const int NetEqDecodingTest::kMaxBlockSize;
const int NetEqDecodingTest::kInitSampleRateHz;

NetEqDecodingTest::NetEqDecodingTest()
    : neteq_(NULL),
      rtp_fp_(NULL),
      sim_clock_(0),
      output_sample_rate_(kInitSampleRateHz) {
  memset(out_data_, 0, sizeof(out_data_));
}

void NetEqDecodingTest::SetUp() {
  neteq_ = NetEq::Create(kInitSampleRateHz);
  ASSERT_TRUE(neteq_);
  LoadDecoders();
}

void NetEqDecodingTest::TearDown() {
  delete neteq_;
  if (rtp_fp_)
    fclose(rtp_fp_);
}

void NetEqDecodingTest::LoadDecoders() {
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderPCMu, 0));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderPCMa, 8));
#ifndef WEBRTC_ANDROID
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderILBC, 102));
#endif  
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderISAC, 103));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderISACswb, 104));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderISACfb, 105));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderPCM16B, 93));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderPCM16Bwb, 94));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderPCM16Bswb32kHz, 95));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderCNGnb, 13));
  
  ASSERT_EQ(0, neteq_->RegisterPayloadType(kDecoderCNGwb, 98));
}

void NetEqDecodingTest::OpenInputFile(const std::string &rtp_file) {
  rtp_fp_ = fopen(rtp_file.c_str(), "rb");
  ASSERT_TRUE(rtp_fp_ != NULL);
  ASSERT_EQ(0, NETEQTEST_RTPpacket::skipFileHeader(rtp_fp_));
}

void NetEqDecodingTest::Process(NETEQTEST_RTPpacket* rtp, int* out_len) {
  
  while ((sim_clock_ >= rtp->time()) &&
         (rtp->dataLen() >= 0)) {
    if (rtp->dataLen() > 0) {
      WebRtcRTPHeader rtpInfo;
      rtp->parseHeader(&rtpInfo);
      ASSERT_EQ(0, neteq_->InsertPacket(
          rtpInfo,
          rtp->payload(),
          rtp->payloadLen(),
          rtp->time() * (output_sample_rate_ / 1000)));
    }
    
    ASSERT_NE(-1, rtp->readFromFile(rtp_fp_));
  }

  
  NetEqOutputType type;
  int num_channels;
  ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, out_len,
                                &num_channels, &type));
  ASSERT_TRUE((*out_len == kBlockSize8kHz) ||
              (*out_len == kBlockSize16kHz) ||
              (*out_len == kBlockSize32kHz));
  output_sample_rate_ = *out_len / 10 * 1000;

  
  sim_clock_ += kTimeStepMs;
}

void NetEqDecodingTest::DecodeAndCompare(const std::string &rtp_file,
                                         const std::string &ref_file) {
  OpenInputFile(rtp_file);

  std::string ref_out_file = "";
  if (ref_file.empty()) {
    ref_out_file = webrtc::test::OutputPath() + "neteq_out.pcm";
  }
  RefFiles ref_files(ref_file, ref_out_file);

  NETEQTEST_RTPpacket rtp;
  ASSERT_GT(rtp.readFromFile(rtp_fp_), 0);
  int i = 0;
  while (rtp.dataLen() >= 0) {
    std::ostringstream ss;
    ss << "Lap number " << i++ << " in DecodeAndCompare while loop";
    SCOPED_TRACE(ss.str());  
    int out_len;
    ASSERT_NO_FATAL_FAILURE(Process(&rtp, &out_len));
    ASSERT_NO_FATAL_FAILURE(ref_files.ProcessReference(out_data_, out_len));
  }
}

void NetEqDecodingTest::DecodeAndCheckStats(const std::string &rtp_file,
                                            const std::string &stat_ref_file,
                                            const std::string &rtcp_ref_file) {
  OpenInputFile(rtp_file);
  std::string stat_out_file = "";
  if (stat_ref_file.empty()) {
    stat_out_file = webrtc::test::OutputPath() +
        "neteq_network_stats.dat";
  }
  RefFiles network_stat_files(stat_ref_file, stat_out_file);

  std::string rtcp_out_file = "";
  if (rtcp_ref_file.empty()) {
    rtcp_out_file = webrtc::test::OutputPath() +
        "neteq_rtcp_stats.dat";
  }
  RefFiles rtcp_stat_files(rtcp_ref_file, rtcp_out_file);

  NETEQTEST_RTPpacket rtp;
  ASSERT_GT(rtp.readFromFile(rtp_fp_), 0);
  while (rtp.dataLen() >= 0) {
    int out_len;
    Process(&rtp, &out_len);

    
    if (sim_clock_ % 1000 == 0) {
      
      NetEqNetworkStatistics network_stats;
      ASSERT_EQ(0, neteq_->NetworkStatistics(&network_stats));
      network_stat_files.ProcessReference(network_stats);

      
      RtcpStatistics rtcp_stats;
      neteq_->GetRtcpStatistics(&rtcp_stats);
      rtcp_stat_files.ProcessReference(rtcp_stats);
    }
  }
}

void NetEqDecodingTest::PopulateRtpInfo(int frame_index,
                                        int timestamp,
                                        WebRtcRTPHeader* rtp_info) {
  rtp_info->header.sequenceNumber = frame_index;
  rtp_info->header.timestamp = timestamp;
  rtp_info->header.ssrc = 0x1234;  
  rtp_info->header.payloadType = 94;  
  rtp_info->header.markerBit = 0;
}

void NetEqDecodingTest::PopulateCng(int frame_index,
                                    int timestamp,
                                    WebRtcRTPHeader* rtp_info,
                                    uint8_t* payload,
                                    int* payload_len) {
  rtp_info->header.sequenceNumber = frame_index;
  rtp_info->header.timestamp = timestamp;
  rtp_info->header.ssrc = 0x1234;  
  rtp_info->header.payloadType = 98;  
  rtp_info->header.markerBit = 0;
  payload[0] = 64;  
  *payload_len = 1;  
}

#if defined(_WIN32) && defined(WEBRTC_ARCH_64_BITS)

#define MAYBE_TestBitExactness DISABLED_TestBitExactness
#else
#define MAYBE_TestBitExactness TestBitExactness
#endif

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(MAYBE_TestBitExactness)) {
  const std::string kInputRtpFile = webrtc::test::ProjectRootPath() +
      "resources/audio_coding/neteq_universal_new.rtp";
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
  
  
  const std::string kInputRefFile = webrtc::test::ProjectRootPath() +
      "resources/audio_coding/neteq_universal_ref.pcm";
#else
  const std::string kInputRefFile =
      webrtc::test::ResourcePath("audio_coding/neteq_universal_ref", "pcm");
#endif
  DecodeAndCompare(kInputRtpFile, kInputRefFile);
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(TestNetworkStatistics)) {
  const std::string kInputRtpFile = webrtc::test::ProjectRootPath() +
      "resources/audio_coding/neteq_universal_new.rtp";
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
  
  
  const std::string kNetworkStatRefFile = webrtc::test::ProjectRootPath() +
      "resources/audio_coding/neteq_network_stats.dat";
#else
  const std::string kNetworkStatRefFile =
      webrtc::test::ResourcePath("audio_coding/neteq_network_stats", "dat");
#endif
  const std::string kRtcpStatRefFile =
      webrtc::test::ResourcePath("audio_coding/neteq_rtcp_stats", "dat");
  DecodeAndCheckStats(kInputRtpFile, kNetworkStatRefFile, kRtcpStatRefFile);
}


TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(TestFrameWaitingTimeStatistics)) {
  
  
  neteq_->SetPlayoutMode(kPlayoutFax);
  ASSERT_EQ(kPlayoutFax, neteq_->PlayoutMode());
  
  size_t num_frames = 30;
  const int kSamples = 10 * 16;
  const int kPayloadBytes = kSamples * 2;
  for (size_t i = 0; i < num_frames; ++i) {
    uint16_t payload[kSamples] = {0};
    WebRtcRTPHeader rtp_info;
    rtp_info.header.sequenceNumber = i;
    rtp_info.header.timestamp = i * kSamples;
    rtp_info.header.ssrc = 0x1234;  
    rtp_info.header.payloadType = 94;  
    rtp_info.header.markerBit = 0;
    ASSERT_EQ(0, neteq_->InsertPacket(
        rtp_info,
        reinterpret_cast<uint8_t*>(payload),
        kPayloadBytes, 0));
  }
  
  for (size_t i = 0; i < num_frames; ++i) {
    int out_len;
    int num_channels;
    NetEqOutputType type;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  std::vector<int> waiting_times;
  neteq_->WaitingTimes(&waiting_times);
  int len = waiting_times.size();
  EXPECT_EQ(num_frames, waiting_times.size());
  
  
  
  for (size_t i = 0; i < waiting_times.size(); ++i) {
    EXPECT_EQ(static_cast<int>(i + 1) * 10, waiting_times[i]);
  }

  
  neteq_->WaitingTimes(&waiting_times);
  len = waiting_times.size();
  EXPECT_EQ(0, len);

  
  
  num_frames = 110;
  for (size_t i = 0; i < num_frames; ++i) {
    uint16_t payload[kSamples] = {0};
    WebRtcRTPHeader rtp_info;
    rtp_info.header.sequenceNumber = i;
    rtp_info.header.timestamp = i * kSamples;
    rtp_info.header.ssrc = 0x1235;  
    rtp_info.header.payloadType = 94;  
    rtp_info.header.markerBit = 0;
    ASSERT_EQ(0, neteq_->InsertPacket(
        rtp_info,
        reinterpret_cast<uint8_t*>(payload),
        kPayloadBytes, 0));
    int out_len;
    int num_channels;
    NetEqOutputType type;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  neteq_->WaitingTimes(&waiting_times);
  EXPECT_EQ(100u, waiting_times.size());
}

TEST_F(NetEqDecodingTest,
       DISABLED_ON_ANDROID(TestAverageInterArrivalTimeNegative)) {
  const int kNumFrames = 3000;  
  int frame_index = 0;
  const int kSamples = 10 * 16;
  const int kPayloadBytes = kSamples * 2;
  while (frame_index < kNumFrames) {
    
    
    int num_packets = (frame_index % 10 == 0 ? 2 : 1);
    for (int n = 0; n < num_packets; ++n) {
      uint8_t payload[kPayloadBytes] = {0};
      WebRtcRTPHeader rtp_info;
      PopulateRtpInfo(frame_index, frame_index * kSamples, &rtp_info);
      ASSERT_EQ(0, neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
      ++frame_index;
    }

    
    int out_len;
    int num_channels;
    NetEqOutputType type;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  NetEqNetworkStatistics network_stats;
  ASSERT_EQ(0, neteq_->NetworkStatistics(&network_stats));
  EXPECT_EQ(-103196, network_stats.clockdrift_ppm);
}

TEST_F(NetEqDecodingTest,
       DISABLED_ON_ANDROID(TestAverageInterArrivalTimePositive)) {
  const int kNumFrames = 5000;  
  int frame_index = 0;
  const int kSamples = 10 * 16;
  const int kPayloadBytes = kSamples * 2;
  for (int i = 0; i < kNumFrames; ++i) {
    
    
    int num_packets = (i % 10 == 9 ? 0 : 1);
    for (int n = 0; n < num_packets; ++n) {
      uint8_t payload[kPayloadBytes] = {0};
      WebRtcRTPHeader rtp_info;
      PopulateRtpInfo(frame_index, frame_index * kSamples, &rtp_info);
      ASSERT_EQ(0, neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
      ++frame_index;
    }

    
    int out_len;
    int num_channels;
    NetEqOutputType type;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  NetEqNetworkStatistics network_stats;
  ASSERT_EQ(0, neteq_->NetworkStatistics(&network_stats));
  EXPECT_EQ(110946, network_stats.clockdrift_ppm);
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(LongCngWithClockDrift)) {
  uint16_t seq_no = 0;
  uint32_t timestamp = 0;
  const int kFrameSizeMs = 30;
  const int kSamples = kFrameSizeMs * 16;
  const int kPayloadBytes = kSamples * 2;
  
  const double kDriftFactor = 1000.0 / (1000.0 + 25.0);
  double next_input_time_ms = 0.0;
  double t_ms;
  NetEqOutputType type;

  
  const int kSpeechDurationMs = 5000;
  for (t_ms = 0; t_ms < kSpeechDurationMs; t_ms += 10) {
    
    while (next_input_time_ms <= t_ms) {
      
      uint8_t payload[kPayloadBytes] = {0};
      WebRtcRTPHeader rtp_info;
      PopulateRtpInfo(seq_no, timestamp, &rtp_info);
      ASSERT_EQ(0, neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
      ++seq_no;
      timestamp += kSamples;
      next_input_time_ms += static_cast<double>(kFrameSizeMs) * kDriftFactor;
    }
    
    int out_len;
    int num_channels;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  EXPECT_EQ(kOutputNormal, type);
  int32_t delay_before = timestamp - neteq_->PlayoutTimestamp();

  
  const int kCngPeriodMs = 100;
  const int kCngPeriodSamples = kCngPeriodMs * 16;  
  const int kCngDurationMs = 60000;
  for (; t_ms < kSpeechDurationMs + kCngDurationMs; t_ms += 10) {
    
    while (next_input_time_ms <= t_ms) {
      
      uint8_t payload[kPayloadBytes];
      int payload_len;
      WebRtcRTPHeader rtp_info;
      PopulateCng(seq_no, timestamp, &rtp_info, payload, &payload_len);
      ASSERT_EQ(0, neteq_->InsertPacket(rtp_info, payload, payload_len, 0));
      ++seq_no;
      timestamp += kCngPeriodSamples;
      next_input_time_ms += static_cast<double>(kCngPeriodMs) * kDriftFactor;
    }
    
    int out_len;
    int num_channels;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
  }

  EXPECT_EQ(kOutputCNG, type);

  
  while (type != kOutputNormal) {
    
    while (next_input_time_ms <= t_ms) {
      
      uint8_t payload[kPayloadBytes] = {0};
      WebRtcRTPHeader rtp_info;
      PopulateRtpInfo(seq_no, timestamp, &rtp_info);
      ASSERT_EQ(0, neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
      ++seq_no;
      timestamp += kSamples;
      next_input_time_ms += static_cast<double>(kFrameSizeMs) * kDriftFactor;
    }
    
    int out_len;
    int num_channels;
    ASSERT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_, &out_len,
                                  &num_channels, &type));
    ASSERT_EQ(kBlockSize16kHz, out_len);
    
    t_ms += 10;
  }

  int32_t delay_after = timestamp - neteq_->PlayoutTimestamp();
  
  EXPECT_LE(delay_after, delay_before + 20 * 16);
  EXPECT_GE(delay_after, delay_before - 20 * 16);
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(UnknownPayloadType)) {
  const int kPayloadBytes = 100;
  uint8_t payload[kPayloadBytes] = {0};
  WebRtcRTPHeader rtp_info;
  PopulateRtpInfo(0, 0, &rtp_info);
  rtp_info.header.payloadType = 1;  
  EXPECT_EQ(NetEq::kFail,
            neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
  EXPECT_EQ(NetEq::kUnknownRtpPayloadType, neteq_->LastError());
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(OversizePacket)) {
  
  const int kPayloadBytes = NetEq::kMaxBytesInBuffer + 1;
  uint8_t payload[kPayloadBytes] = {0};
  WebRtcRTPHeader rtp_info;
  PopulateRtpInfo(0, 0, &rtp_info);
  rtp_info.header.payloadType = 103;  
  EXPECT_EQ(NetEq::kFail,
            neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
  EXPECT_EQ(NetEq::kOversizePacket, neteq_->LastError());
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(DecoderError)) {
  const int kPayloadBytes = 100;
  uint8_t payload[kPayloadBytes] = {0};
  WebRtcRTPHeader rtp_info;
  PopulateRtpInfo(0, 0, &rtp_info);
  rtp_info.header.payloadType = 103;  
  EXPECT_EQ(0, neteq_->InsertPacket(rtp_info, payload, kPayloadBytes, 0));
  NetEqOutputType type;
  
  
  for (int i = 0; i < kMaxBlockSize; ++i) {
    out_data_[i] = 1;
  }
  int num_channels;
  int samples_per_channel;
  EXPECT_EQ(NetEq::kFail,
            neteq_->GetAudio(kMaxBlockSize, out_data_,
                             &samples_per_channel, &num_channels, &type));
  
  EXPECT_EQ(NetEq::kDecoderErrorCode, neteq_->LastError());
  
  EXPECT_EQ(6730, neteq_->LastDecoderError());
  
  
  static const int kExpectedOutputLength = 160;  
  for (int i = 0; i < kExpectedOutputLength; ++i) {
    std::ostringstream ss;
    ss << "i = " << i;
    SCOPED_TRACE(ss.str());  
    EXPECT_EQ(0, out_data_[i]);
  }
  for (int i = kExpectedOutputLength; i < kMaxBlockSize; ++i) {
    std::ostringstream ss;
    ss << "i = " << i;
    SCOPED_TRACE(ss.str());  
    EXPECT_EQ(1, out_data_[i]);
  }
}

TEST_F(NetEqDecodingTest, DISABLED_ON_ANDROID(GetAudioBeforeInsertPacket)) {
  NetEqOutputType type;
  
  
  for (int i = 0; i < kMaxBlockSize; ++i) {
    out_data_[i] = 1;
  }
  int num_channels;
  int samples_per_channel;
  EXPECT_EQ(0, neteq_->GetAudio(kMaxBlockSize, out_data_,
                                &samples_per_channel,
                                &num_channels, &type));
  
  static const int kExpectedOutputLength =
      kInitSampleRateHz / 100;  
  for (int i = 0; i < kExpectedOutputLength; ++i) {
    std::ostringstream ss;
    ss << "i = " << i;
    SCOPED_TRACE(ss.str());  
    EXPECT_EQ(0, out_data_[i]);
  }
}
}  
