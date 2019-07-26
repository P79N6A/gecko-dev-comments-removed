









#ifndef WEBRTC_TEST_TESTSUPPORT_UNITTEST_UTILS_H_
#define WEBRTC_TEST_TESTSUPPORT_UNITTEST_UTILS_H_

namespace webrtc {
namespace test {

const int kPacketSizeInBytes = 1500;
const int kPacketDataLength = kPacketSizeInBytes * 2 + 1;
const int kPacketDataNumberOfPackets = 3;






class PacketRelatedTest: public testing::Test {
 protected:
  
  uint8_t packet1_[kPacketSizeInBytes];
  uint8_t packet2_[kPacketSizeInBytes];
  uint8_t packet3_[1];
  
  uint8_t packet_data_[kPacketDataLength];
  uint8_t* packet_data_pointer_;

  PacketRelatedTest() {
    packet_data_pointer_ = packet_data_;

    memset(packet1_, 1, kPacketSizeInBytes);
    memset(packet2_, 2, kPacketSizeInBytes);
    memset(packet3_, 3, 1);
    
    memcpy(packet_data_pointer_, packet1_, kPacketSizeInBytes);
    memcpy(packet_data_pointer_ + kPacketSizeInBytes, packet2_,
           kPacketSizeInBytes);
    memcpy(packet_data_pointer_ + kPacketSizeInBytes * 2, packet3_, 1);
  }
  virtual ~PacketRelatedTest() {}
  void SetUp() {}
  void TearDown() {}
};

}  
}  

#endif  
