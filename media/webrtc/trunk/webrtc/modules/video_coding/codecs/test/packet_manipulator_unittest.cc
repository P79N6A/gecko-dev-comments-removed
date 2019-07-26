









#include "modules/video_coding/codecs/test/packet_manipulator.h"

#include <queue>

#include "gtest/gtest.h"
#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/codecs/test/predictive_packet_manipulator.h"
#include "testsupport/unittest_utils.h"
#include "typedefs.h"

namespace webrtc {
namespace test {

const double kNeverDropProbability = 0.0;
const double kAlwaysDropProbability = 1.0;
const int kBurstLength = 1;

class PacketManipulatorTest: public PacketRelatedTest {
 protected:
  PacketReader packet_reader_;
  EncodedImage image_;
  NetworkingConfig drop_config_;
  NetworkingConfig no_drop_config_;

  PacketManipulatorTest() {
    image_._buffer = packet_data_;
    image_._length = kPacketDataLength;
    image_._size = kPacketDataLength;

    drop_config_.packet_size_in_bytes = kPacketSizeInBytes;
    drop_config_.packet_loss_probability = kAlwaysDropProbability;
    drop_config_.packet_loss_burst_length = kBurstLength;
    drop_config_.packet_loss_mode = kUniform;

    no_drop_config_.packet_size_in_bytes = kPacketSizeInBytes;
    no_drop_config_.packet_loss_probability = kNeverDropProbability;
    no_drop_config_.packet_loss_burst_length = kBurstLength;
    no_drop_config_.packet_loss_mode = kUniform;
  }

  virtual ~PacketManipulatorTest() {}

  void SetUp() {
    PacketRelatedTest::SetUp();
  }

  void TearDown() {
    PacketRelatedTest::TearDown();
  }

  void VerifyPacketLoss(int expected_nbr_packets_dropped,
                        int actual_nbr_packets_dropped,
                        int expected_packet_data_length,
                        WebRtc_UWord8* expected_packet_data,
                        EncodedImage& actual_image) {
    EXPECT_EQ(expected_nbr_packets_dropped, actual_nbr_packets_dropped);
    EXPECT_EQ(expected_packet_data_length, static_cast<int>(image_._length));
    EXPECT_EQ(0, memcmp(expected_packet_data, actual_image._buffer,
                        expected_packet_data_length));
  }
};

TEST_F(PacketManipulatorTest, Constructor) {
  PacketManipulatorImpl manipulator(&packet_reader_, no_drop_config_, false);
}

TEST_F(PacketManipulatorTest, DropNone) {
  PacketManipulatorImpl manipulator(&packet_reader_,  no_drop_config_, false);
  int nbr_packets_dropped = manipulator.ManipulatePackets(&image_);
  VerifyPacketLoss(0, nbr_packets_dropped, kPacketDataLength,
                   packet_data_, image_);
}

TEST_F(PacketManipulatorTest, UniformDropNoneSmallFrame) {
  int data_length = 400;  
  image_._length = data_length;
  PacketManipulatorImpl manipulator(&packet_reader_, no_drop_config_, false);
  int nbr_packets_dropped = manipulator.ManipulatePackets(&image_);

  VerifyPacketLoss(0, nbr_packets_dropped, data_length,
                     packet_data_, image_);
}

TEST_F(PacketManipulatorTest, UniformDropAll) {
  PacketManipulatorImpl manipulator(&packet_reader_, drop_config_, false);
  int nbr_packets_dropped = manipulator.ManipulatePackets(&image_);
  VerifyPacketLoss(kPacketDataNumberOfPackets, nbr_packets_dropped,
                   0, packet_data_, image_);
}


TEST_F(PacketManipulatorTest, UniformDropSinglePacket) {
  drop_config_.packet_loss_probability = 0.5;
  PredictivePacketManipulator manipulator(&packet_reader_, drop_config_);
  manipulator.AddRandomResult(1.0);
  manipulator.AddRandomResult(0.3);  
  manipulator.AddRandomResult(1.0);

  
  int nbr_packets_dropped = manipulator.ManipulatePackets(&image_);

  
  
  
  VerifyPacketLoss(2, nbr_packets_dropped, kPacketSizeInBytes, packet1_,
                   image_);
}


TEST_F(PacketManipulatorTest, BurstDropNinePackets) {
  
  const int kNbrPackets = 10;
  const int kDataLength = kPacketSizeInBytes * kNbrPackets;
  WebRtc_UWord8 data[kDataLength];
  WebRtc_UWord8* data_pointer = data;
  
  for (int i = 0; i < kNbrPackets; ++i) {
    memset(data_pointer + i * kPacketSizeInBytes, i, kPacketSizeInBytes);
  }
  
  image_._buffer = data;
  image_._length = kDataLength;
  image_._size = kDataLength;

  drop_config_.packet_loss_probability = 0.5;
  drop_config_.packet_loss_burst_length = 5;
  drop_config_.packet_loss_mode = kBurst;
  PredictivePacketManipulator manipulator(&packet_reader_, drop_config_);
  manipulator.AddRandomResult(1.0);
  manipulator.AddRandomResult(0.3);  
  for (int i = 0; i < kNbrPackets - 2; ++i) {
    manipulator.AddRandomResult(1.0);
  }

  
  int nbr_packets_dropped = manipulator.ManipulatePackets(&image_);

  
  VerifyPacketLoss(9, nbr_packets_dropped, kPacketSizeInBytes, data, image_);
}

}  
}  
