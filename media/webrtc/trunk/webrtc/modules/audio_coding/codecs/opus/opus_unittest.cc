








#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "webrtc/modules/audio_coding/codecs/opus/opus_inst.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {


const int kOpusMaxFrameSamples = 48 * 60 * 2;

const size_t kMaxBytes = 1000;

const int kOpus20msFrameSamples = 48 * 20;

const int kOpus10msFrameSamples = 48 * 10;

class OpusTest : public ::testing::Test {
 protected:
  OpusTest();
  virtual void SetUp();

  void TestSetMaxPlaybackRate(opus_int32 expect, int32_t set);

  WebRtcOpusEncInst* opus_mono_encoder_;
  WebRtcOpusEncInst* opus_stereo_encoder_;
  WebRtcOpusDecInst* opus_mono_decoder_;
  WebRtcOpusDecInst* opus_mono_decoder_new_;
  WebRtcOpusDecInst* opus_stereo_decoder_;
  WebRtcOpusDecInst* opus_stereo_decoder_new_;

  int16_t speech_data_[kOpusMaxFrameSamples];
  int16_t output_data_[kOpusMaxFrameSamples];
  uint8_t bitstream_[kMaxBytes];
};

OpusTest::OpusTest()
    : opus_mono_encoder_(NULL),
      opus_stereo_encoder_(NULL),
      opus_mono_decoder_(NULL),
      opus_mono_decoder_new_(NULL),
      opus_stereo_decoder_(NULL),
      opus_stereo_decoder_new_(NULL) {
}

void OpusTest::SetUp() {
  FILE* input_file;
  const std::string file_name =
        webrtc::test::ResourcePath("audio_coding/speech_mono_32_48kHz", "pcm");
  input_file = fopen(file_name.c_str(), "rb");
  ASSERT_TRUE(input_file != NULL);
  ASSERT_EQ(kOpusMaxFrameSamples,
            static_cast<int32_t>(fread(speech_data_, sizeof(int16_t),
                                       kOpusMaxFrameSamples, input_file)));
  fclose(input_file);
  input_file = NULL;
}

void OpusTest::TestSetMaxPlaybackRate(opus_int32 expect, int32_t set) {
  opus_int32 bandwidth;
  
  EXPECT_EQ(0, WebRtcOpus_SetMaxPlaybackRate(opus_mono_encoder_, set));
  opus_encoder_ctl(opus_mono_encoder_->encoder,
                   OPUS_GET_MAX_BANDWIDTH(&bandwidth));
  EXPECT_EQ(expect, bandwidth);
  
  EXPECT_EQ(0, WebRtcOpus_SetMaxPlaybackRate(opus_stereo_encoder_, set));
  opus_encoder_ctl(opus_stereo_encoder_->encoder,
                   OPUS_GET_MAX_BANDWIDTH(&bandwidth));
  EXPECT_EQ(expect, bandwidth);
}


TEST_F(OpusTest, OpusCreateFail) {
  
  EXPECT_EQ(-1, WebRtcOpus_EncoderCreate(NULL, 1));
  EXPECT_EQ(-1, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 3));
  EXPECT_EQ(-1, WebRtcOpus_DecoderCreate(NULL, 1));
  EXPECT_EQ(-1, WebRtcOpus_DecoderCreate(&opus_mono_decoder_, 3));
}


TEST_F(OpusTest, OpusFreeFail) {
  
  EXPECT_EQ(-1, WebRtcOpus_EncoderFree(NULL));
  EXPECT_EQ(-1, WebRtcOpus_DecoderFree(NULL));
}


TEST_F(OpusTest, OpusCreateFree) {
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_mono_decoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_, 2));
  EXPECT_TRUE(opus_mono_encoder_ != NULL);
  EXPECT_TRUE(opus_mono_decoder_ != NULL);
  EXPECT_TRUE(opus_stereo_encoder_ != NULL);
  EXPECT_TRUE(opus_stereo_decoder_ != NULL);
  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_mono_decoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_));
}

TEST_F(OpusTest, OpusEncodeDecodeMono) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_mono_decoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_mono_decoder_new_, 1));

  
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_mono_encoder_, 32000));

  
  EXPECT_EQ(1, WebRtcOpus_DecoderChannels(opus_mono_decoder_));
  EXPECT_EQ(1, WebRtcOpus_DecoderChannels(opus_mono_decoder_new_));

  
  int16_t encoded_bytes;
  int16_t audio_type;
  int16_t output_data_decode_new[kOpusMaxFrameSamples];
  int16_t output_data_decode[kOpusMaxFrameSamples];
  encoded_bytes = WebRtcOpus_Encode(opus_mono_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_mono_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_mono_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));

  
  
  for (int i = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(output_data_decode_new[i], output_data_decode[i]);
  }

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_mono_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_mono_decoder_new_));
}

