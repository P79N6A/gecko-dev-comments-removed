









#include "webrtc/modules/audio_coding/main/acm2/acm_receive_test_oldapi.h"

#include <assert.h>
#include <stdio.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/neteq/tools/audio_sink.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet.h"
#include "webrtc/modules/audio_coding/neteq/tools/packet_source.h"

namespace webrtc {
namespace test {

namespace {


bool ModifyAndUseThisCodec(CodecInst* codec_param) {
  if (STR_CASE_CMP(codec_param->plname, "CN") == 0 &&
      codec_param->plfreq == 48000)
    return false;  

  if (STR_CASE_CMP(codec_param->plname, "telephone-event") == 0)
    return false;  

  return true;
}



















bool RemapPltypeAndUseThisCodec(const char* plname,
                                int plfreq,
                                int channels,
                                int* pltype) {
  if (channels != 1)
    return false;  

  
  if (STR_CASE_CMP(plname, "PCMU") == 0 && plfreq == 8000) {
    *pltype = 0;
  } else if (STR_CASE_CMP(plname, "PCMA") == 0 && plfreq == 8000) {
    *pltype = 8;
  } else if (STR_CASE_CMP(plname, "CN") == 0 && plfreq == 8000) {
    *pltype = 13;
  } else if (STR_CASE_CMP(plname, "CN") == 0 && plfreq == 16000) {
    *pltype = 98;
  } else if (STR_CASE_CMP(plname, "CN") == 0 && plfreq == 32000) {
    *pltype = 99;
  } else if (STR_CASE_CMP(plname, "ILBC") == 0) {
    *pltype = 102;
  } else if (STR_CASE_CMP(plname, "ISAC") == 0 && plfreq == 16000) {
    *pltype = 103;
  } else if (STR_CASE_CMP(plname, "ISAC") == 0 && plfreq == 32000) {
    *pltype = 104;
  } else if (STR_CASE_CMP(plname, "ISAC") == 0 && plfreq == 48000) {
    *pltype = 124;
  } else if (STR_CASE_CMP(plname, "telephone-event") == 0) {
    *pltype = 106;
  } else if (STR_CASE_CMP(plname, "red") == 0) {
    *pltype = 117;
  } else if (STR_CASE_CMP(plname, "L16") == 0 && plfreq == 8000) {
    *pltype = 93;
  } else if (STR_CASE_CMP(plname, "L16") == 0 && plfreq == 16000) {
    *pltype = 94;
  } else if (STR_CASE_CMP(plname, "L16") == 0 && plfreq == 32000) {
    *pltype = 95;
  } else if (STR_CASE_CMP(plname, "G722") == 0) {
    *pltype = 9;
  } else {
    
    return false;
  }
  return true;
}
}  

AcmReceiveTestOldApi::AcmReceiveTestOldApi(
    PacketSource* packet_source,
    AudioSink* audio_sink,
    int output_freq_hz,
    NumOutputChannels exptected_output_channels)
    : clock_(0),
      acm_(webrtc::AudioCodingModule::Create(0, &clock_)),
      packet_source_(packet_source),
      audio_sink_(audio_sink),
      output_freq_hz_(output_freq_hz),
      exptected_output_channels_(exptected_output_channels) {
}

void AcmReceiveTestOldApi::RegisterDefaultCodecs() {
  CodecInst my_codec_param;
  for (int n = 0; n < acm_->NumberOfCodecs(); n++) {
    ASSERT_EQ(0, acm_->Codec(n, &my_codec_param)) << "Failed to get codec.";
    if (ModifyAndUseThisCodec(&my_codec_param)) {
      ASSERT_EQ(0, acm_->RegisterReceiveCodec(my_codec_param))
          << "Couldn't register receive codec.\n";
    }
  }
}

void AcmReceiveTestOldApi::RegisterNetEqTestCodecs() {
  CodecInst my_codec_param;
  for (int n = 0; n < acm_->NumberOfCodecs(); n++) {
    ASSERT_EQ(0, acm_->Codec(n, &my_codec_param)) << "Failed to get codec.";
    if (!ModifyAndUseThisCodec(&my_codec_param)) {
      
      continue;
    }

    if (RemapPltypeAndUseThisCodec(my_codec_param.plname,
                                   my_codec_param.plfreq,
                                   my_codec_param.channels,
                                   &my_codec_param.pltype)) {
      ASSERT_EQ(0, acm_->RegisterReceiveCodec(my_codec_param))
          << "Couldn't register receive codec.\n";
    }
  }
}

void AcmReceiveTestOldApi::Run() {
  for (scoped_ptr<Packet> packet(packet_source_->NextPacket()); packet;
       packet.reset(packet_source_->NextPacket())) {
    
    while (clock_.TimeInMilliseconds() < packet->time_ms()) {
      AudioFrame output_frame;
      EXPECT_EQ(0, acm_->PlayoutData10Ms(output_freq_hz_, &output_frame));
      EXPECT_EQ(output_freq_hz_, output_frame.sample_rate_hz_);
      const int samples_per_block = output_freq_hz_ * 10 / 1000;
      EXPECT_EQ(samples_per_block, output_frame.samples_per_channel_);
      if (exptected_output_channels_ != kArbitraryChannels) {
        if (output_frame.speech_type_ == webrtc::AudioFrame::kPLC) {
          
          
          
        } else {
          EXPECT_EQ(exptected_output_channels_, output_frame.num_channels_);
        }
      }
      ASSERT_TRUE(audio_sink_->WriteAudioFrame(output_frame));
      clock_.AdvanceTimeMilliseconds(10);
      AfterGetAudio();
    }

    
    WebRtcRTPHeader header;
    header.header = packet->header();
    header.frameType = kAudioFrameSpeech;
    memset(&header.type.Audio, 0, sizeof(RTPAudioHeader));
    EXPECT_EQ(0,
              acm_->IncomingPacket(
                  packet->payload(),
                  static_cast<int32_t>(packet->payload_length_bytes()),
                  header))
        << "Failure when inserting packet:" << std::endl
        << "  PT = " << static_cast<int>(header.header.payloadType) << std::endl
        << "  TS = " << header.header.timestamp << std::endl
        << "  SN = " << header.header.sequenceNumber;
  }
}

AcmReceiveTestToggleOutputFreqOldApi::AcmReceiveTestToggleOutputFreqOldApi(
    PacketSource* packet_source,
    AudioSink* audio_sink,
    int output_freq_hz_1,
    int output_freq_hz_2,
    int toggle_period_ms,
    NumOutputChannels exptected_output_channels)
    : AcmReceiveTestOldApi(packet_source,
                           audio_sink,
                           output_freq_hz_1,
                           exptected_output_channels),
      output_freq_hz_1_(output_freq_hz_1),
      output_freq_hz_2_(output_freq_hz_2),
      toggle_period_ms_(toggle_period_ms),
      last_toggle_time_ms_(clock_.TimeInMilliseconds()) {
}

void AcmReceiveTestToggleOutputFreqOldApi::AfterGetAudio() {
  if (clock_.TimeInMilliseconds() >= last_toggle_time_ms_ + toggle_period_ms_) {
    output_freq_hz_ = (output_freq_hz_ == output_freq_hz_1_)
                          ? output_freq_hz_2_
                          : output_freq_hz_1_;
    last_toggle_time_ms_ = clock_.TimeInMilliseconds();
  }
}

}  
}  
