









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_NETEQ_QUALITY_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_TOOLS_NETEQ_QUALITY_TEST_H_

#include <gflags/gflags.h>
#include <string>
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq/tools/input_audio_file.h"
#include "webrtc/modules/audio_coding/neteq/tools/rtp_generator.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

using google::RegisterFlagValidator;

namespace webrtc {
namespace test {

class LossModel {
 public:
  virtual ~LossModel() {};
  virtual bool Lost() = 0;
};

class NoLoss : public LossModel {
 public:
  virtual bool Lost() OVERRIDE;
};

class UniformLoss : public LossModel {
 public:
  UniformLoss(double loss_rate);
  virtual bool Lost() OVERRIDE;
  void set_loss_rate(double loss_rate) { loss_rate_ = loss_rate; }

 private:
  double loss_rate_;
};

class GilbertElliotLoss : public LossModel {
 public:
  GilbertElliotLoss(double prob_trans_11, double prob_trans_01);
  virtual bool Lost() OVERRIDE;

 private:
  
  double prob_trans_11_;
  
  double prob_trans_01_;
  bool lost_last_;
  scoped_ptr<UniformLoss> uniform_loss_model_;
};

class NetEqQualityTest : public ::testing::Test {
 protected:
  NetEqQualityTest(int block_duration_ms,
                   int in_sampling_khz,
                   int out_sampling_khz,
                   enum NetEqDecoder decoder_type,
                   int channels,
                   std::string in_filename,
                   std::string out_filename);
  virtual void SetUp() OVERRIDE;
  virtual void TearDown() OVERRIDE;

  
  
  
  
  
  virtual int EncodeBlock(int16_t* in_data, int block_size_samples,
                          uint8_t* payload, int max_bytes) = 0;

  
  
  bool PacketLost();

  
  
  
  int DecodeBlock();

  
  
  int Transmit();

  
  
  
  void Simulate(int end_time_ms);

 private:
  int decoded_time_ms_;
  int decodable_time_ms_;
  double drift_factor_;
  int packet_loss_rate_;
  const int block_duration_ms_;
  const int in_sampling_khz_;
  const int out_sampling_khz_;
  const enum NetEqDecoder decoder_type_;
  const int channels_;
  const std::string in_filename_;
  const std::string out_filename_;
  const std::string log_filename_;

  
  const int in_size_samples_;

  
  const int out_size_samples_;

  int payload_size_bytes_;
  int max_payload_bytes_;

  scoped_ptr<InputAudioFile> in_file_;
  FILE* out_file_;
  FILE* log_file_;

  scoped_ptr<RtpGenerator> rtp_generator_;
  scoped_ptr<NetEq> neteq_;
  scoped_ptr<LossModel> loss_model_;

  scoped_ptr<int16_t[]> in_data_;
  scoped_ptr<uint8_t[]> payload_;
  scoped_ptr<int16_t[]> out_data_;
  WebRtcRTPHeader rtp_header_;

  long total_payload_size_bytes_;
};

}  
}  

#endif  
