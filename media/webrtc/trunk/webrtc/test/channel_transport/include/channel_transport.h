









#ifndef WEBRTC_TEST_CHANNEL_TRANSPORT_INCLUDE_CHANNEL_TRANSPORT_H_
#define WEBRTC_TEST_CHANNEL_TRANSPORT_INCLUDE_CHANNEL_TRANSPORT_H_

#include "webrtc/test/channel_transport/udp_transport.h"

namespace webrtc {

class ViENetwork;
class VoENetwork;

namespace test {


class VoiceChannelTransport : public UdpTransportData {
 public:
  VoiceChannelTransport(VoENetwork* voe_network, int channel);

  virtual ~VoiceChannelTransport();

  
  virtual void IncomingRTPPacket(const int8_t* incoming_rtp_packet,
                                 const int32_t packet_length,
                                 const char* ,
                                 const uint16_t ) OVERRIDE;

  virtual void IncomingRTCPPacket(const int8_t* incoming_rtcp_packet,
                                  const int32_t packet_length,
                                  const char* ,
                                  const uint16_t ) OVERRIDE;
  

  
  int SetLocalReceiver(uint16_t rtp_port);

  
  int SetSendDestination(const char* ip_address, uint16_t rtp_port);

 private:
  int channel_;
  VoENetwork* voe_network_;
  UdpTransport* socket_transport_;
};


class VideoChannelTransport : public UdpTransportData {
 public:
  VideoChannelTransport(ViENetwork* vie_network, int channel);

  virtual  ~VideoChannelTransport();

  
  virtual void IncomingRTPPacket(const int8_t* incoming_rtp_packet,
                         const int32_t packet_length,
                         const char* ,
                         const uint16_t ) OVERRIDE;

  virtual void IncomingRTCPPacket(const int8_t* incoming_rtcp_packet,
                          const int32_t packet_length,
                          const char* ,
                          const uint16_t ) OVERRIDE;
  

  
  int SetLocalReceiver(uint16_t rtp_port);

  
  int SetSendDestination(const char* ip_address, uint16_t rtp_port);

 private:
  int channel_;
  ViENetwork* vie_network_;
  UdpTransport* socket_transport_;
};

}  
}  

#endif  
