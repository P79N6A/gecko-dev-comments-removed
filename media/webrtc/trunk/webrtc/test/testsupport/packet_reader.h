









#ifndef WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_PACKET_READER_H_

#include "typedefs.h"

namespace webrtc {
namespace test {


class PacketReader {
 public:
  PacketReader();
  virtual ~PacketReader();

  
  
  
  
  
  
  virtual void InitializeReading(WebRtc_UWord8* data, int data_length_in_bytes,
                                 int packet_size_in_bytes);

  
  
  
  
  
  
  virtual int NextPacket(WebRtc_UWord8** packet_pointer);

 private:
  WebRtc_UWord8* data_;
  int data_length_;
  int packet_size_;
  int currentIndex_;
  bool initialized_;
};

}  
}  

#endif  
