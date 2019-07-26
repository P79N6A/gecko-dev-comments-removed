









#ifndef WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_NETWORK_IMPL_H

#include "voe_network.h"

#include "shared_data.h"


namespace webrtc
{

class VoENetworkImpl: public VoENetwork
{
public:
    virtual int RegisterExternalTransport(int channel, Transport& transport);

    virtual int DeRegisterExternalTransport(int channel);

    virtual int ReceivedRTPPacket(int channel,
                                  const void* data,
                                  unsigned int length);

    virtual int ReceivedRTCPPacket(int channel,
                                   const void* data,
                                   unsigned int length);

    virtual int SetPacketTimeoutNotification(int channel,
                                             bool enable,
                                             int timeoutSeconds = 2);

    virtual int GetPacketTimeoutNotification(int channel,
                                             bool& enabled,
                                             int& timeoutSeconds);

    virtual int RegisterDeadOrAliveObserver(int channel,
                                            VoEConnectionObserver& observer);

    virtual int DeRegisterDeadOrAliveObserver(int channel);

    virtual int SetPeriodicDeadOrAliveStatus(int channel,
                                             bool enable,
                                             int sampleTimeSeconds = 2);

    virtual int GetPeriodicDeadOrAliveStatus(int channel,
                                             bool& enabled,
                                             int& sampleTimeSeconds);

protected:
    VoENetworkImpl(voe::SharedData* shared);
    virtual ~VoENetworkImpl();
private:
    voe::SharedData* _shared;
};

} 

#endif  
