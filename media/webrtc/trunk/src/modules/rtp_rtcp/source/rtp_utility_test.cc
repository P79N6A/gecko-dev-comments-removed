














#include "gtest/gtest.h"
#include "modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"
#include "typedefs.h"  

namespace webrtc {

using ModuleRTPUtility::RTPPayloadParser;
using ModuleRTPUtility::RTPPayload;
using ModuleRTPUtility::RTPPayloadVP8;

































void VerifyBasicHeader(const RTPPayloadVP8 &header,
                       bool N, bool S, int PartID) {
  EXPECT_EQ(N, header.nonReferenceFrame);
  EXPECT_EQ(S, header.beginningOfPartition);
  EXPECT_EQ(PartID, header.partitionID);
}

void VerifyExtensions(const RTPPayloadVP8 &header,
                      bool I, bool L, bool T, bool K) {
  EXPECT_EQ(I, header.hasPictureID);
  EXPECT_EQ(L, header.hasTl0PicIdx);
  EXPECT_EQ(T, header.hasTID);
  EXPECT_EQ(K, header.hasKeyIdx);
}

TEST(ParseVP8Test, BasicHeader) {
  WebRtc_UWord8 payload[4] = {0};
  payload[0] = 0x14;  
  payload[1] = 0x01;  

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 4, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kPFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 0 , 1 , 4 );
  VerifyExtensions(parsedPacket.info.VP8, 0 , 0 , 0 , 0 );

  EXPECT_EQ(payload + 1, parsedPacket.info.VP8.data);
  EXPECT_EQ(4 - 1, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, PictureID) {
  WebRtc_UWord8 payload[10] = {0};
  payload[0] = 0xA0;
  payload[1] = 0x80;
  payload[2] = 17;

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 10, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kPFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 1 , 0 , 0 );
  VerifyExtensions(parsedPacket.info.VP8, 1 , 0 , 0 , 0 );

  EXPECT_EQ(17, parsedPacket.info.VP8.pictureID);

  EXPECT_EQ(payload + 3, parsedPacket.info.VP8.data);
  EXPECT_EQ(10 - 3, parsedPacket.info.VP8.dataLength);


  
  payload[2] = 0x80 | 17;
  payload[3] = 17;
  RTPPayloadParser rtpPayloadParser2(kRtpVp8Video, payload, 10, 0);

  ASSERT_TRUE(rtpPayloadParser2.Parse(parsedPacket));

  VerifyBasicHeader(parsedPacket.info.VP8, 1 , 0 , 0 );
  VerifyExtensions(parsedPacket.info.VP8, 1 , 0 , 0 , 0 );

  EXPECT_EQ((17<<8) + 17, parsedPacket.info.VP8.pictureID);

  EXPECT_EQ(payload + 4, parsedPacket.info.VP8.data);
  EXPECT_EQ(10 - 4, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, Tl0PicIdx) {
  WebRtc_UWord8 payload[13] = {0};
  payload[0] = 0x90;
  payload[1] = 0x40;
  payload[2] = 17;

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 13, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kIFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 0 , 1 , 0 );
  VerifyExtensions(parsedPacket.info.VP8, 0 , 1 , 0 , 0 );

  EXPECT_EQ(17, parsedPacket.info.VP8.tl0PicIdx);

  EXPECT_EQ(payload + 3, parsedPacket.info.VP8.data);
  EXPECT_EQ(13 - 3, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, TIDAndLayerSync) {
  WebRtc_UWord8 payload[10] = {0};
  payload[0] = 0x88;
  payload[1] = 0x20;
  payload[2] = 0x80;  

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 10, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kPFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 0 , 0 , 8 );
  VerifyExtensions(parsedPacket.info.VP8, 0 , 0 , 1 , 0 );

  EXPECT_EQ(2, parsedPacket.info.VP8.tID);
  EXPECT_FALSE(parsedPacket.info.VP8.layerSync);

