









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_TEST_STEREO_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_TEST_TEST_STEREO_H_

#include <math.h>

#include "ACMTest.h"
#include "Channel.h"
#include "PCMFile.h"

namespace webrtc {

enum StereoMonoMode {
  kNotSet,
  kMono,
  kStereo
};

class TestPackStereo : public AudioPacketizationCallback {
 public:
  TestPackStereo();
  ~TestPackStereo();

  void RegisterReceiverACM(AudioCodingModule* acm);

  virtual WebRtc_Word32 SendData(const FrameType frame_type,
                                 const WebRtc_UWord8 payload_type,
                                 const WebRtc_UWord32 timestamp,
                                 const WebRtc_UWord8* payload_data,
                                 const WebRtc_UWord16 payload_size,
                                 const RTPFragmentationHeader* fragmentation);

  WebRtc_UWord16 payload_size();
  WebRtc_UWord32 timestamp_diff();
  void reset_payload_size();
  void set_codec_mode(StereoMonoMode mode);
  void set_lost_packet(bool lost);

 private:
  AudioCodingModule* receiver_acm_;
  WebRtc_Word16 seq_no_;
  WebRtc_UWord32 timestamp_diff_;
  WebRtc_UWord32 last_in_timestamp_;
  WebRtc_UWord64 total_bytes_;
  WebRtc_UWord16 payload_size_;
  StereoMonoMode codec_mode_;
  
  bool lost_packet_;
};

class TestStereo : public ACMTest {
 public:
  TestStereo(int test_mode);
  ~TestStereo();

  void Perform();
 private:
  
  
  
  void RegisterSendCodec(char side, char* codec_name,
                         WebRtc_Word32 samp_freq_hz, int rate, int pack_size,
                         int channels, int payload_type);

  void Run(TestPackStereo* channel, int in_channels, int out_channels,
           int percent_loss = 0);
  void OpenOutFile(WebRtc_Word16 test_number);
  void DisplaySendReceiveCodec();

  WebRtc_Word32 SendData(const FrameType frame_type,
                         const WebRtc_UWord8 payload_type,
                         const WebRtc_UWord32 timestamp,
                         const WebRtc_UWord8* payload_data,
                         const WebRtc_UWord16 payload_size,
                         const RTPFragmentationHeader* fragmentation);

  int test_mode_;

  AudioCodingModule* acm_a_;
  AudioCodingModule* acm_b_;

  TestPackStereo* channel_a2b_;

  PCMFile* in_file_stereo_;
  PCMFile* in_file_mono_;
  PCMFile out_file_;
  WebRtc_Word16 test_cntr_;
  WebRtc_UWord16 pack_size_samp_;
  WebRtc_UWord16 pack_size_bytes_;
  int counter_;
  char* send_codec_name_;

  
  int g722_pltype_;
  int l16_8khz_pltype_;
  int l16_16khz_pltype_;
  int l16_32khz_pltype_;
  int pcma_pltype_;
  int pcmu_pltype_;
  int celt_pltype_;
  int opus_pltype_;
  int cn_8khz_pltype_;
  int cn_16khz_pltype_;
  int cn_32khz_pltype_;
};

}  

#endif  
