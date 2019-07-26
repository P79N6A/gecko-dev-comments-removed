









#include "after_streaming_fixture.h"
#include "voe_test_defines.h"
#include "voice_engine_defines.h"

class CodecTest : public AfterStreamingFixture {
 protected:
  void SetUp() {
    memset(&codec_instance_, 0, sizeof(codec_instance_));
  }

  void SetArbitrarySendCodec() {
    
    EXPECT_EQ(0, voe_codec_->GetCodec(0, codec_instance_));
    EXPECT_EQ(0, voe_codec_->SetSendCodec(channel_, codec_instance_));
  }

  webrtc::CodecInst codec_instance_;
};

static void SetRateIfILBC(webrtc::CodecInst* codec_instance, int packet_size) {
  if (!_stricmp(codec_instance->plname, "ilbc")) {
    if (packet_size == 160 || packet_size == 320) {
      codec_instance->rate = 15200;
    } else {
      codec_instance->rate = 13300;
    }
  }
}

static bool IsNotViableSendCodec(const char* codec_name) {
  return !_stricmp(codec_name, "CN") ||
         !_stricmp(codec_name, "telephone-event") ||
         !_stricmp(codec_name, "red");
}

TEST_F(CodecTest, PcmuIsDefaultCodecAndHasTheRightValues) {
  EXPECT_EQ(0, voe_codec_->GetSendCodec(channel_, codec_instance_));
  EXPECT_EQ(1, codec_instance_.channels);
  EXPECT_EQ(160, codec_instance_.pacsize);
  EXPECT_EQ(8000, codec_instance_.plfreq);
  EXPECT_EQ(0, codec_instance_.pltype);
  EXPECT_EQ(64000, codec_instance_.rate);
  EXPECT_STRCASEEQ("PCMU", codec_instance_.plname);
}

TEST_F(CodecTest, VoiceActivityDetectionIsOffByDefault) {
  bool vad_enabled = false;
  bool dtx_disabled = false;
  webrtc::VadModes vad_mode = webrtc::kVadAggressiveMid;

  voe_codec_->GetVADStatus(channel_, vad_enabled, vad_mode, dtx_disabled);

  EXPECT_FALSE(vad_enabled);
  EXPECT_TRUE(dtx_disabled);
  EXPECT_EQ(webrtc::kVadConventional, vad_mode);
}

TEST_F(CodecTest, VoiceActivityDetectionCanBeEnabled) {
  EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, true));

  bool vad_enabled = false;
  bool dtx_disabled = false;
  webrtc::VadModes vad_mode = webrtc::kVadAggressiveMid;

  voe_codec_->GetVADStatus(channel_, vad_enabled, vad_mode, dtx_disabled);

  EXPECT_TRUE(vad_enabled);
  EXPECT_EQ(webrtc::kVadConventional, vad_mode);
  EXPECT_FALSE(dtx_disabled);
}

TEST_F(CodecTest, VoiceActivityDetectionTypeSettingsCanBeChanged) {
  bool vad_enabled = false;
  bool dtx_disabled = false;
  webrtc::VadModes vad_mode = webrtc::kVadAggressiveMid;

  EXPECT_EQ(0, voe_codec_->SetVADStatus(
      channel_, true, webrtc::kVadAggressiveLow, false));
  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, dtx_disabled));
  EXPECT_EQ(vad_mode, webrtc::kVadAggressiveLow);
  EXPECT_FALSE(dtx_disabled);

  EXPECT_EQ(0, voe_codec_->SetVADStatus(
      channel_, true, webrtc::kVadAggressiveMid, false));
  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, dtx_disabled));
  EXPECT_EQ(vad_mode, webrtc::kVadAggressiveMid);
  EXPECT_FALSE(dtx_disabled);

  
  EXPECT_EQ(0, voe_codec_->SetVADStatus(
      channel_, true, webrtc::kVadAggressiveHigh, true));
  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, dtx_disabled));
  EXPECT_EQ(vad_mode, webrtc::kVadAggressiveHigh);
  EXPECT_TRUE(dtx_disabled);

  EXPECT_EQ(0, voe_codec_->SetVADStatus(
      channel_, true, webrtc::kVadConventional, true));
  EXPECT_EQ(0, voe_codec_->GetVADStatus(
      channel_, vad_enabled, vad_mode, dtx_disabled));
  EXPECT_EQ(vad_mode, webrtc::kVadConventional);
}

TEST_F(CodecTest, VoiceActivityDetectionCanBeTurnedOff) {
  EXPECT_EQ(0, voe_codec_->SetVADStatus(channel_, true));

  
  EXPECT_EQ(0, voe_codec_->SetVADStatus(
      channel_, false, webrtc::kVadConventional, true));

  bool vad_enabled = false;
  bool dtx_disabled = false;
  webrtc::VadModes vad_mode = webrtc::kVadAggressiveMid;

  voe_codec_->GetVADStatus(channel_, vad_enabled, vad_mode, dtx_disabled);

  EXPECT_FALSE(vad_enabled);
  EXPECT_TRUE(dtx_disabled);
  EXPECT_EQ(webrtc::kVadConventional, vad_mode);
}