  EXPECT_EQ(payload + 3, parsedPacket.info.VP8.data);
  EXPECT_EQ(10 - 3, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, KeyIdx) {
  WebRtc_UWord8 payload[10] = {0};
  payload[0] = 0x88;
  payload[1] = 0x10;  
  payload[2] = 0x11;  

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 10, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kPFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 0 , 0 , 8 );
  VerifyExtensions(parsedPacket.info.VP8, 0 , 0 , 0 , 1 );

  EXPECT_EQ(17, parsedPacket.info.VP8.keyIdx);

  EXPECT_EQ(payload + 3, parsedPacket.info.VP8.data);
  EXPECT_EQ(10 - 3, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, MultipleExtensions) {
  WebRtc_UWord8 payload[10] = {0};
  payload[0] = 0x88;
  payload[1] = 0x80 | 0x40 | 0x20 | 0x10;
  payload[2] = 0x80 | 17;    
  payload[3] = 17;           
  payload[4] = 42;           
  payload[5] = 0x40 | 0x20 | 0x11;  

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 10, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kPFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8, 0 , 0 , 8 );
  VerifyExtensions(parsedPacket.info.VP8, 1 , 1 , 1 , 1 );

  EXPECT_EQ((17<<8) + 17, parsedPacket.info.VP8.pictureID);
  EXPECT_EQ(42, parsedPacket.info.VP8.tl0PicIdx);
  EXPECT_EQ(1, parsedPacket.info.VP8.tID);
  EXPECT_EQ(17, parsedPacket.info.VP8.keyIdx);

  EXPECT_EQ(payload + 6, parsedPacket.info.VP8.data);
  EXPECT_EQ(10 - 6, parsedPacket.info.VP8.dataLength);
}

TEST(ParseVP8Test, TooShortHeader) {
  WebRtc_UWord8 payload[4] = {0};
  payload[0] = 0x88;
  payload[1] = 0x80 | 0x40 | 0x20 | 0x10;  
  payload[2] = 0x80 | 17;  
  payload[3] = 17;  

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, payload, 4, 0);

  RTPPayload parsedPacket;
  EXPECT_FALSE(rtpPayloadParser.Parse(parsedPacket));
}

TEST(ParseVP8Test, TestWithPacketizer) {
  WebRtc_UWord8 payload[10] = {0};
  WebRtc_UWord8 packet[20] = {0};
  RTPVideoHeaderVP8 inputHeader;
  inputHeader.nonReference = true;
  inputHeader.pictureId = 300;
  inputHeader.temporalIdx = 1;
  inputHeader.layerSync = false;
  inputHeader.tl0PicIdx = kNoTl0PicIdx;  
  inputHeader.keyIdx = 31;
  RtpFormatVp8 packetizer(payload, 10, inputHeader, 20);
  bool last;
  int send_bytes;
  ASSERT_EQ(0, packetizer.NextPacket(packet, &send_bytes, &last));
  ASSERT_TRUE(last);

  RTPPayloadParser rtpPayloadParser(kRtpVp8Video, packet, send_bytes, 0);

  RTPPayload parsedPacket;
  ASSERT_TRUE(rtpPayloadParser.Parse(parsedPacket));

  EXPECT_EQ(ModuleRTPUtility::kIFrame, parsedPacket.frameType);
  EXPECT_EQ(kRtpVp8Video, parsedPacket.type);

  VerifyBasicHeader(parsedPacket.info.VP8,
                    inputHeader.nonReference ,
                    1 ,
                    0 );
  VerifyExtensions(parsedPacket.info.VP8,
                   1 ,
                   0 ,
                   1 ,
                   1 );

  EXPECT_EQ(inputHeader.pictureId, parsedPacket.info.VP8.pictureID);
  EXPECT_EQ(inputHeader.temporalIdx, parsedPacket.info.VP8.tID);
  EXPECT_EQ(inputHeader.layerSync, parsedPacket.info.VP8.layerSync);
  EXPECT_EQ(inputHeader.keyIdx, parsedPacket.info.VP8.keyIdx);

  EXPECT_EQ(packet + 5, parsedPacket.info.VP8.data);
  EXPECT_EQ(send_bytes - 5, parsedPacket.info.VP8.dataLength);
}

}  
