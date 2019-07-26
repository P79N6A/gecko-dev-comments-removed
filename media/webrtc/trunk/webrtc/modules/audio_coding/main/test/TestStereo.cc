









#include "webrtc/modules/audio_coding/main/test/TestStereo.h"

#include <assert.h>

#include <string>

#include "gtest/gtest.h"
#include "webrtc/common.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/test/utility.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {


TestPackStereo::TestPackStereo()
    : receiver_acm_(NULL),
      seq_no_(0),
      timestamp_diff_(0),
      last_in_timestamp_(0),
      total_bytes_(0),
      payload_size_(0),
      codec_mode_(kNotSet),
      lost_packet_(false) {
}

TestPackStereo::~TestPackStereo() {
}

void TestPackStereo::RegisterReceiverACM(AudioCodingModule* acm) {
  receiver_acm_ = acm;
  return;
}

int32_t TestPackStereo::SendData(const FrameType frame_type,
                                 const uint8_t payload_type,
                                 const uint32_t timestamp,
                                 const uint8_t* payload_data,
                                 const uint16_t payload_size,
                                 const RTPFragmentationHeader* fragmentation) {
  WebRtcRTPHeader rtp_info;
  int32_t status = 0;

  rtp_info.header.markerBit = false;
  rtp_info.header.ssrc = 0;
  rtp_info.header.sequenceNumber = seq_no_++;
  rtp_info.header.payloadType = payload_type;
  rtp_info.header.timestamp = timestamp;
  if (frame_type == kFrameEmpty) {
    
    return 0;
  }

  if (lost_packet_ == false) {
    if (frame_type != kAudioFrameCN) {
      rtp_info.type.Audio.isCNG = false;
      rtp_info.type.Audio.channel = static_cast<int>(codec_mode_);
    } else {
      rtp_info.type.Audio.isCNG = true;
      rtp_info.type.Audio.channel = static_cast<int>(kMono);
    }
    status = receiver_acm_->IncomingPacket(payload_data, payload_size,
                                           rtp_info);

    if (frame_type != kAudioFrameCN) {
      payload_size_ = payload_size;
    } else {
      payload_size_ = -1;
    }

    timestamp_diff_ = timestamp - last_in_timestamp_;
    last_in_timestamp_ = timestamp;
    total_bytes_ += payload_size;
  }
  return status;
}

uint16_t TestPackStereo::payload_size() {
  return payload_size_;
}

uint32_t TestPackStereo::timestamp_diff() {
  return timestamp_diff_;
}

void TestPackStereo::reset_payload_size() {
  payload_size_ = 0;
}

void TestPackStereo::set_codec_mode(enum StereoMonoMode mode) {
  codec_mode_ = mode;
}

void TestPackStereo::set_lost_packet(bool lost) {
  lost_packet_ = lost;
}

TestStereo::TestStereo(int test_mode, const Config& config)
    : acm_a_(config.Get<AudioCodingModuleFactory>().Create(0)),
      acm_b_(config.Get<AudioCodingModuleFactory>().Create(1)),
      channel_a2b_(NULL),
      test_cntr_(0),
      pack_size_samp_(0),
      pack_size_bytes_(0),
      counter_(0),
      g722_pltype_(0),
      l16_8khz_pltype_(-1),
      l16_16khz_pltype_(-1),
      l16_32khz_pltype_(-1),
      pcma_pltype_(-1),
      pcmu_pltype_(-1),
      celt_pltype_(-1),
      opus_pltype_(-1),
      cn_8khz_pltype_(-1),
      cn_16khz_pltype_(-1),
      cn_32khz_pltype_(-1) {
  
  test_mode_ = test_mode;
}

TestStereo::~TestStereo() {
  if (channel_a2b_ != NULL) {
    delete channel_a2b_;
    channel_a2b_ = NULL;
  }
}

