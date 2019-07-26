









#include <string.h>

#include "modules/video_coding/main/source/decoding_state.h"
#include "modules/video_coding/main/source/frame_buffer.h"
#include "gtest/gtest.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/source/packet.h"

namespace webrtc {


TEST(TestDecodingState, Sanity) {
  VCMDecodingState dec_state;
  dec_state.Reset();
  EXPECT_TRUE(dec_state.init());
  EXPECT_TRUE(dec_state.full_sync());
}

TEST(TestDecodingState, FrameContinuity) {
  VCMDecodingState dec_state;
  
  VCMFrameBuffer frame;
  frame.SetState(kStateEmpty);
  VCMPacket* packet = new VCMPacket();
  packet->isFirstPacket = 1;
  packet->timestamp = 1;
  packet->seqNum = 0xffff;
  packet->frameType = kVideoFrameDelta;
  packet->codecSpecificHeader.codec = kRTPVideoVP8;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0x007F;
  frame.InsertPacket(*packet, 0, false, 0);
  
  dec_state.Reset();
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  frame.Reset();
  
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0x0002;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet->seqNum = 10;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));

  
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = kNoPictureId;
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->seqNum = dec_state.sequence_num() - 1u;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->seqNum = dec_state.sequence_num() + 1u;
  frame.InsertPacket(*packet, 0, false, 0);
  
  packet->seqNum++;
  frame.InsertPacket(*packet, 0, false, 0);
  
  EXPECT_EQ(dec_state.sequence_num(), 0xffff);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);

  
  dec_state.Reset();
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet->seqNum = 1;
  packet->timestamp = 1;
  EXPECT_TRUE(dec_state.full_sync());
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  frame.SetState(kStateEmpty);
  
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  packet->seqNum = 2;
  packet->timestamp = 2;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  frame.SetState(kStateEmpty);
  
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 3;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 3;
  packet->seqNum = 4;
  packet->timestamp = 4;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 4;
  packet->seqNum = 5;
  packet->timestamp = 5;
  frame.InsertPacket(*packet, 0, false, 0);
  
  
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 5;
  packet->seqNum = 6;
  packet->timestamp = 6;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());

  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0x00FF;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 6;
  packet->seqNum = 7;
  packet->timestamp = 7;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0x0000;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 7;
  packet->seqNum = 8;
  packet->timestamp = 8;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  delete packet;
}

TEST(TestDecodingState, SetStateOneBack) {
  VCMDecodingState dec_state;
  VCMFrameBuffer frame;
  frame.SetState(kStateEmpty);
  VCMPacket* packet = new VCMPacket();
  
  packet->frameType = kVideoFrameDelta;
  packet->codecSpecificHeader.codec = kRTPVideoVP8;
  packet->timestamp = 0;
  packet->seqNum = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet->frameType = kVideoFrameDelta;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetStateOneBack(&frame);
  EXPECT_EQ(dec_state.sequence_num(), 0xFFFF);
  
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));

  
  packet->timestamp = 0;
  packet->seqNum = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = kNoPictureId;
  packet->frameType = kVideoFrameDelta;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetStateOneBack(&frame);
  
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  delete packet;
}

TEST(TestDecodingState, UpdateOldPacket) {
  VCMDecodingState dec_state;
  
  
  VCMFrameBuffer frame;
  frame.SetState(kStateEmpty);
  VCMPacket* packet = new VCMPacket();
  packet->timestamp = 1;
  packet->seqNum = 1;
  packet->frameType = kVideoFrameDelta;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_EQ(dec_state.sequence_num(), 1);
  
  
  packet->timestamp = 2;
  dec_state.UpdateOldPacket(packet);
  EXPECT_EQ(dec_state.sequence_num(), 1);
  
  packet->timestamp = 1;
  packet->seqNum = 2;
  packet->frameType = kFrameEmpty;
  packet->sizeBytes = 0;
  dec_state.UpdateOldPacket(packet);
  EXPECT_EQ(dec_state.sequence_num(), 2);
  
  packet->timestamp = 1;
  packet->seqNum = 3;
  packet->frameType = kVideoFrameDelta;
  packet->sizeBytes = 1400;
  dec_state.UpdateOldPacket(packet);
  EXPECT_EQ(dec_state.sequence_num(), 3);
  
  
  packet->timestamp = 0;
  packet->seqNum = 4;
  packet->frameType = kFrameEmpty;
  packet->sizeBytes = 0;
  dec_state.UpdateOldPacket(packet);
  EXPECT_EQ(dec_state.sequence_num(), 3);

  delete packet;
}

