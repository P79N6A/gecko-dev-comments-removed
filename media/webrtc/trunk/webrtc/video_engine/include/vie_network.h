









#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_NETWORK_H_









#include "common_types.h"

namespace webrtc {

class Transport;
class VideoEngine;


enum ViEPacketTimeout {
  NoPacket = 0,
  PacketReceived = 1
};





class WEBRTC_DLLEXPORT ViENetworkObserver {
 public:
  
  
  virtual void OnPeriodicDeadOrAlive(const int video_channel,
                                     const bool alive) = 0;

  
  virtual void PacketTimeout(const int video_channel,
                             const ViEPacketTimeout timeout) = 0;
 protected:
  virtual ~ViENetworkObserver() {}
};

class WEBRTC_DLLEXPORT ViENetwork {
 public:
  
  enum { KDefaultSampleTimeSeconds = 2 };

  
  
  
  static ViENetwork* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  
  virtual int SetLocalReceiver(const int video_channel,
                               const unsigned short rtp_port,
                               const unsigned short rtcp_port = 0,
                               const char* ip_address = NULL) = 0;

  
  virtual int GetLocalReceiver(const int video_channel,
                               unsigned short& rtp_port,
                               unsigned short& rtcp_port, char* ip_address) = 0;

  
  virtual int SetSendDestination(const int video_channel,
                                 const char* ip_address,
                                 const unsigned short rtp_port,
                                 const unsigned short rtcp_port = 0,
                                 const unsigned short source_rtp_port = 0,
                                 const unsigned short source_rtcp_port = 0) = 0;

  
  virtual int GetSendDestination(const int video_channel,
                                 char* ip_address,
                                 unsigned short& rtp_port,
                                 unsigned short& rtcp_port,
                                 unsigned short& source_rtp_port,
                                 unsigned short& source_rtcp_port) = 0;

  
  
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport) = 0;

  
  virtual int DeregisterSendTransport(const int video_channel) = 0;

  
  
  
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const int length) = 0;

  
  
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const int length) = 0;

  
  
  virtual int GetSourceInfo(const int video_channel,
                            unsigned short& rtp_port,
                            unsigned short& rtcp_port,
                            char* ip_address,
                            unsigned int ip_address_length) = 0;

  
  virtual int GetLocalIP(char ip_address[64], bool ipv6 = false) = 0;

  
  virtual int EnableIPv6(int video_channel) = 0;

  
  virtual bool IsIPv6Enabled(int video_channel) = 0;

  
  
  virtual int SetSourceFilter(const int video_channel,
                              const unsigned short rtp_port,
                              const unsigned short rtcp_port = 0,
                              const char* ip_address = NULL) = 0;

  
  virtual int GetSourceFilter(const int video_channel,
                              unsigned short& rtp_port,
                              unsigned short& rtcp_port,
                              char* ip_address) = 0;

  
  
  
  virtual int SetSendToS(const int video_channel,
                         const int DSCP,
                         const bool use_set_sockOpt = false) = 0;

  
  
  virtual int GetSendToS(const int video_channel,
                         int& DSCP,
                         bool& use_set_sockOpt) = 0;

  
  
  
  virtual int SetSendGQoS(const int video_channel, const bool enable,
                          const int service_type,
                          const int overrideDSCP = 0) = 0;

  
  
  virtual int GetSendGQoS(const int video_channel,
                          bool& enabled,
                          int& service_type,
                          int& overrideDSCP) = 0;

  
  
  
  virtual int SetMTU(int video_channel, unsigned int mtu) = 0;

  
  
  virtual int SetPacketTimeoutNotification(const int video_channel,
                                           bool enable,
                                           int timeout_seconds) = 0;

  
  
  virtual int RegisterObserver(const int video_channel,
                               ViENetworkObserver& observer) = 0;

  
  virtual int DeregisterObserver(const int video_channel) = 0;

  
  
  virtual int SetPeriodicDeadOrAliveStatus(
      const int video_channel,
      const bool enable,
      const unsigned int sample_time_seconds = KDefaultSampleTimeSeconds) = 0;

  
  
  virtual int SendUDPPacket(const int video_channel,
                            const void* data,
                            const unsigned int length,
                            int& transmitted_bytes,
                            bool use_rtcp_socket = false) = 0;

 protected:
  ViENetwork() {}
  virtual ~ViENetwork() {}
};

}  

#endif
