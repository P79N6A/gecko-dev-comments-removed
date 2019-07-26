









#include "webrtc/modules/audio_coding/neteq4/decoder_database.h"

#include <assert.h>
#include <stdlib.h>

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "webrtc/modules/audio_coding/neteq4/mock/mock_audio_decoder.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace webrtc {

TEST(DecoderDatabase, CreateAndDestroy) {
  DecoderDatabase db;
  EXPECT_EQ(0, db.Size());
  EXPECT_TRUE(db.Empty());
}

TEST(DecoderDatabase, InsertAndRemove) {
  DecoderDatabase db;
  const uint8_t kPayloadType = 0;
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadType, kDecoderPCMu));
  EXPECT_EQ(1, db.Size());
  EXPECT_FALSE(db.Empty());
  EXPECT_EQ(DecoderDatabase::kOK, db.Remove(kPayloadType));
  EXPECT_EQ(0, db.Size());
  EXPECT_TRUE(db.Empty());
}

TEST(DecoderDatabase, GetDecoderInfo) {
  DecoderDatabase db;
  const uint8_t kPayloadType = 0;
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadType, kDecoderPCMu));
  const DecoderDatabase::DecoderInfo* info;
  info = db.GetDecoderInfo(kPayloadType);
  ASSERT_TRUE(info != NULL);
  EXPECT_EQ(kDecoderPCMu, info->codec_type);
  EXPECT_EQ(NULL, info->decoder);
  EXPECT_EQ(8000, info->fs_hz);
  EXPECT_FALSE(info->external);
  info = db.GetDecoderInfo(kPayloadType + 1);  
  EXPECT_TRUE(info == NULL);  
}

TEST(DecoderDatabase, GetRtpPayloadType) {
  DecoderDatabase db;
  const uint8_t kPayloadType = 0;
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadType, kDecoderPCMu));
  EXPECT_EQ(kPayloadType, db.GetRtpPayloadType(kDecoderPCMu));
  const uint8_t expected_value = DecoderDatabase::kRtpPayloadTypeError;
  EXPECT_EQ(expected_value,
            db.GetRtpPayloadType(kDecoderISAC));  
}

TEST(DecoderDatabase, DISABLED_ON_ANDROID(GetDecoder)) {
  DecoderDatabase db;
  const uint8_t kPayloadType = 0;
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadType, kDecoderILBC));
  AudioDecoder* dec = db.GetDecoder(kPayloadType);
  ASSERT_TRUE(dec != NULL);
}

TEST(DecoderDatabase, TypeTests) {
  DecoderDatabase db;
  const uint8_t kPayloadTypePcmU = 0;
  const uint8_t kPayloadTypeCng = 13;
  const uint8_t kPayloadTypeDtmf = 100;
  const uint8_t kPayloadTypeRed = 101;
  const uint8_t kPayloadNotUsed = 102;
  
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadTypePcmU, kDecoderPCMu));
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadTypeCng, kDecoderCNGnb));
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadTypeDtmf, kDecoderAVT));
  EXPECT_EQ(DecoderDatabase::kOK,
            db.RegisterPayload(kPayloadTypeRed, kDecoderRED));
  EXPECT_EQ(4, db.Size());
  
  EXPECT_FALSE(db.IsComfortNoise(kPayloadNotUsed));
  EXPECT_FALSE(db.IsDtmf(kPayloadNotUsed));
  EXPECT_FALSE(db.IsRed(kPayloadNotUsed));
  EXPECT_FALSE(db.IsComfortNoise(kPayloadTypePcmU));
  EXPECT_FALSE(db.IsDtmf(kPayloadTypePcmU));
  EXPECT_FALSE(db.IsRed(kPayloadTypePcmU));
  EXPECT_FALSE(db.IsType(kPayloadTypePcmU, kDecoderISAC));
  EXPECT_TRUE(db.IsType(kPayloadTypePcmU, kDecoderPCMu));
  EXPECT_TRUE(db.IsComfortNoise(kPayloadTypeCng));
  EXPECT_TRUE(db.IsDtmf(kPayloadTypeDtmf));
  EXPECT_TRUE(db.IsRed(kPayloadTypeRed));
}

