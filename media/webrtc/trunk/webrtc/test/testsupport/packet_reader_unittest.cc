









#include "testsupport/packet_reader.h"

#include "gtest/gtest.h"
#include "testsupport/unittest_utils.h"

namespace webrtc {
namespace test {

class PacketReaderTest: public PacketRelatedTest {
 protected:
  PacketReaderTest() {}
  virtual ~PacketReaderTest() {}
  void SetUp() {
    reader_ = new PacketReader();
  }
  void TearDown() {
    delete reader_;
  }
  void VerifyPacketData(int expected_length,
                        int actual_length,
                        WebRtc_UWord8* original_data_pointer,
                        WebRtc_UWord8* new_data_pointer) {
    EXPECT_EQ(expected_length, actual_length);
    EXPECT_EQ(*original_data_pointer, *new_data_pointer);
    EXPECT_EQ(0, memcmp(original_data_pointer, new_data_pointer,
                        actual_length));
  }
  PacketReader* reader_;
};


TEST_F(PacketReaderTest, Uninitialized) {
  WebRtc_UWord8* data_pointer = NULL;
  EXPECT_EQ(-1, reader_->NextPacket(&data_pointer));
  EXPECT_EQ(NULL, data_pointer);
}

TEST_F(PacketReaderTest, InitializeZeroLengthArgument) {
  reader_->InitializeReading(packet_data_, 0, kPacketSizeInBytes);
  ASSERT_EQ(0, reader_->NextPacket(&packet_data_pointer_));
}


TEST_F(PacketReaderTest, NormalSmallData) {
  const int kDataLengthInBytes = 1499;
  WebRtc_UWord8 data[kDataLengthInBytes];
  WebRtc_UWord8* data_pointer = data;
  memset(data, 1, kDataLengthInBytes);

  reader_->InitializeReading(data, kDataLengthInBytes, kPacketSizeInBytes);
  int length_to_read = reader_->NextPacket(&data_pointer);
  VerifyPacketData(kDataLengthInBytes, length_to_read, data, data_pointer);
  EXPECT_EQ(0, data_pointer - data);  

  
  length_to_read = reader_->NextPacket(&data_pointer);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kDataLengthInBytes, data_pointer - data);
}


TEST_F(PacketReaderTest, NormalOnePacketData) {
  WebRtc_UWord8 data[kPacketSizeInBytes];
  WebRtc_UWord8* data_pointer = data;
  memset(data, 1, kPacketSizeInBytes);

  reader_->InitializeReading(data, kPacketSizeInBytes, kPacketSizeInBytes);
  int length_to_read = reader_->NextPacket(&data_pointer);
  VerifyPacketData(kPacketSizeInBytes, length_to_read, data, data_pointer);
  EXPECT_EQ(0, data_pointer - data);  

  
  length_to_read = reader_->NextPacket(&data_pointer);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kPacketSizeInBytes, data_pointer - data);
}


TEST_F(PacketReaderTest, NormalLargeData) {
  reader_->InitializeReading(packet_data_, kPacketDataLength,
                             kPacketSizeInBytes);

  int length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(kPacketSizeInBytes, length_to_read,
                   packet1_, packet_data_pointer_);

  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(kPacketSizeInBytes, length_to_read,
                   packet2_, packet_data_pointer_);

  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  VerifyPacketData(1u, length_to_read,
                   packet3_, packet_data_pointer_);

  
  length_to_read = reader_->NextPacket(&packet_data_pointer_);
  EXPECT_EQ(0, length_to_read);
  EXPECT_EQ(kPacketDataLength, packet_data_pointer_ - packet_data_);
}


TEST_F(PacketReaderTest, EmptyData) {
  const int kDataLengthInBytes = 0;
  
  WebRtc_UWord8 data[kPacketSizeInBytes];
  WebRtc_UWord8* data_pointer = data;
  reader_->InitializeReading(data, kDataLengthInBytes, kPacketSizeInBytes);
  EXPECT_EQ(kDataLengthInBytes, reader_->NextPacket(&data_pointer));
  
  EXPECT_EQ(kDataLengthInBytes, reader_->NextPacket(&data_pointer));
}

}  
}  
