









#ifndef WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_NETWORK_IMPL_H_

#include "webrtc/typedefs.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/vie_ref_count.h"

namespace webrtc {

class ViESharedData;

class ViENetworkImpl
    : public ViENetwork,
      public ViERefCount {
 public:
  
  virtual int Release();
  virtual void SetNetworkTransmissionState(const int video_channel,
                                           const bool is_transmitting);
  virtual int RegisterSendTransport(const int video_channel,
                                    Transport& transport);
  virtual int DeregisterSendTransport(const int video_channel);
  virtual int ReceivedRTPPacket(const int video_channel,
                                const void* data,
                                const int length,
                                const PacketTime& packet_time);
  virtual int ReceivedRTCPPacket(const int video_channel,
                                 const void* data,
                                 const int length);
  virtual int SetMTU(int video_channel, unsigned int mtu);

 protected:
  explicit ViENetworkImpl(ViESharedData* shared_data);
  virtual ~ViENetworkImpl();

 private:
  ViESharedData* shared_data_;
};

}  

#endif  
