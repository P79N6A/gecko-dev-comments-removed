









#include <stdio.h>

#include "gflags/gflags.h"
#include "gtest/gtest.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/test/Channel.h"
#include "webrtc/modules/audio_coding/main/test/PCMFile.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/fileutils.h"


DEFINE_string(codec, "opus", "Codec Name");
DEFINE_int32(codec_sample_rate_hz, 48000, "Sampling rate in Hertz.");
DEFINE_int32(codec_channels, 1, "Number of channels of the codec.");


DEFINE_string(input, "", "Input PCM file at 16 kHz.");
DEFINE_bool(input_stereo, false, "Input is stereo.");
DEFINE_int32(input_fs_hz, 32000, "Input sample rate Hz.");
DEFINE_string(output, "insert_rtp_with_timing_out.pcm", "OutputFile");
DEFINE_int32(output_fs_hz, 32000, "Output sample rate Hz");


DEFINE_string(seq_num, "seq_num", "Sequence number file.");
DEFINE_string(send_ts, "send_timestamp", "Send timestamp file.");
DEFINE_string(receive_ts, "last_rec_timestamp", "Receive timestamp file");


DEFINE_string(delay, "", "Log for delay.");


DEFINE_int32(init_delay, 0, "Initial delay.");
DEFINE_bool(verbose, false, "Verbosity.");
DEFINE_double(loss_rate, 0, "Rate of packet loss < 1");

const int32_t kAudioPlayedOut = 0x00000001;
const int32_t kPacketPushedIn = 0x00000001 << 1;
const int kPlayoutPeriodMs = 10;

namespace webrtc {

class InsertPacketWithTiming {
 public:
  InsertPacketWithTiming()
      : sender_clock_(new SimulatedClock(0)),
        receiver_clock_(new SimulatedClock(0)),
        send_acm_(AudioCodingModule::Create(0, sender_clock_)),
        receive_acm_(AudioCodingModule::Create(0, receiver_clock_)),
        channel_(new Channel),
        seq_num_fid_(fopen(FLAGS_seq_num.c_str(), "rt")),
        send_ts_fid_(fopen(FLAGS_send_ts.c_str(), "rt")),
        receive_ts_fid_(fopen(FLAGS_receive_ts.c_str(), "rt")),
        pcm_out_fid_(fopen(FLAGS_output.c_str(), "wb")),
        samples_in_1ms_(48),
        num_10ms_in_codec_frame_(2),  
        time_to_insert_packet_ms_(3),  
        next_receive_ts_(0),
        time_to_playout_audio_ms_(kPlayoutPeriodMs),
        loss_threshold_(0),
        playout_timing_fid_(fopen("playout_timing.txt", "wt")) {}

  void SetUp() {
    ASSERT_TRUE(sender_clock_ != NULL);
    ASSERT_TRUE(receiver_clock_ != NULL);

    ASSERT_TRUE(send_acm_.get() != NULL);
    ASSERT_TRUE(receive_acm_.get() != NULL);
    ASSERT_TRUE(channel_ != NULL);

    ASSERT_TRUE(seq_num_fid_ != NULL);
    ASSERT_TRUE(send_ts_fid_ != NULL);
    ASSERT_TRUE(receive_ts_fid_ != NULL);

    ASSERT_TRUE(playout_timing_fid_ != NULL);

    next_receive_ts_ = ReceiveTimestamp();

    CodecInst codec;
    ASSERT_EQ(0, AudioCodingModule::Codec(FLAGS_codec.c_str(), &codec,
                             FLAGS_codec_sample_rate_hz,
                             FLAGS_codec_channels));
    ASSERT_EQ(0, receive_acm_->InitializeReceiver());
    ASSERT_EQ(0, send_acm_->RegisterSendCodec(codec));
    ASSERT_EQ(0, receive_acm_->RegisterReceiveCodec(codec));

    
    samples_in_1ms_ = codec.plfreq / 1000;
    num_10ms_in_codec_frame_ = codec.pacsize / (codec.plfreq / 100);

    channel_->RegisterReceiverACM(receive_acm_.get());
    send_acm_->RegisterTransportCallback(channel_);

    if (FLAGS_input.size() == 0) {
      std::string file_name = test::ResourcePath("audio_coding/testfile32kHz",
                                                 "pcm");
      pcm_in_fid_.Open(file_name, 32000, "r", true);  
      std::cout << "Input file " << file_name << " 32 kHz mono." << std::endl;
    } else {
      pcm_in_fid_.Open(FLAGS_input, static_cast<uint16_t>(FLAGS_input_fs_hz),
                    "r", true);  
      std::cout << "Input file " << FLAGS_input << "at " << FLAGS_input_fs_hz
          << " Hz in " << ((FLAGS_input_stereo) ? "stereo." : "mono.")
          << std::endl;
      pcm_in_fid_.ReadStereo(FLAGS_input_stereo);
    }

    ASSERT_TRUE(pcm_out_fid_ != NULL);
    std::cout << "Output file " << FLAGS_output << " at " << FLAGS_output_fs_hz
        << " Hz." << std::endl;

    
    if (FLAGS_init_delay > 0)
      EXPECT_EQ(0, receive_acm_->SetInitialPlayoutDelay(FLAGS_init_delay));

    if (FLAGS_loss_rate > 0)
      loss_threshold_ = RAND_MAX * FLAGS_loss_rate;
    else
      loss_threshold_ = 0;
  }