TEST(DecoderDatabase, ExternalDecoder) {
  DecoderDatabase db;
  const uint8_t kPayloadType = 0;
  MockAudioDecoder decoder;
  
  EXPECT_EQ(DecoderDatabase::kOK,
            db.InsertExternal(kPayloadType, kDecoderPCMu, 8000,
                               &decoder));
  EXPECT_EQ(1, db.Size());
  
  EXPECT_EQ(&decoder, db.GetDecoder(kPayloadType));
  
  const DecoderDatabase::DecoderInfo* info;
  info = db.GetDecoderInfo(kPayloadType);
  ASSERT_TRUE(info != NULL);
  EXPECT_EQ(kDecoderPCMu, info->codec_type);
  EXPECT_EQ(&decoder, info->decoder);
  EXPECT_EQ(8000, info->fs_hz);
  EXPECT_TRUE(info->external);
  
  
  EXPECT_CALL(decoder, Die()).Times(0);
  EXPECT_EQ(DecoderDatabase::kOK, db.Remove(kPayloadType));
  EXPECT_TRUE(db.Empty());

  EXPECT_CALL(decoder, Die()).Times(1);  
}

TEST(DecoderDatabase, CheckPayloadTypes) {
  DecoderDatabase db;
  
  
  
  const int kNumPayloads = 10;
  for (uint8_t payload_type = 0; payload_type < kNumPayloads; ++payload_type) {
    EXPECT_EQ(DecoderDatabase::kOK,
              db.RegisterPayload(payload_type, kDecoderArbitrary));
  }
  PacketList packet_list;
  for (int i = 0; i < kNumPayloads + 1; ++i) {
    
    
    Packet* packet = new Packet;
    packet->header.payloadType = i;
    packet_list.push_back(packet);
  }

  
  EXPECT_EQ(DecoderDatabase::kDecoderNotFound,
            db.CheckPayloadTypes(packet_list));

  delete packet_list.back();
  packet_list.pop_back();  

  EXPECT_EQ(DecoderDatabase::kOK, db.CheckPayloadTypes(packet_list));

  
  PacketList::iterator it = packet_list.begin();
  while (it != packet_list.end()) {
    delete packet_list.front();
    it = packet_list.erase(it);
  }
}


TEST(DecoderDatabase, ActiveDecoders) {
  DecoderDatabase db;
  
  ASSERT_EQ(DecoderDatabase::kOK, db.RegisterPayload(0, kDecoderPCMu));
  ASSERT_EQ(DecoderDatabase::kOK, db.RegisterPayload(103, kDecoderISAC));
  ASSERT_EQ(DecoderDatabase::kOK, db.RegisterPayload(13, kDecoderCNGnb));
  
  EXPECT_EQ(NULL, db.GetActiveDecoder());
  EXPECT_EQ(NULL, db.GetActiveCngDecoder());

  
  bool changed;  
  EXPECT_EQ(DecoderDatabase::kOK, db.SetActiveDecoder(0, &changed));
  EXPECT_TRUE(changed);
  AudioDecoder* decoder = db.GetActiveDecoder();
  ASSERT_FALSE(decoder == NULL);  
  EXPECT_EQ(kDecoderPCMu, decoder->codec_type());

  
  EXPECT_EQ(DecoderDatabase::kOK, db.SetActiveDecoder(0, &changed));
  EXPECT_FALSE(changed);
  decoder = db.GetActiveDecoder();
  ASSERT_FALSE(decoder == NULL);  
  EXPECT_EQ(kDecoderPCMu, decoder->codec_type());

  
  EXPECT_EQ(DecoderDatabase::kOK, db.SetActiveDecoder(103, &changed));
  EXPECT_TRUE(changed);
  decoder = db.GetActiveDecoder();
  ASSERT_FALSE(decoder == NULL);  
  EXPECT_EQ(kDecoderISAC, decoder->codec_type());

  
  EXPECT_EQ(DecoderDatabase::kOK, db.Remove(103));
  EXPECT_EQ(NULL, db.GetActiveDecoder());

  
  EXPECT_EQ(DecoderDatabase::kOK, db.SetActiveCngDecoder(13));
  decoder = db.GetActiveCngDecoder();
  ASSERT_FALSE(decoder == NULL);  
  EXPECT_EQ(kDecoderCNGnb, decoder->codec_type());

  
  EXPECT_EQ(DecoderDatabase::kOK, db.Remove(13));
  EXPECT_EQ(NULL, db.GetActiveCngDecoder());

  
  EXPECT_EQ(DecoderDatabase::kDecoderNotFound,
            db.SetActiveDecoder(17, &changed));
  EXPECT_EQ(DecoderDatabase::kDecoderNotFound,
            db.SetActiveCngDecoder(17));
}
}  
