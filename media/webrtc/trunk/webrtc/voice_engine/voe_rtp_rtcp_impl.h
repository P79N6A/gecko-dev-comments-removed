









#ifndef WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_IMPL_H
#define WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_IMPL_H

#include "voe_rtp_rtcp.h"

#include "shared_data.h"

namespace webrtc {

class VoERTP_RTCPImpl : public VoERTP_RTCP
{
public:
    
    virtual int RegisterRTPObserver(int channel, VoERTPObserver& observer);

    virtual int DeRegisterRTPObserver(int channel);

    virtual int RegisterRTCPObserver(int channel, VoERTCPObserver& observer);

    virtual int DeRegisterRTCPObserver(int channel);

    
    virtual int SetRTCPStatus(int channel, bool enable);

    virtual int GetRTCPStatus(int channel, bool& enabled);

    virtual int SetRTCP_CNAME(int channel, const char cName[256]);

    virtual int GetRTCP_CNAME(int channel, char cName[256]);

    virtual int GetRemoteRTCP_CNAME(int channel, char cName[256]);

    virtual int GetRemoteRTCPData(int channel,
                                  unsigned int& NTPHigh,
                                  unsigned int& NTPLow,
                                  unsigned int& timestamp,
                                  unsigned int& playoutTimestamp,
                                  unsigned int* jitter = NULL,
                                  unsigned short* fractionLost = NULL);

    virtual int SendApplicationDefinedRTCPPacket(
        int channel,
        const unsigned char subType,
        unsigned int name,
        const char* data,
        unsigned short dataLengthInBytes);

    
    virtual int SetLocalSSRC(int channel, unsigned int ssrc);

    virtual int GetLocalSSRC(int channel, unsigned int& ssrc);

    virtual int GetRemoteSSRC(int channel, unsigned int& ssrc);

    
    virtual int SetRTPAudioLevelIndicationStatus(int channel,
                                                 bool enable,
                                                 unsigned char ID);

    virtual int GetRTPAudioLevelIndicationStatus(int channel,
                                                 bool& enabled,
                                                 unsigned char& ID);

    
    virtual int GetRemoteCSRCs(int channel, unsigned int arrCSRC[15]);

    
    virtual int GetRTPStatistics(int channel,
                                 unsigned int& averageJitterMs,
                                 unsigned int& maxJitterMs,
                                 unsigned int& discardedPackets);

    virtual int GetRTCPStatistics(int channel, CallStatistics& stats);

    virtual int GetRemoteRTCPSenderInfo(int channel, SenderInfo* sender_info);

    virtual int GetRemoteRTCPReportBlocks(
        int channel, std::vector<ReportBlock>* report_blocks);

    
    virtual int SetFECStatus(int channel,
                             bool enable,
                             int redPayloadtype = -1);

    virtual int GetFECStatus(int channel, bool& enabled, int& redPayloadtype);

    
    virtual int StartRTPDump(int channel,
                             const char fileNameUTF8[1024],
                             RTPDirections direction = kRtpIncoming);

    virtual int StopRTPDump(int channel,
                            RTPDirections direction = kRtpIncoming);

    virtual int RTPDumpIsActive(int channel,
                                RTPDirections direction = kRtpIncoming);

    
    virtual int InsertExtraRTPPacket(int channel,
                                     unsigned char payloadType,
                                     bool markerBit,
                                     const char* payloadData,
                                     unsigned short payloadSize);
    virtual int GetLastRemoteTimeStamp(int channel,
                                       uint32_t* lastRemoteTimeStamp);
protected:
    VoERTP_RTCPImpl(voe::SharedData* shared);
    virtual ~VoERTP_RTCPImpl();

private:
    voe::SharedData* _shared;
};

}  

#endif    

