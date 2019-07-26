









#include <string.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/source/decoding_state.h"
#include "webrtc/modules/video_coding/main/source/frame_buffer.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/modules/video_coding/main/source/packet.h"

namespace webrtc {

TEST(TestDecodingState, Sanity) {
  VCMDecodingState dec_state;
  dec_state.Reset();
  EXPECT_TRUE(dec_state.in_initial_state());
  EXPECT_TRUE(dec_state.full_sync());
}

TEST(TestDecodingState, FrameContinuity) {
  VCMDecodingState dec_state;
  
  VCMFrameBuffer frame;
  VCMFrameBuffer frame_key;
  VCMPacket packet;
  packet.isFirstPacket = true;
  packet.timestamp = 1;
  packet.seqNum = 0xffff;
  packet.frameType = kVideoFrameDelta;
  packet.codecSpecificHeader.codec = kRtpVideoVp8;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0x007F;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  
  dec_state.Reset();
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  packet.frameType = kVideoFrameKey;
  EXPECT_LE(0, frame_key.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame_key));
  dec_state.SetState(&frame);
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  
  packet.isFirstPacket = false;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0x0002;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet.seqNum = 10;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));

  
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = kNoPictureId;
  frame.Reset();
  packet.seqNum = dec_state.sequence_num() - 1u;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  frame.Reset();
  packet.seqNum = dec_state.sequence_num() + 1u;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  
  packet.seqNum++;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  
  EXPECT_LE(dec_state.sequence_num(), 0xffff);
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);

  
  dec_state.Reset();
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet.seqNum = 1;
  packet.timestamp = 1;
  EXPECT_TRUE(dec_state.full_sync());
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  packet.seqNum = 2;
  packet.timestamp = 2;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 3;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 3;
  packet.seqNum = 4;
  packet.timestamp = 4;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 4;
  packet.seqNum = 5;
  packet.timestamp = 5;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  
  
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 5;
  packet.seqNum = 6;
  packet.timestamp = 6;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());

  
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0x00FF;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 6;
  packet.seqNum = 7;
  packet.timestamp = 7;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  frame.Reset();
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0x0000;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 7;
  packet.seqNum = 8;
  packet.timestamp = 8;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
}

TEST(TestDecodingState, UpdateOldPacket) {
  VCMDecodingState dec_state;
  
  
  VCMFrameBuffer frame;
  VCMPacket packet;
  packet.timestamp = 1;
  packet.seqNum = 1;
  packet.frameType = kVideoFrameDelta;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_EQ(dec_state.sequence_num(), 1);
  
  
  packet.timestamp = 2;
  dec_state.UpdateOldPacket(&packet);
  EXPECT_EQ(dec_state.sequence_num(), 1);
  
  packet.timestamp = 1;
  packet.seqNum = 2;
  packet.frameType = kFrameEmpty;
  packet.sizeBytes = 0;
  dec_state.UpdateOldPacket(&packet);
  EXPECT_EQ(dec_state.sequence_num(), 2);
  
  packet.timestamp = 1;
  packet.seqNum = 3;
  packet.frameType = kVideoFrameDelta;
  packet.sizeBytes = 1400;
  dec_state.UpdateOldPacket(&packet);
  EXPECT_EQ(dec_state.sequence_num(), 3);
  
  
  packet.timestamp = 0;
  packet.seqNum = 4;
  packet.frameType = kFrameEmpty;
  packet.sizeBytes = 0;
  dec_state.UpdateOldPacket(&packet);
  EXPECT_EQ(dec_state.sequence_num(), 3);
}

