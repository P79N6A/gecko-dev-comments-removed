









#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_









#include "webrtc/common_types.h"

namespace webrtc {

class Transport;
class VideoEngine;


enum ViEPacketTimeout {
  NoPacket = 0,
  PacketReceived = 1
};

class WEBRTC_DLLEXPORT ViENetwork {
 public:
  
  enum { KDefaultSampleTimeSeconds = 2 };

  
  
  
  static ViENetwork* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  
  virtual void SetNetworkTransmissionState(const int video_channel,
                                           const bool is_transmitting) = 0;

  
  
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport) = 0;

  
  virtual int DeregisterSendTransport(const int video_channel) = 0;

  
  
  
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const int length) = 0;

  
  
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const int length) = 0;

  
  
  
  virtual int SetMTU(int video_channel, unsigned int mtu) = 0;

 protected:
  ViENetwork() {}
  virtual ~ViENetwork() {}
};

}  

#endif