TEST_F(OpusTest, OpusEncodeDecodeStereo) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_new_, 2));

  
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_stereo_encoder_, 64000));

  
  EXPECT_EQ(2, WebRtcOpus_DecoderChannels(opus_stereo_decoder_));
  EXPECT_EQ(2, WebRtcOpus_DecoderChannels(opus_stereo_decoder_new_));

  
  int16_t encoded_bytes;
  int16_t audio_type;
  int16_t output_data_decode_new[kOpusMaxFrameSamples];
  int16_t output_data_decode[kOpusMaxFrameSamples];
  int16_t output_data_decode_slave[kOpusMaxFrameSamples];
  encoded_bytes = WebRtcOpus_Encode(opus_stereo_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_stereo_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_stereo_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeSlave(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes, output_data_decode_slave,
                                   &audio_type));

  
  
  
  for (int i = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(output_data_decode_new[i * 2], output_data_decode[i]);
    EXPECT_EQ(output_data_decode_new[i * 2 + 1], output_data_decode_slave[i]);
  }

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_new_));
}

TEST_F(OpusTest, OpusSetBitRate) {
  
  EXPECT_EQ(-1, WebRtcOpus_SetBitRate(opus_mono_encoder_, 60000));
  EXPECT_EQ(-1, WebRtcOpus_SetBitRate(opus_stereo_encoder_, 60000));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_mono_encoder_, 30000));
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_stereo_encoder_, 60000));
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_mono_encoder_, 300000));
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_stereo_encoder_, 600000));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
}

TEST_F(OpusTest, OpusSetComplexity) {
  
  EXPECT_EQ(-1, WebRtcOpus_SetComplexity(opus_mono_encoder_, 9));
  EXPECT_EQ(-1, WebRtcOpus_SetComplexity(opus_stereo_encoder_, 9));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));

  EXPECT_EQ(0, WebRtcOpus_SetComplexity(opus_mono_encoder_, 0));
  EXPECT_EQ(0, WebRtcOpus_SetComplexity(opus_stereo_encoder_, 0));
  EXPECT_EQ(0, WebRtcOpus_SetComplexity(opus_mono_encoder_, 10));
  EXPECT_EQ(0, WebRtcOpus_SetComplexity(opus_stereo_encoder_, 10));
  EXPECT_EQ(-1, WebRtcOpus_SetComplexity(opus_mono_encoder_, 11));
  EXPECT_EQ(-1, WebRtcOpus_SetComplexity(opus_stereo_encoder_, 11));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
}



TEST_F(OpusTest, OpusDecodeInit) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_new_, 2));

  
  int16_t encoded_bytes;
  int16_t audio_type;
  int16_t output_data_decode_new[kOpusMaxFrameSamples];
  int16_t output_data_decode[kOpusMaxFrameSamples];
  int16_t output_data_decode_slave[kOpusMaxFrameSamples];
  encoded_bytes = WebRtcOpus_Encode(opus_stereo_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_stereo_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_stereo_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeSlave(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes, output_data_decode_slave,
                                   &audio_type));

  
  
  
  for (int i = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(output_data_decode_new[i * 2], output_data_decode[i]);
    EXPECT_EQ(output_data_decode_new[i * 2 + 1], output_data_decode_slave[i]);
  }

  EXPECT_EQ(0, WebRtcOpus_DecoderInitNew(opus_stereo_decoder_new_));
  EXPECT_EQ(0, WebRtcOpus_DecoderInit(opus_stereo_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderInitSlave(opus_stereo_decoder_));

  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_stereo_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_stereo_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeSlave(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes, output_data_decode_slave,
                                   &audio_type));

  
  
  
  for (int i = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(output_data_decode_new[i * 2], output_data_decode[i]);
    EXPECT_EQ(output_data_decode_new[i * 2 + 1], output_data_decode_slave[i]);
  }

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_new_));
}

TEST_F(OpusTest, OpusEnableDisableFec) {
  
  EXPECT_EQ(-1, WebRtcOpus_EnableFec(opus_mono_encoder_));
  EXPECT_EQ(-1, WebRtcOpus_DisableFec(opus_stereo_encoder_));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));

  EXPECT_EQ(0, WebRtcOpus_EnableFec(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EnableFec(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DisableFec(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DisableFec(opus_stereo_encoder_));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
}

TEST_F(OpusTest, OpusSetPacketLossRate) {
  
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_mono_encoder_, 50));
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_stereo_encoder_, 50));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));

  EXPECT_EQ(0, WebRtcOpus_SetPacketLossRate(opus_mono_encoder_, 50));
  EXPECT_EQ(0, WebRtcOpus_SetPacketLossRate(opus_stereo_encoder_, 50));
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_mono_encoder_, -1));
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_stereo_encoder_, -1));
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_mono_encoder_, 101));
  EXPECT_EQ(-1, WebRtcOpus_SetPacketLossRate(opus_stereo_encoder_, 101));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
}

