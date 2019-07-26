









#ifndef WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class PacketReader {
 public:
  PacketReader();
  virtual ~PacketReader();

  
  
  
  
  
  
  virtual void InitializeReading(uint8_t* data, int data_length_in_bytes,
                                 int packet_size_in_bytes);

  
  
  
  
  
  
  virtual int NextPacket(uint8_t** packet_pointer);

 private:
  uint8_t* data_;
  int data_length_;
  int packet_size_;
  int currentIndex_;
  bool initialized_;
};

}  
}  

#endif  
