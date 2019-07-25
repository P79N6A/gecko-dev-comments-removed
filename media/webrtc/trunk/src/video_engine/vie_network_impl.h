









#ifndef WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_

#include "typedefs.h"
#include "video_engine/include/vie_network.h"
#include "video_engine/vie_ref_count.h"

namespace webrtc {

class ViESharedData;

class ViENetworkImpl
    : public ViENetwork,
      public ViERefCount {
 public:
  
  virtual int Release();
  virtual int SetLocalReceiver(const int video_channel,
                               const unsigned short rtp_port,
                               const unsigned short rtcp_port,
                               const char* ip_address);
  virtual int GetLocalReceiver(const int video_channel,
                               unsigned short& rtp_port,
                               unsigned short& rtcp_port,
                               char* ip_address);
  virtual int SetSendDestination(const int video_channel,
                                 const char* ip_address,
                                 const unsigned short rtp_port,
                                 const unsigned short rtcp_port,
                                 const unsigned short source_rtp_port,
                                 const unsigned short source_rtcp_port);
  virtual int GetSendDestination(const int video_channel,
                                 char* ip_address,
                                 unsigned short& rtp_port,
                                 unsigned short& rtcp_port,
                                 unsigned short& source_rtp_port,
                                 unsigned short& source_rtcp_port);
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport);
  virtual int DeregisterSendTransport(const int video_channel);
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const int length);
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const int length);
  virtual int GetSourceInfo(const int video_channel,
                            unsigned short& rtp_port,
                            unsigned short& rtcp_port,
                            char* ip_address,
                            unsigned int ip_address_length);
  virtual int GetLocalIP(char ip_address[64], bool ipv6);
  virtual int EnableIPv6(int video_channel);
  virtual bool IsIPv6Enabled(int video_channel);
  virtual int SetSourceFilter(const int video_channel,
                              const unsigned short rtp_port,
                              const unsigned short rtcp_port,
                              const char* ip_address);
  virtual int GetSourceFilter(const int video_channel,
                              unsigned short& rtp_port,
                              unsigned short& rtcp_port,
                              char* ip_address);
  virtual int SetSendToS(const int video_channel,
                         const int DSCP,
                         const bool use_set_sockOpt);
  virtual int GetSendToS(const int video_channel,
                         int& DSCP,
                         bool& use_set_sockOpt);
  virtual int SetSendGQoS(const int video_channel,
                          const bool enable,
                          const int service_type,
                          const int overrideDSCP);
  virtual int GetSendGQoS(const int video_channel,
                          bool& enabled,
                          int& service_type,
                          int& overrideDSCP);
  virtual int SetMTU(int video_channel, unsigned int mtu);
  virtual int SetPacketTimeoutNotification(const int video_channel,
                                           bool enable,
                                           int timeout_seconds);
  virtual int RegisterObserver(const int video_channel,
                               ViENetworkObserver& observer);
  virtual int DeregisterObserver(const int video_channel);
  virtual int SetPeriodicDeadOrAliveStatus(
      const int video_channel,
      const bool enable,
      const unsigned int sample_time_seconds);
  virtual int SendUDPPacket(const int video_channel,
                            const void* data,
                            const unsigned int length,
                            int& transmitted_bytes,
                            bool use_rtcp_socket);

 protected:
  ViENetworkImpl(ViESharedData* shared_data);
  virtual ~ViENetworkImpl();

 private:
  ViESharedData* shared_data_;
};

}  

#endif  
