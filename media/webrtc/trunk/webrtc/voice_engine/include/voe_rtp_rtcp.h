





































#ifndef WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_H
#define WEBRTC_VOICE_ENGINE_VOE_RTP_RTCP_H

#include <vector>
#include "webrtc/common_types.h"

namespace webrtc {

class ViENetwork;
class VoiceEngine;


class WEBRTC_DLLEXPORT VoERTPObserver
{
public:
    virtual void OnIncomingCSRCChanged(
        int channel, unsigned int CSRC, bool added) = 0;

    virtual void OnIncomingSSRCChanged(
        int channel, unsigned int SSRC) = 0;

protected:
    virtual ~VoERTPObserver() {}
};


class WEBRTC_DLLEXPORT VoERTCPObserver
{
public:
    virtual void OnApplicationDataReceived(
        int channel, unsigned char subType,
        unsigned int name, const unsigned char* data,
        unsigned short dataLengthInBytes) = 0;

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
    
    
    int64_t capture_start_ntp_time_ms_;
};


struct SenderInfo {
  uint32_t NTP_timestamp_high;
  uint32_t NTP_timestamp_low;
  uint32_t RTP_timestamp;
  uint32_t sender_packet_count;
  uint32_t sender_octet_count;
};


struct ReportBlock {
  uint32_t sender_SSRC; 
  uint32_t source_SSRC;
  uint8_t fraction_lost;
  uint32_t cumulative_num_packets_lost;
  uint32_t extended_highest_sequence_number;
  uint32_t interarrival_jitter;
  uint32_t last_SR_timestamp;
  uint32_t delay_since_last_SR;
};


class WEBRTC_DLLEXPORT VoERTP_RTCP
{
public:

    
    
    
    static VoERTP_RTCP* GetInterface(VoiceEngine* voiceEngine);

    
    
    
    
    virtual int Release() = 0;

    
    virtual int SetLocalSSRC(int channel, unsigned int ssrc) = 0;

    
    virtual int GetLocalSSRC(int channel, unsigned int& ssrc) = 0;

    
    virtual int GetRemoteSSRC(int channel, unsigned int& ssrc) = 0;

    
    virtual int SetSendAudioLevelIndicationStatus(int channel,
                                                  bool enable,
                                                  unsigned char id = 1) = 0;

    
    
    virtual int SetReceiveAudioLevelIndicationStatus(int channel,
                                                     bool enable,
                                                     unsigned char id = 1) {
      
      return 0;
    }

    
    virtual int SetSendAbsoluteSenderTimeStatus(int channel,
                                                bool enable,
                                                unsigned char id) = 0;

    
    virtual int SetReceiveAbsoluteSenderTimeStatus(int channel,
                                                   bool enable,
                                                   unsigned char id) = 0;

    
    virtual int SetRTCPStatus(int channel, bool enable) = 0;

    
    virtual int GetRTCPStatus(int channel, bool& enabled) = 0;

    
    
    virtual int SetRTCP_CNAME(int channel, const char cName[256]) = 0;

    
    
    virtual int GetRTCP_CNAME(int channel, char cName[256]) {
      return -1;
    }

    
    
    virtual int GetRemoteRTCP_CNAME(int channel, char cName[256]) = 0;

    
    virtual int GetRemoteRTCPReceiverInfo(
        int channel, uint32_t& NTPHigh, uint32_t& NTPLow,
        uint32_t& receivedPacketCount, uint64_t& receivedOctetCount,
        uint32_t& jitter, uint16_t& fractionLost,
        uint32_t& cumulativeLost,
        int32_t& rttMs) = 0;

    
    virtual int GetRTPStatistics(
        int channel, unsigned int& averageJitterMs, unsigned int& maxJitterMs,
        unsigned int& discardedPackets, unsigned int& cumulativeLost) = 0;

    
    virtual int GetRTCPStatistics(int channel, CallStatistics& stats) = 0;

    
    
    
    
    virtual int GetRemoteRTCPReportBlocks(
        int channel, std::vector<ReportBlock>* receive_blocks) = 0;

    
    
    
    virtual int SetREDStatus(
        int channel, bool enable, int redPayloadtype = -1) { return -1; }

    
    
    
    virtual int GetREDStatus(
        int channel, bool& enabled, int& redPayloadtype) { return -1; }

    
    
    
    virtual int SetFECStatus(
        int channel, bool enable, int redPayloadtype = -1) {
      return SetREDStatus(channel, enable, redPayloadtype);
    };

    
    
    
    virtual int GetFECStatus(
        int channel, bool& enabled, int& redPayloadtype) {
      return SetREDStatus(channel, enabled, redPayloadtype);
    }

    
    
    
    
    virtual int SetNACKStatus(int channel,
                              bool enable,
                              int maxNoPackets) = 0;

    
    
    
    
    virtual int StartRTPDump(
        int channel, const char fileNameUTF8[1024],
        RTPDirections direction = kRtpIncoming) = 0;

    
    
    virtual int StopRTPDump(
        int channel, RTPDirections direction = kRtpIncoming) = 0;

    
    
    virtual int RTPDumpIsActive(
        int channel, RTPDirections direction = kRtpIncoming) = 0;

    
    
    
    virtual int SetVideoEngineBWETarget(int channel, ViENetwork* vie_network,
                                        int video_channel) {
      return 0;
    }

    
    virtual int RegisterRTPObserver(int channel,
            VoERTPObserver& observer) { return -1; };
    virtual int DeRegisterRTPObserver(int channel) { return -1; };
    virtual int RegisterRTCPObserver(
            int channel, VoERTCPObserver& observer) { return -1; };
    virtual int DeRegisterRTCPObserver(int channel) { return -1; };
    virtual int GetRemoteCSRCs(int channel,
            unsigned int arrCSRC[15]) { return -1; };
    virtual int InsertExtraRTPPacket(
            int channel, unsigned char payloadType, bool markerBit,
            const char* payloadData, unsigned short payloadSize) { return -1; };
    virtual int GetRemoteRTCPSenderInfo(
            int channel, SenderInfo* sender_info) { return -1; };
    virtual int SendApplicationDefinedRTCPPacket(
            int channel, unsigned char subType, unsigned int name,
            const char* data, unsigned short dataLengthInBytes) { return -1; };
    virtual int GetLastRemoteTimeStamp(int channel,
            uint32_t* lastRemoteTimeStamp) { return -1; };

protected:
    VoERTP_RTCP() {}
    virtual ~VoERTP_RTCP() {}
};

}  

#endif  