TEST_F(CodecTest, ManualExtendedISACApisBehaveAsExpected) {
   strcpy(codec_instance_.plname, "isac");
   codec_instance_.pltype = 103;
   codec_instance_.plfreq = 16000;
   codec_instance_.channels = 1;
   
   codec_instance_.rate = -1;
   codec_instance_.pacsize = 480;

   EXPECT_EQ(0, voe_codec_->SetSendCodec(channel_, codec_instance_));

   EXPECT_NE(0, voe_codec_->SetISACInitTargetRate(channel_, 5000)) <<
       "iSAC should reject rate 5000.";
   EXPECT_NE(0, voe_codec_->SetISACInitTargetRate(channel_, 33000)) <<
       "iSAC should reject rate 33000.";
   EXPECT_EQ(0, voe_codec_->SetISACInitTargetRate(channel_, 32000));

   TEST_LOG("Ensure that the sound is good (iSAC, target = 32kbps)...\n");
   Sleep(3000);

   EXPECT_EQ(0, voe_codec_->SetISACInitTargetRate(channel_, 10000));
   TEST_LOG("Ensure that the sound is good (iSAC, target = 10kbps)...\n");
   Sleep(3000);

   EXPECT_EQ(0, voe_codec_->SetISACInitTargetRate(channel_, 10000, true));
   EXPECT_EQ(0, voe_codec_->SetISACInitTargetRate(channel_, 10000, false));
   EXPECT_EQ(0, voe_codec_->SetISACInitTargetRate(channel_, 0));
   TEST_LOG("Ensure that the sound is good (iSAC, target = default)...\n");
   Sleep(3000);

   TEST_LOG("  Testing SetISACMaxPayloadSize:\n");
   EXPECT_EQ(0, voe_base_->StopSend(channel_));
   EXPECT_NE(0, voe_codec_->SetISACMaxPayloadSize(channel_, 50));
   EXPECT_NE(0, voe_codec_->SetISACMaxPayloadSize(channel_, 650));
   EXPECT_EQ(0, voe_codec_->SetISACMaxPayloadSize(channel_, 120));
   EXPECT_EQ(0, voe_base_->StartSend(channel_));
   TEST_LOG("Ensure that the sound is good (iSAC, "
            "max payload size = 100 bytes)...\n");
   Sleep(3000);

   TEST_LOG("  Testing SetISACMaxRate:\n");
   EXPECT_EQ(0, voe_base_->StopSend(channel_));
   EXPECT_EQ(0, voe_codec_->SetISACMaxPayloadSize(channel_, 400));
   EXPECT_EQ(0, voe_base_->StartSend(channel_));

   EXPECT_EQ(0, voe_base_->StopSend(channel_));
   EXPECT_NE(0, voe_codec_->SetISACMaxRate(channel_, 31900));
   EXPECT_NE(0, voe_codec_->SetISACMaxRate(channel_, 53500));
   EXPECT_EQ(0, voe_codec_->SetISACMaxRate(channel_, 32000));
   EXPECT_EQ(0, voe_base_->StartSend(channel_));
   TEST_LOG("Ensure that the sound is good (iSAC, max rate = 32 kbps)...\n");
   Sleep(3000);

   EXPECT_EQ(0, voe_base_->StopSend(channel_));

   
   EXPECT_EQ(0, voe_codec_->SetISACMaxRate(channel_, 53400));
   EXPECT_EQ(0, voe_base_->StartSend(channel_));
}


TEST_F(CodecTest, DISABLED_ManualVerifySendCodecsForAllPacketSizes) {
  for (int i = 0; i < voe_codec_->NumOfCodecs(); ++i) {
    voe_codec_->GetCodec(i, codec_instance_);
    if (IsNotViableSendCodec(codec_instance_.plname)) {
      TEST_LOG("Skipping %s.\n", codec_instance_.plname);
      continue;
    }
    EXPECT_NE(-1, codec_instance_.pltype) <<
        "The codec database should suggest a payload type.";

    
    TEST_LOG("%s (pt=%d): default packet size(%d), accepts sizes ",
             codec_instance_.plname, codec_instance_.pltype,
             codec_instance_.pacsize);
    voe_codec_->SetSendCodec(channel_, codec_instance_);
    Sleep(CODEC_TEST_TIME);

    
    bool at_least_one_succeeded = false;
    for (int packet_size = 80; packet_size < 1000; packet_size += 80) {
      SetRateIfILBC(&codec_instance_, packet_size);
      codec_instance_.pacsize = packet_size;

      if (voe_codec_->SetSendCodec(channel_, codec_instance_) != -1) {
        
        
        TEST_LOG("%d ", packet_size);
        TEST_LOG_FLUSH;
        at_least_one_succeeded = true;
        Sleep(CODEC_TEST_TIME);
      }
    }
    TEST_LOG("\n");
    EXPECT_TRUE(at_least_one_succeeded);
  }
}