TEST(TestDecodingState, MultiLayerBehavior) {
  
  VCMDecodingState dec_state;
  
  
  
  VCMFrameBuffer frame;
  VCMPacket* packet = new VCMPacket();
  packet->frameType = kVideoFrameDelta;
  packet->codecSpecificHeader.codec = kRTPVideoVP8;
  frame.SetState(kStateEmpty);
  packet->timestamp = 0;
  packet->seqNum = 0;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->timestamp = 1;
  packet->seqNum = 1;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->timestamp = 3;
  packet->seqNum = 3;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 3;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 3;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->timestamp = 4;
  packet->seqNum = 4;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 4;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameKey;
  packet->isFirstPacket = 1;
  packet->timestamp = 5;
  packet->seqNum = 5;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 2;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 5;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->timestamp = 6;
  packet->seqNum = 6;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 3;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 6;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->isFirstPacket = 1;
  packet->timestamp = 8;
  packet->seqNum = 8;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 4;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 8;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());

  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->isFirstPacket = 1;
  packet->timestamp = 9;
  packet->seqNum = 9;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 4;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 9;
  packet->codecSpecificHeader.codecHeader.VP8.layerSync = true;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());

  
  
  
  
  
  
  frame.Reset();
  dec_state.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->isFirstPacket = 1;
  packet->markerBit = 1;
  packet->timestamp = 0;
  packet->seqNum = 0;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet->codecSpecificHeader.codecHeader.VP8.layerSync = false;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->isFirstPacket = 1;
  packet->markerBit = 0;
  packet->timestamp = 1;
  packet->seqNum = 1;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  packet->codecSpecificHeader.codecHeader.VP8.layerSync = true;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet->frameType = kVideoFrameDelta;
  packet->isFirstPacket = 1;
  packet->markerBit = 1;
  packet->timestamp = 2;
  packet->seqNum = 3;
  packet->codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet->codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet->codecSpecificHeader.codecHeader.VP8.pictureId = 2;
  packet->codecSpecificHeader.codecHeader.VP8.layerSync = true;
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());

  delete packet;
}

TEST(TestDecodingState, DiscontinuousPicIdContinuousSeqNum) {
  VCMDecodingState dec_state;
  VCMFrameBuffer frame;
  VCMPacket packet;
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet.frameType = kVideoFrameKey;
  packet.codecSpecificHeader.codec = kRTPVideoVP8;
  packet.timestamp = 0;
  packet.seqNum = 0;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  frame.InsertPacket(packet, 0, false, 0);
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());

  
  
  frame.Reset();
  frame.SetState(kStateEmpty);
  packet.frameType = kVideoFrameDelta;
  packet.timestamp += 3000;
  ++packet.seqNum;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 2;
  frame.InsertPacket(packet, 0, false, 0);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
}

TEST(TestDecodingState, OldInput) {
  VCMDecodingState dec_state;
  
  
  VCMFrameBuffer frame;
  frame.SetState(kStateEmpty);
  VCMPacket* packet = new VCMPacket();
  packet->timestamp = 10;
  packet->seqNum = 1;
  frame.InsertPacket(*packet, 0, false, 0);
  dec_state.SetState(&frame);
  packet->timestamp = 9;
  EXPECT_TRUE(dec_state.IsOldPacket(packet));
  
  frame.Reset();
  frame.InsertPacket(*packet, 0, false, 0);
  EXPECT_TRUE(dec_state.IsOldFrame(&frame));


  delete packet;
}

}  
