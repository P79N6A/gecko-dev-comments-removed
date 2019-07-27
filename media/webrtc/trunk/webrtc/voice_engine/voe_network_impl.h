









#ifndef WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H

#include "webrtc/voice_engine/include/voe_network.h"

#include "webrtc/voice_engine/shared_data.h"


namespace webrtc
{

class VoENetworkImpl: public VoENetwork
{
public:
    virtual int RegisterExternalTransport(int channel,
                                          Transport& transport) OVERRIDE;

    virtual int DeRegisterExternalTransport(int channel) OVERRIDE;

    virtual int ReceivedRTPPacket(int channel,
                                  const void* data,
                                  unsigned int length) OVERRIDE;
    virtual int ReceivedRTPPacket(int channel,
                                  const void* data,
                                  unsigned int length,
                                  const PacketTime& packet_time) OVERRIDE;

    virtual int ReceivedRTCPPacket(int channel,
                                   const void* data,
                                   unsigned int length) OVERRIDE;

protected:
    VoENetworkImpl(voe::SharedData* shared);
    virtual ~VoENetworkImpl();
private:
    voe::SharedData* _shared;
};

}  

#endif  
