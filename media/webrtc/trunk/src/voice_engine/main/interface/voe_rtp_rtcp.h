






































#ifndef WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_H
#define WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_H

#include "common_types.h"

namespace webrtc {

class VoiceEngine;


class WEBRTC_DLLEXPORT VoERTPObserver
{
public:
    virtual void OnIncomingCSRCChanged(
        const int channel, const unsigned int CSRC, const bool added) = 0;

    virtual void OnIncomingSSRCChanged(
        const int channel, const unsigned int SSRC) = 0;

protected:
    virtual ~VoERTPObserver() {}
};


class WEBRTC_DLLEXPORT VoERTCPObserver
{
public:
    virtual void OnApplicationDataReceived(
        const int channel, const unsigned char subType,
        const unsigned int name, const unsigned char* data,
        const unsigned short dataLengthInBytes) = 0;

protected:
    virtual ~VoERTCPObserver() {}
};


struct CallStatistics
{
    unsigned short fractionLost;
    unsigned int cumulativeLost;
    unsigned int extendedMax;
    unsigned int jitterSamples;
    int rttMs;
    int bytesSent;
    int packetsSent;
    int bytesReceived;
    int packetsReceived;
};


class WEBRTC_DLLEXPORT VoERTP_RTCP
{
public:

    
    
    
    static VoERTP_RTCP* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    
    
    virtual int RegisterRTPObserver(int channel, VoERTPObserver& observer) = 0;

    
    
    virtual int DeRegisterRTPObserver(int channel) = 0;

    
    
    virtual int RegisterRTCPObserver(
        int channel, VoERTCPObserver& observer) = 0;

    
    
    virtual int DeRegisterRTCPObserver(int channel) = 0;

    
    virtual int SetLocalSSRC(int channel, unsigned int ssrc) = 0;

    
    virtual int GetLocalSSRC(int channel, unsigned int& ssrc) = 0;

    
    virtual int GetRemoteSSRC(int channel, unsigned int& ssrc) = 0;

    
    virtual int SetRTPAudioLevelIndicationStatus(
        int channel, bool enable, unsigned char ID = 1) = 0;

    
    virtual int GetRTPAudioLevelIndicationStatus(
        int channel, bool& enabled, unsigned char& ID) = 0;

    
    virtual int GetRemoteCSRCs(int channel, unsigned int arrCSRC[15]) = 0;

    
    virtual int SetRTCPStatus(int channel, bool enable) = 0;

    
    virtual int GetRTCPStatus(int channel, bool& enabled) = 0;

    
    
    virtual int SetRTCP_CNAME(int channel, const char cName[256]) = 0;

    
    
    virtual int GetRTCP_CNAME(int channel, char cName[256]) = 0;

    
    
    virtual int GetRemoteRTCP_CNAME(int channel, char cName[256]) = 0;

    
    virtual int GetRemoteRTCPData(
        int channel, unsigned int& NTPHigh, unsigned int& NTPLow,
        unsigned int& timestamp, unsigned int& playoutTimestamp,
        unsigned int* jitter = NULL, unsigned short* fractionLost = NULL) = 0;

    
    virtual int GetRTPStatistics(
        int channel, unsigned int& averageJitterMs, unsigned int& maxJitterMs,
        unsigned int& discardedPackets) = 0;

    
    virtual int GetRTCPStatistics(int channel, CallStatistics& stats) = 0;

    
    virtual int SendApplicationDefinedRTCPPacket(
        int channel, const unsigned char subType, unsigned int name,
        const char* data, unsigned short dataLengthInBytes) = 0;

    
    virtual int SetFECStatus(
        int channel, bool enable, int redPayloadtype = -1) = 0;

    
    virtual int GetFECStatus(
        int channel, bool& enabled, int& redPayloadtype) = 0;

    
    
    
    
    virtual int StartRTPDump(
        int channel, const char fileNameUTF8[1024],
        RTPDirections direction = kRtpIncoming) = 0;

    
    
    virtual int StopRTPDump(
        int channel, RTPDirections direction = kRtpIncoming) = 0;

    
    
    virtual int RTPDumpIsActive(
        int channel, RTPDirections direction = kRtpIncoming) = 0;

    
    
    
    virtual int InsertExtraRTPPacket(
        int channel, unsigned char payloadType, bool markerBit,
        const char* payloadData, unsigned short payloadSize) = 0;

protected:
    VoERTP_RTCP() {}
    virtual ~VoERTP_RTCP() {}
};

}  

#endif  
