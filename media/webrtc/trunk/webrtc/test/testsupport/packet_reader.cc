









#include "webrtc/test/testsupport/packet_reader.h"

#include <assert.h>
#include <stdio.h>

namespace webrtc {
namespace test {

PacketReader::PacketReader()
    : initialized_(false) {}

PacketReader::~PacketReader() {}

void PacketReader::InitializeReading(uint8_t* data,
                                     int data_length_in_bytes,
                                     int packet_size_in_bytes) {
  assert(data);
  assert(data_length_in_bytes >= 0);
  assert(packet_size_in_bytes > 0);
  data_ = data;
  data_length_ = data_length_in_bytes;
  packet_size_ = packet_size_in_bytes;
  currentIndex_ = 0;
  initialized_ = true;
}

int PacketReader::NextPacket(uint8_t** packet_pointer) {
  if (!initialized_) {
    fprintf(stderr, "Attempting to use uninitialized PacketReader!\n");
    return -1;
  }
  *packet_pointer = data_ + currentIndex_;
  
  if (data_length_ - currentIndex_ <= packet_size_) {
    int size = data_length_ - currentIndex_;
    currentIndex_ = data_length_;
    assert(size >= 0);
    return size;
  }
  currentIndex_ += packet_size_;
  assert(packet_size_ >= 0);
  return packet_size_;
}

}  
}  
