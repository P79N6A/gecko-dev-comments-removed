









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_PACKET_MANIPULATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_PACKET_MANIPULATOR_H_

#include <cstdlib>

#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "testsupport/packet_reader.h"

namespace webrtc {
namespace test {


enum PacketLossMode {
  
  kUniform,
  
  
  
  kBurst
};

const char* PacketLossModeToStr(PacketLossMode e);



struct NetworkingConfig {
  NetworkingConfig()
  : packet_size_in_bytes(1500), max_payload_size_in_bytes(1440),
    packet_loss_mode(kUniform), packet_loss_probability(0.0),
    packet_loss_burst_length(1) {
  }

  
  int packet_size_in_bytes;

  
  
  int max_payload_size_in_bytes;

  
  
  
  
  PacketLossMode packet_loss_mode;

  
  
  
  double packet_loss_probability;

  
  
  int packet_loss_burst_length;
};










class PacketManipulator {
 public:
  virtual ~PacketManipulator() {}

  
  
  
  
  
  virtual int
    ManipulatePackets(webrtc::EncodedImage* encoded_image) = 0;
};

class PacketManipulatorImpl : public PacketManipulator {
 public:
  PacketManipulatorImpl(PacketReader* packet_reader,
                        const NetworkingConfig& config,
                        bool verbose);
  virtual ~PacketManipulatorImpl();
  virtual int ManipulatePackets(webrtc::EncodedImage* encoded_image);
  virtual void InitializeRandomSeed(unsigned int seed);
 protected:
  
  virtual double RandomUniform();
 private:
  PacketReader* packet_reader_;
  const NetworkingConfig& config_;
  
  int active_burst_packets_;
  CriticalSectionWrapper* critsect_;
  unsigned int random_seed_;
  bool verbose_;
};

}  
}  

#endif  
