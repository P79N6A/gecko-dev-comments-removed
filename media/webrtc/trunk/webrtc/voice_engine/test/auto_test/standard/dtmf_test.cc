









#include "webrtc/voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

class DtmfTest : public AfterStreamingFixture {
 protected:
  void RunSixteenDtmfEvents(bool out_of_band) {
    TEST_LOG("Sending telephone events:\n");
    EXPECT_EQ(0, voe_dtmf_->SetDtmfFeedbackStatus(false));

    for (int i = 0; i < 16; i++) {
      TEST_LOG("%d ", i);
      TEST_LOG_FLUSH;
      EXPECT_EQ(0, voe_dtmf_->SendTelephoneEvent(
          channel_, i, out_of_band, 160, 10));
      Sleep(500);
    }
    TEST_LOG("\n");
  }
};

TEST_F(DtmfTest, DtmfFeedbackIsEnabledByDefaultButNotDirectFeedback) {
  bool dtmf_feedback = false;
  bool dtmf_direct_feedback = false;

  EXPECT_EQ(0, voe_dtmf_->GetDtmfFeedbackStatus(dtmf_feedback,
                                                dtmf_direct_feedback));

  EXPECT_TRUE(dtmf_feedback);
  EXPECT_FALSE(dtmf_direct_feedback);
}

TEST_F(DtmfTest, ManualSuccessfullySendsInBandTelephoneEvents) {
  RunSixteenDtmfEvents(false);
}

TEST_F(DtmfTest, ManualSuccessfullySendsOutOfBandTelephoneEvents) {
  RunSixteenDtmfEvents(true);
}

TEST_F(DtmfTest, TestTwoNonDtmfEvents) {
  EXPECT_EQ(0, voe_dtmf_->SendTelephoneEvent(channel_, 32, true));
  EXPECT_EQ(0, voe_dtmf_->SendTelephoneEvent(channel_, 110, true));
}



TEST_F(DtmfTest, ManualCanChangeDtmfPayloadType) {
  webrtc::CodecInst codec_instance = webrtc::CodecInst();

  TEST_LOG("Changing DTMF payload type.\n");

  
  for (int i = 0; i < voe_codec_->NumOfCodecs(); i++) {
    EXPECT_EQ(0, voe_codec_->GetCodec(i, codec_instance));
    if (!_stricmp("telephone-event", codec_instance.plname)) {
      codec_instance.pltype = 88;  
      EXPECT_EQ(0, voe_base_->StopSend(channel_));
      EXPECT_EQ(0, voe_base_->StopPlayout(channel_));
      EXPECT_EQ(0, voe_base_->StopReceive(channel_));
      EXPECT_EQ(0, voe_codec_->SetRecPayloadType(channel_, codec_instance));
      EXPECT_EQ(0, voe_base_->StartReceive(channel_));
      EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
      EXPECT_EQ(0, voe_base_->StartSend(channel_));
      break;
    }
  }

  Sleep(500);

  
  EXPECT_EQ(0, voe_dtmf_->SetSendTelephoneEventPayloadType(
      channel_, codec_instance.pltype));

  RunSixteenDtmfEvents(true);

  EXPECT_EQ(0, voe_dtmf_->SetDtmfFeedbackStatus(true, false));
}
