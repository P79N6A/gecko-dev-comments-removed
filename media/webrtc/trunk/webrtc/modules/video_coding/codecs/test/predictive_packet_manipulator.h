









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_PREDICTIVE_PACKET_MANIPULATOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_PREDICTIVE_PACKET_MANIPULATOR_H_

#include <queue>

#include "modules/video_coding/codecs/test/packet_manipulator.h"
#include "testsupport/packet_reader.h"

namespace webrtc {
namespace test {



class PredictivePacketManipulator : public PacketManipulatorImpl {
 public:
  PredictivePacketManipulator(PacketReader* packet_reader,
                              const NetworkingConfig& config);
  virtual ~PredictivePacketManipulator();
  
  
  
  
  void AddRandomResult(double result);
 protected:
  
  virtual double RandomUniform();

 private:
  std::queue<double> random_results_;
};

}  
}  

#endif  