TEST(TestDecodingState, MultiLayerBehavior) {
  
  VCMDecodingState dec_state;
  
  
  
  VCMFrameBuffer frame;
  VCMPacket packet;
  packet.frameType = kVideoFrameDelta;
  packet.codecSpecificHeader.codec = kRtpVideoVp8;
  packet.timestamp = 0;
  packet.seqNum = 0;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  
  frame.Reset();
  packet.timestamp = 1;
  packet.seqNum = 1;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  
  frame.Reset();
  packet.timestamp = 3;
  packet.seqNum = 3;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 3;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 3;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  
  frame.Reset();
  packet.timestamp = 4;
  packet.seqNum = 4;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 4;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
  
  
  frame.Reset();
  packet.frameType = kVideoFrameKey;
  packet.isFirstPacket = 1;
  packet.timestamp = 5;
  packet.seqNum = 5;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 2;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 5;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.timestamp = 6;
  packet.seqNum = 6;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 3;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 6;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.isFirstPacket = 1;
  packet.timestamp = 8;
  packet.seqNum = 8;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 4;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 8;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());

  
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.isFirstPacket = 1;
  packet.timestamp = 9;
  packet.seqNum = 9;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 4;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 9;
  packet.codecSpecificHeader.codecHeader.VP8.layerSync = true;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());

  
  
  
  
  
  
  frame.Reset();
  dec_state.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.isFirstPacket = 1;
  packet.markerBit = 1;
  packet.timestamp = 0;
  packet.seqNum = 0;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  packet.codecSpecificHeader.codecHeader.VP8.layerSync = false;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());
  
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.isFirstPacket = 1;
  packet.markerBit = 0;
  packet.timestamp = 1;
  packet.seqNum = 1;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 2;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 1;
  packet.codecSpecificHeader.codecHeader.VP8.layerSync = true;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.isFirstPacket = 1;
  packet.markerBit = 1;
  packet.timestamp = 2;
  packet.seqNum = 3;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 2;
  packet.codecSpecificHeader.codecHeader.VP8.layerSync = true;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  EXPECT_TRUE(dec_state.full_sync());
}

TEST(TestDecodingState, DiscontinuousPicIdContinuousSeqNum) {
  VCMDecodingState dec_state;
  VCMFrameBuffer frame;
  VCMPacket packet;
  frame.Reset();
  packet.frameType = kVideoFrameKey;
  packet.codecSpecificHeader.codec = kRtpVideoVp8;
  packet.timestamp = 0;
  packet.seqNum = 0;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  EXPECT_TRUE(dec_state.full_sync());

  
  
  frame.Reset();
  packet.frameType = kVideoFrameDelta;
  packet.timestamp += 3000;
  ++packet.seqNum;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 1;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 2;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
  dec_state.SetState(&frame);
  EXPECT_FALSE(dec_state.full_sync());
}

TEST(TestDecodingState, OldInput) {
  VCMDecodingState dec_state;
  
  
  VCMFrameBuffer frame;
  VCMPacket packet;
  packet.timestamp = 10;
  packet.seqNum = 1;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  packet.timestamp = 9;
  EXPECT_TRUE(dec_state.IsOldPacket(&packet));
  
  frame.Reset();
  frame.InsertPacket(packet, 0, kNoErrors, frame_data);
  EXPECT_TRUE(dec_state.IsOldFrame(&frame));
}

TEST(TestDecodingState, PictureIdRepeat) {
  VCMDecodingState dec_state;
  VCMFrameBuffer frame;
  VCMPacket packet;
  packet.frameType = kVideoFrameDelta;
  packet.codecSpecificHeader.codec = kRtpVideoVp8;
  packet.timestamp = 0;
  packet.seqNum = 0;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx = 0;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId = 0;
  FrameData frame_data;
  frame_data.rtt_ms = 0;
  frame_data.rolling_average_packets_per_frame = -1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  dec_state.SetState(&frame);
  
  frame.Reset();
  ++packet.timestamp;
  ++packet.seqNum;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx++;
  packet.codecSpecificHeader.codecHeader.VP8.pictureId++;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_TRUE(dec_state.ContinuousFrame(&frame));
  frame.Reset();
  
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx += 3;
  packet.codecSpecificHeader.codecHeader.VP8.temporalIdx++;
  packet.codecSpecificHeader.codecHeader.VP8.tl0PicIdx = 1;
  EXPECT_LE(0, frame.InsertPacket(packet, 0, kNoErrors, frame_data));
  EXPECT_FALSE(dec_state.ContinuousFrame(&frame));
}

}  