TEST_F(OpusTest, OpusSetMaxPlaybackRate) {
  
  EXPECT_EQ(-1, WebRtcOpus_SetMaxPlaybackRate(opus_mono_encoder_, 20000));
  EXPECT_EQ(-1, WebRtcOpus_SetMaxPlaybackRate(opus_stereo_encoder_, 20000));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));

  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_FULLBAND, 48000);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_FULLBAND, 24001);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_SUPERWIDEBAND, 24000);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_SUPERWIDEBAND, 16001);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_WIDEBAND, 16000);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_WIDEBAND, 12001);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_MEDIUMBAND, 12000);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_MEDIUMBAND, 8001);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_NARROWBAND, 8000);
  TestSetMaxPlaybackRate(OPUS_BANDWIDTH_NARROWBAND, 4000);

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
}


TEST_F(OpusTest, OpusDecodePlcMono) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_mono_encoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_mono_decoder_, 1));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_mono_decoder_new_, 1));

  
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_mono_encoder_, 32000));

  
  EXPECT_EQ(1, WebRtcOpus_DecoderChannels(opus_mono_decoder_));
  EXPECT_EQ(1, WebRtcOpus_DecoderChannels(opus_mono_decoder_new_));

  
  int16_t encoded_bytes;
  int16_t audio_type;
  int16_t output_data_decode_new[kOpusMaxFrameSamples];
  int16_t output_data_decode[kOpusMaxFrameSamples];
  encoded_bytes = WebRtcOpus_Encode(opus_mono_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_mono_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_mono_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));

  
  int16_t plc_buffer[kOpusMaxFrameSamples];
  int16_t plc_buffer_new[kOpusMaxFrameSamples];
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodePlcMaster(opus_mono_decoder_, plc_buffer, 1));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodePlc(opus_mono_decoder_new_, plc_buffer_new, 1));

  
  for (int i = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(plc_buffer[i], plc_buffer_new[i]);
  }

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_mono_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_mono_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_mono_decoder_new_));
}


TEST_F(OpusTest, OpusDecodePlcStereo) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_new_, 2));

  
  EXPECT_EQ(0, WebRtcOpus_SetBitRate(opus_stereo_encoder_, 64000));

  
  EXPECT_EQ(2, WebRtcOpus_DecoderChannels(opus_stereo_decoder_));
  EXPECT_EQ(2, WebRtcOpus_DecoderChannels(opus_stereo_decoder_new_));

  
  int16_t encoded_bytes;
  int16_t audio_type;
  int16_t output_data_decode_new[kOpusMaxFrameSamples];
  int16_t output_data_decode[kOpusMaxFrameSamples];
  int16_t output_data_decode_slave[kOpusMaxFrameSamples];
  encoded_bytes = WebRtcOpus_Encode(opus_stereo_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeNew(opus_stereo_decoder_new_, bitstream_,
                                 encoded_bytes, output_data_decode_new,
                                 &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_Decode(opus_stereo_decoder_, bitstream_,
                              encoded_bytes, output_data_decode,
                              &audio_type));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodeSlave(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes,
                                   output_data_decode_slave,
                                   &audio_type));

  
  int16_t plc_buffer_left[kOpusMaxFrameSamples];
  int16_t plc_buffer_right[kOpusMaxFrameSamples];
  int16_t plc_buffer_new[kOpusMaxFrameSamples];
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodePlcMaster(opus_stereo_decoder_,
                                       plc_buffer_left, 1));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodePlcSlave(opus_stereo_decoder_,
                                      plc_buffer_right, 1));
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DecodePlc(opus_stereo_decoder_new_, plc_buffer_new, 1));
  
  
  for (int i = 0, j = 0; i < kOpus20msFrameSamples; i++) {
    EXPECT_EQ(plc_buffer_left[i], plc_buffer_new[j++]);
    EXPECT_EQ(plc_buffer_right[i], plc_buffer_new[j++]);
  }

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_new_));
}


TEST_F(OpusTest, OpusDurationEstimation) {
  
  EXPECT_EQ(0, WebRtcOpus_EncoderCreate(&opus_stereo_encoder_, 2));
  EXPECT_EQ(0, WebRtcOpus_DecoderCreate(&opus_stereo_decoder_, 2));

  int16_t encoded_bytes;

  
  encoded_bytes = WebRtcOpus_Encode(opus_stereo_encoder_, speech_data_,
                                    kOpus10msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus10msFrameSamples,
            WebRtcOpus_DurationEst(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes));

  
  encoded_bytes = WebRtcOpus_Encode(opus_stereo_encoder_, speech_data_,
                                    kOpus20msFrameSamples, kMaxBytes,
                                    bitstream_);
  EXPECT_EQ(kOpus20msFrameSamples,
            WebRtcOpus_DurationEst(opus_stereo_decoder_, bitstream_,
                                   encoded_bytes));

  
  EXPECT_EQ(0, WebRtcOpus_EncoderFree(opus_stereo_encoder_));
  EXPECT_EQ(0, WebRtcOpus_DecoderFree(opus_stereo_decoder_));
}

}  