  void TickOneMillisecond(uint32_t* action) {
    
    time_to_insert_packet_ms_--;
    time_to_playout_audio_ms_--;
    sender_clock_->AdvanceTimeMilliseconds(1);
    receiver_clock_->AdvanceTimeMilliseconds(1);

    
    *action = 0;

    
    if (time_to_playout_audio_ms_ == 0) {
      time_to_playout_audio_ms_ = kPlayoutPeriodMs;
      receive_acm_->PlayoutData10Ms(static_cast<int>(FLAGS_output_fs_hz),
                                    &frame_);
      fwrite(frame_.data_, sizeof(frame_.data_[0]),
             frame_.samples_per_channel_ * frame_.num_channels_, pcm_out_fid_);
      *action |= kAudioPlayedOut;
    }

    
    if (time_to_insert_packet_ms_ <= .5) {
      *action |= kPacketPushedIn;

      
      uint32_t t = next_receive_ts_;
      next_receive_ts_ = ReceiveTimestamp();
      time_to_insert_packet_ms_ += static_cast<float>(next_receive_ts_ - t) /
          samples_in_1ms_;

      
      for (int n = 0; n < num_10ms_in_codec_frame_; n++) {
        pcm_in_fid_.Read10MsData(frame_);
        EXPECT_EQ(0, send_acm_->Add10MsData(frame_));
      }

      
      
      uint32_t ts = SendTimestamp();
      int seq_num = SequenceNumber();
      bool lost = false;
      channel_->set_send_timestamp(ts);
      channel_->set_sequence_number(seq_num);
      if (loss_threshold_ > 0 && rand() < loss_threshold_) {
        channel_->set_num_packets_to_drop(1);
        lost = true;
      }

      
      
      EXPECT_GT(send_acm_->Process(), 0);

      if (FLAGS_verbose) {
        if (!lost) {
          std::cout << "\nInserting packet number " << seq_num
              << " timestamp " << ts << std::endl;
        } else {
          std::cout << "\nLost packet number " << seq_num
              << " timestamp " << ts << std::endl;
        }
      }
    }
  }

  void TearDown() {
    delete channel_;

    fclose(seq_num_fid_);
    fclose(send_ts_fid_);
    fclose(receive_ts_fid_);
    fclose(pcm_out_fid_);
    pcm_in_fid_.Close();
  }

  ~InsertPacketWithTiming() {
    delete sender_clock_;
    delete receiver_clock_;
  }

  
  bool HasPackets() {
    if (feof(seq_num_fid_) || feof(send_ts_fid_) || feof(receive_ts_fid_))
      return false;
    return true;
  }

  
  void Delay(int* optimal_delay, int* current_delay) {
    ACMNetworkStatistics statistics;
    receive_acm_->NetworkStatistics(&statistics);
    *optimal_delay = statistics.preferredBufferSize;
    *current_delay = statistics.currentBufferSize;
  }

 private:
  uint32_t SendTimestamp() {
    uint32_t t;
    EXPECT_EQ(1, fscanf(send_ts_fid_, "%u\n", &t));
    return t;
  }

  uint32_t ReceiveTimestamp() {
    uint32_t t;
    EXPECT_EQ(1, fscanf(receive_ts_fid_, "%u\n", &t));
    return t;
  }

  int SequenceNumber() {
    int n;
    EXPECT_EQ(1, fscanf(seq_num_fid_, "%d\n", &n));
    return n;
  }

  
  
  SimulatedClock* sender_clock_;
  SimulatedClock* receiver_clock_;

  scoped_ptr<AudioCodingModule> send_acm_;
  scoped_ptr<AudioCodingModule> receive_acm_;
  Channel* channel_;

  FILE* seq_num_fid_;  
  FILE* send_ts_fid_;  
  FILE* receive_ts_fid_;  
  FILE* pcm_out_fid_;  

  PCMFile pcm_in_fid_;  

  int samples_in_1ms_;

  
  
  int num_10ms_in_codec_frame_;

  float time_to_insert_packet_ms_;
  uint32_t next_receive_ts_;
  uint32_t time_to_playout_audio_ms_;

  AudioFrame frame_;

  double loss_threshold_;

  
  
  FILE* playout_timing_fid_;
};

}  

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  webrtc::InsertPacketWithTiming test;
  test.SetUp();

  FILE* delay_log = NULL;
  if (FLAGS_delay.size() > 0) {
    delay_log = fopen(FLAGS_delay.c_str(), "wt");
    if (delay_log == NULL) {
      std::cout << "Cannot open the file to log delay values." << std::endl;
      exit(1);
    }
  }

  uint32_t action_taken;
  int optimal_delay_ms;
  int current_delay_ms;
  while (test.HasPackets()) {
    test.TickOneMillisecond(&action_taken);

    if (action_taken != 0) {
      test.Delay(&optimal_delay_ms, &current_delay_ms);
      if (delay_log != NULL) {
        fprintf(delay_log, "%3d %3d\n", optimal_delay_ms, current_delay_ms);
      }
    }
  }
  std::cout << std::endl;
  test.TearDown();
  if (delay_log != NULL)
    fclose(delay_log);
}