void TestStereo::Perform() {
  uint16_t frequency_hz;
  int audio_channels;
  int codec_channels;
  bool dtx;
  bool vad;
  ACMVADMode vad_mode;

  
  const std::string file_name_stereo = webrtc::test::ResourcePath(
      "audio_coding/teststereo32kHz", "pcm");
  const std::string file_name_mono = webrtc::test::ResourcePath(
      "audio_coding/testfile32kHz", "pcm");
  frequency_hz = 32000;
  in_file_stereo_ = new PCMFile();
  in_file_mono_ = new PCMFile();
  in_file_stereo_->Open(file_name_stereo, frequency_hz, "rb");
  in_file_stereo_->ReadStereo(true);
  in_file_mono_->Open(file_name_mono, frequency_hz, "rb");
  in_file_mono_->ReadStereo(false);

  
  ASSERT_TRUE((acm_a_.get() != NULL) && (acm_b_.get() != NULL));
  EXPECT_EQ(0, acm_a_->InitializeReceiver());
  EXPECT_EQ(0, acm_b_->InitializeReceiver());

  
  uint8_t num_encoders = acm_a_->NumberOfCodecs();
  CodecInst my_codec_param;
  for (uint8_t n = 0; n < num_encoders; n++) {
    EXPECT_EQ(0, acm_b_->Codec(n, &my_codec_param));
    EXPECT_EQ(0, acm_b_->RegisterReceiveCodec(my_codec_param));
  }

  
  for (uint8_t n = 0; n < num_encoders; n++) {
    EXPECT_EQ(0, acm_b_->Codec(n, &my_codec_param));
    EXPECT_EQ(0, acm_b_->UnregisterReceiveCodec(my_codec_param.pltype));
  }

  
  for (uint8_t n = 0; n < num_encoders; n++) {
    EXPECT_EQ(0, acm_b_->Codec(n, &my_codec_param));
    EXPECT_EQ(0, acm_b_->RegisterReceiveCodec(my_codec_param));
  }

  
  channel_a2b_ = new TestPackStereo;
  EXPECT_EQ(0, acm_a_->RegisterTransportCallback(channel_a2b_));
  channel_a2b_->RegisterReceiverACM(acm_b_.get());

  
  
  
  EXPECT_EQ(0, acm_a_->SetVAD(true, true, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_TRUE(dtx);
  EXPECT_TRUE(vad);
  char codec_pcma_temp[] = "PCMA";
  RegisterSendCodec('A', codec_pcma_temp, 8000, 64000, 80, 2, pcma_pltype_);
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_FALSE(dtx);
  EXPECT_FALSE(vad);
  if (test_mode_ != 0) {
    printf("\n");
  }

  
  
  
  audio_channels = 2;
  codec_channels = 2;

  
  
#ifdef WEBRTC_CODEC_G722
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  channel_a2b_->set_codec_mode(kStereo);
  test_cntr_++;
  OpenOutFile(test_cntr_);
  char codec_g722[] = "G722";
  RegisterSendCodec('A', codec_g722, 16000, 64000, 160, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 320, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 480, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 640, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 800, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 960, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_PCM16
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  channel_a2b_->set_codec_mode(kStereo);
  test_cntr_++;
  OpenOutFile(test_cntr_);
  char codec_l16[] = "L16";
  RegisterSendCodec('A', codec_l16, 8000, 128000, 80, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 160, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 240, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 320, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();

  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 160, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 320, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 480, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 640, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();

  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 320, codec_channels,
      l16_32khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 640, codec_channels,
      l16_32khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#define PCMA_AND_PCMU
#ifdef PCMA_AND_PCMU
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  channel_a2b_->set_codec_mode(kStereo);
  audio_channels = 2;
  codec_channels = 2;
  test_cntr_++;
  OpenOutFile(test_cntr_);
  char codec_pcma[] = "PCMA";
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 80, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 160, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 240, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 320, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 400, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 480, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);

  
  EXPECT_EQ(-1, acm_a_->SetVAD(true, true, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_FALSE(dtx);
  EXPECT_FALSE(vad);
  EXPECT_EQ(-1, acm_a_->SetVAD(true, false, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_FALSE(dtx);
  EXPECT_FALSE(vad);
  EXPECT_EQ(-1, acm_a_->SetVAD(false, true, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_FALSE(dtx);
  EXPECT_FALSE(vad);
  EXPECT_EQ(0, acm_a_->SetVAD(false, false, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_FALSE(dtx);
  EXPECT_FALSE(vad);

  out_file_.Close();
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  char codec_pcmu[] = "PCMU";
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 80, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 160, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 240, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 320, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 400, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 480, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_CELT
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  channel_a2b_->set_codec_mode(kStereo);
  audio_channels = 2;
  codec_channels = 2;
  test_cntr_++;
  OpenOutFile(test_cntr_);
  char codec_celt[] = "CELT";
  RegisterSendCodec('A', codec_celt, 32000, 48000, 640, codec_channels,
      celt_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_celt, 32000, 64000, 640, codec_channels,
      celt_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_celt, 32000, 128000, 640, codec_channels,
      celt_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_OPUS
  if (test_mode_ != 0) {
    printf("===========================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-stereo\n");
  }
  channel_a2b_->set_codec_mode(kStereo);
  audio_channels = 2;
  codec_channels = 2;
  test_cntr_++;
  OpenOutFile(test_cntr_);

  char codec_opus[] = "opus";
  
  RegisterSendCodec('A', codec_opus, 48000, 64000, 480, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  
  RegisterSendCodec('A', codec_opus, 48000, 64000, 480*2, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  
  RegisterSendCodec('A', codec_opus, 48000, 64000, 480*4, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  
  RegisterSendCodec('A', codec_opus, 48000, 64000, 480*6, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  
  RegisterSendCodec('A', codec_opus, 48000, 40000, 960, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_opus, 48000, 510000, 960, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
  
  
  
  audio_channels = 1;
  codec_channels = 2;

#ifdef WEBRTC_CODEC_G722
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  channel_a2b_->set_codec_mode(kStereo);
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 160, codec_channels,
      g722_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_PCM16
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  channel_a2b_->set_codec_mode(kStereo);
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 80, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 160, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 320, codec_channels,
      l16_32khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef PCMA_AND_PCMU
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  channel_a2b_->set_codec_mode(kStereo);
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 80, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 80, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_CELT
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }
  test_cntr_++;
  channel_a2b_->set_codec_mode(kStereo);
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_celt, 32000, 64000, 640, codec_channels,
      celt_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_OPUS
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Mono-to-stereo\n");
  }

  
  test_cntr_++;
  channel_a2b_->set_codec_mode(kStereo);
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_opus, 48000, 64000, 960, codec_channels,
      opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);

  
  RegisterSendCodec('A', codec_opus, 48000, 64000, 960, 1, opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif

  
  
  
  audio_channels = 2;
  codec_channels = 1;
  channel_a2b_->set_codec_mode(kMono);

#ifdef WEBRTC_CODEC_G722
  
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 160, codec_channels,
      g722_pltype_);

  
  
  EXPECT_EQ(0, acm_a_->SetVAD(true, true, VADNormal));
  EXPECT_EQ(0, acm_a_->VAD(&dtx, &vad, &vad_mode));
  EXPECT_TRUE(dtx);
  EXPECT_TRUE(vad);
  EXPECT_EQ(0, acm_a_->SetVAD(false, false, VADNormal));
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_PCM16
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 80, codec_channels,
      l16_8khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 160, codec_channels,
      l16_16khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
  if (test_mode_ != 0) {
    printf("==============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 320, codec_channels,
      l16_32khz_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef PCMA_AND_PCMU
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 80, codec_channels,
                    pcmu_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 80, codec_channels,
                    pcma_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_CELT
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  RegisterSendCodec('A', codec_celt, 32000, 64000, 640, codec_channels,
      celt_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
#endif
#ifdef WEBRTC_CODEC_OPUS
  if (test_mode_ != 0) {
    printf("===============================================================\n");
    printf("Test number: %d\n", test_cntr_ + 1);
    printf("Test type: Stereo-to-mono\n");
  }
  test_cntr_++;
  OpenOutFile(test_cntr_);
  
  RegisterSendCodec('A', codec_opus, 48000, 32000, 960, codec_channels,
      opus_pltype_);
  CodecInst opus_codec_param;
  for (uint8_t n = 0; n < num_encoders; n++) {
    EXPECT_EQ(0, acm_b_->Codec(n, &opus_codec_param));
    if (!strcmp(opus_codec_param.plname, "opus")) {
      opus_codec_param.channels = 1;
      EXPECT_EQ(0, acm_b_->RegisterReceiveCodec(opus_codec_param));
      break;
    }
  }
  Run(channel_a2b_, audio_channels, codec_channels);

  
  RegisterSendCodec('A', codec_opus, 48000, 32000, 960, 2, opus_pltype_);
  Run(channel_a2b_, audio_channels, codec_channels);

  out_file_.Close();

  

  
  test_cntr_++;
  OpenOutFile(test_cntr_);
  if (test_mode_ != 0) {
    
    printf("Test number: %d\nCodec: Opus Freq: 48000 Rate :32000 PackSize: 960"
        " Decode: mono\n", test_cntr_);
  }
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();
  
  test_cntr_++;
  OpenOutFile(test_cntr_);
  if (test_mode_ != 0) {
    
    printf("Test number: %d\nCodec: Opus Freq: 48000 Rate :32000 PackSize: 960"
        " Decode: stereo\n", test_cntr_);
  }
  opus_codec_param.channels = 2;
  EXPECT_EQ(0, acm_b_->RegisterReceiveCodec(opus_codec_param));
  Run(channel_a2b_, audio_channels, 2);
  out_file_.Close();
  
  test_cntr_++;
  OpenOutFile(test_cntr_);
  if (test_mode_ != 0) {
    
    printf("Test number: %d\nCodec: Opus Freq: 48000 Rate :32000 PackSize: 960"
        " Decode: mono\n", test_cntr_);
  }
  opus_codec_param.channels = 1;
  EXPECT_EQ(0, acm_b_->RegisterReceiveCodec(opus_codec_param));
  Run(channel_a2b_, audio_channels, codec_channels);
  out_file_.Close();

#endif

  
  if (test_mode_ != 0) {
    printf("\nThe following codecs was INCLUDED in the test:\n");
#ifdef WEBRTC_CODEC_G722
    printf("   G.722\n");
#endif
#ifdef WEBRTC_CODEC_PCM16
    printf("   PCM16\n");
#endif
    printf("   G.711\n");
#ifdef WEBRTC_CODEC_CELT
    printf("   CELT\n");
#endif
#ifdef WEBRTC_CODEC_OPUS
    printf("   Opus\n");
#endif
    printf("\nTo complete the test, listen to the %d number of output "
           "files.\n",
           test_cntr_);
  }

  
  delete in_file_stereo_;
  delete in_file_mono_;
}










void TestStereo::RegisterSendCodec(char side, char* codec_name,
                                   int32_t sampling_freq_hz, int rate,
                                   int pack_size, int channels,
                                   int payload_type) {
  if (test_mode_ != 0) {
    
    printf("Codec: %s Freq: %d Rate: %d PackSize: %d\n", codec_name,
           sampling_freq_hz, rate, pack_size);
  }

  
  pack_size_samp_ = pack_size;

  
  
  
  if (!strcmp(codec_name, "CELT")) {
    pack_size_bytes_ = (uint16_t)(
        static_cast<float>(pack_size * rate) /
            static_cast<float>(sampling_freq_hz * 8) + 0.875) / channels;
  } else {
    pack_size_bytes_ = (uint16_t)(
        static_cast<float>(pack_size * rate) /
            static_cast<float>(sampling_freq_hz * 8) + 0.875);
  }

  
  AudioCodingModule* my_acm = NULL;
  switch (side) {
    case 'A': {
      my_acm = acm_a_.get();
      break;
    }
    case 'B': {
      my_acm = acm_b_.get();
      break;
    }
    default:
      break;
  }
  ASSERT_TRUE(my_acm != NULL);

  CodecInst my_codec_param;
  
  EXPECT_GT(AudioCodingModule::Codec(codec_name, &my_codec_param,
                                     sampling_freq_hz, channels), -1);
  my_codec_param.rate = rate;
  my_codec_param.pacsize = pack_size;
  EXPECT_EQ(0, my_acm->RegisterSendCodec(my_codec_param));

  send_codec_name_ = codec_name;
}

void TestStereo::Run(TestPackStereo* channel, int in_channels, int out_channels,
                     int percent_loss) {
  AudioFrame audio_frame;

  int32_t out_freq_hz_b = out_file_.SamplingFrequency();
  uint16_t rec_size;
  uint32_t time_stamp_diff;
  channel->reset_payload_size();
  int error_count = 0;

  while (1) {
    
    
    if (percent_loss > 0) {
      if (counter_ == floor((100 / percent_loss) + 0.5)) {
        counter_ = 0;
        channel->set_lost_packet(true);
      } else {
        channel->set_lost_packet(false);
      }
      counter_++;
    }

    
    if (in_channels == 1) {
      if (in_file_mono_->EndOfFile()) {
        break;
      }
      in_file_mono_->Read10MsData(audio_frame);
    } else {
      if (in_file_stereo_->EndOfFile()) {
        break;
      }
      in_file_stereo_->Read10MsData(audio_frame);
    }
    EXPECT_EQ(0, acm_a_->Add10MsData(audio_frame));

    
    EXPECT_GT(acm_a_->Process(), -1);

    
    rec_size = channel->payload_size();
    if ((0 < rec_size) & (rec_size < 65535)) {
      
      if (strcmp(send_codec_name_, "opus")) {
        if ((rec_size != pack_size_bytes_ * out_channels)
            && (pack_size_bytes_ < 65535)) {
          error_count++;
        }
      }
      
      time_stamp_diff = channel->timestamp_diff();
      if ((counter_ > 10) && (time_stamp_diff != pack_size_samp_)) {
        error_count++;
      }
    }

    
    EXPECT_EQ(0, acm_b_->PlayoutData10Ms(out_freq_hz_b, &audio_frame));

    
    out_file_.Write10MsData(
        audio_frame.data_,
        audio_frame.samples_per_channel_ * audio_frame.num_channels_);
  }

  EXPECT_EQ(0, error_count);

  if (in_file_mono_->EndOfFile()) {
    in_file_mono_->Rewind();
  }
  if (in_file_stereo_->EndOfFile()) {
    in_file_stereo_->Rewind();
  }
  
  channel->set_lost_packet(false);
}

void TestStereo::OpenOutFile(int16_t test_number) {
  std::string file_name;
  std::stringstream file_stream;
  file_stream << webrtc::test::OutputPath() << "teststereo_out_" << test_number
      << ".pcm";
  file_name = file_stream.str();
  out_file_.Open(file_name, 32000, "wb");
}

void TestStereo::DisplaySendReceiveCodec() {
  CodecInst my_codec_param;
  acm_a_->SendCodec(&my_codec_param);
  if (test_mode_ != 0) {
    printf("%s -> ", my_codec_param.plname);
  }
  acm_b_->ReceiveCodec(&my_codec_param);
  if (test_mode_ != 0) {
    printf("%s\n", my_codec_param.plname);
  }
}

}  
