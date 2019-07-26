




































#ifndef WEBRTC_VOICE_ENGINE_VOE_NETWORK_H
#define WEBRTC_VOICE_ENGINE_VOE_NETWORK_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoEConnectionObserver
{
public:
    
    
    
    virtual void OnPeriodicDeadOrAlive(const int channel, const bool alive) = 0;

protected:
    virtual ~VoEConnectionObserver() {}
};


class WEBRTC_DLLEXPORT VoENetwork
{
public:
    
    
    
    static VoENetwork* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    virtual int RegisterExternalTransport(
        int channel, Transport& transport) = 0;

    
    
    virtual int DeRegisterExternalTransport(int channel) = 0;

    
    
    
    virtual int ReceivedRTPPacket(
        int channel, const void* data, unsigned int length) = 0;

    
    
    
    virtual int ReceivedRTCPPacket(
        int channel, const void* data, unsigned int length) = 0;

    
    
    virtual int GetSourceInfo(
        int channel, int& rtpPort, int& rtcpPort, char ipAddr[64]) = 0;

    
    virtual int GetLocalIP(char ipAddr[64], bool ipv6 = false) = 0;

    
    virtual int EnableIPv6(int channel) = 0;

    
    virtual bool IPv6IsEnabled(int channel) = 0;

    
    
    virtual int SetSourceFilter(int channel,
        int rtpPort, int rtcpPort = 0, const char ipAddr[64] = 0) = 0;

    
    virtual int GetSourceFilter(
        int channel, int& rtpPort, int& rtcpPort, char ipAddr[64]) = 0;

    
    
    virtual int SetSendTOS(int channel,
        int DSCP, int priority = -1, bool useSetSockopt = false) = 0;

    
    
    virtual int GetSendTOS(
        int channel, int& DSCP, int& priority, bool& useSetSockopt) = 0;

    
    
    
    virtual int SetSendGQoS(
        int channel, bool enable, int serviceType, int overrideDSCP = 0) = 0;

    
    virtual int GetSendGQoS(
        int channel, bool& enabled, int& serviceType, int& overrideDSCP) = 0;

    
    
    virtual int SetPacketTimeoutNotification(
        int channel, bool enable, int timeoutSeconds = 2) = 0;

    
    virtual int GetPacketTimeoutNotification(
        int channel, bool& enabled, int& timeoutSeconds) = 0;

    
    virtual int RegisterDeadOrAliveObserver(
        int channel, VoEConnectionObserver& observer) = 0;

    
    virtual int DeRegisterDeadOrAliveObserver(int channel) = 0;

    
    
    virtual int SetPeriodicDeadOrAliveStatus(
        int channel, bool enable, int sampleTimeSeconds = 2) = 0;

    
    virtual int GetPeriodicDeadOrAliveStatus(
        int channel, bool& enabled, int& sampleTimeSeconds) = 0;

    
    
    virtual int SendUDPPacket(
        int channel, const void* data, unsigned int length,
        int& transmittedBytes, bool useRtcpSocket = false) = 0;

protected:
    VoENetwork() {}
    virtual ~VoENetwork() {}
};

} 

#endif  
