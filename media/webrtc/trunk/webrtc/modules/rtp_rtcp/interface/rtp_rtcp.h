









#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RTCP_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RTCP_H_

#include <vector>

#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {

class PacedSender;
class ReceiveStatistics;
class RemoteBitrateEstimator;
class RtpReceiver;
class Transport;

class RtpRtcp : public Module {
 public:
  struct Configuration {
    Configuration();

   
























    int32_t id;
    bool audio;
    Clock* clock;
    RtpRtcp* default_module;
    ReceiveStatistics* receive_statistics;
    Transport* outgoing_transport;
    RtcpFeedback* rtcp_feedback;
    RtcpIntraFrameObserver* intra_frame_callback;
    RtcpBandwidthObserver* bandwidth_callback;
    RtcpRttStats* rtt_stats;
    RtpAudioFeedback* audio_messages;
    RemoteBitrateEstimator* remote_bitrate_estimator;
    PacedSender* paced_sender;
  };

  




  static RtpRtcp* CreateRtpRtcp(const RtpRtcp::Configuration& configuration);

  





    virtual int32_t IncomingRtcpPacket(const uint8_t* incoming_packet,
                                       uint16_t incoming_packet_length) = 0;

    virtual void SetRemoteSSRC(const uint32_t ssrc) = 0;

    





    






    virtual int32_t SetMaxTransferUnit(const uint16_t size) = 0;

    










    virtual int32_t SetTransportOverhead(
        const bool TCP,
        const bool IPV6,
        const uint8_t authenticationOverhead = 0) = 0;

    







    virtual uint16_t MaxPayloadLength() const = 0;

    







    virtual uint16_t MaxDataPayloadLength() const = 0;

    




    virtual int32_t RegisterSendPayload(
        const CodecInst& voiceCodec) = 0;

    




    virtual int32_t RegisterSendPayload(
        const VideoCodec& videoCodec) = 0;

    






    virtual int32_t DeRegisterSendPayload(
        const int8_t payloadType) = 0;

   




    virtual int32_t RegisterSendRtpHeaderExtension(
        const RTPExtensionType type,
        const uint8_t id) = 0;

    virtual int32_t DeregisterSendRtpHeaderExtension(
        const RTPExtensionType type) = 0;

    


    virtual uint32_t StartTimestamp() const = 0;

    






    virtual int32_t SetStartTimestamp(
        const uint32_t timestamp) = 0;

    


    virtual uint16_t SequenceNumber() const = 0;

    




    virtual int32_t SetSequenceNumber(const uint16_t seq) = 0;

    


    virtual uint32_t SSRC() const = 0;

    




    virtual int32_t SetSSRC(const uint32_t ssrc) = 0;

    






    virtual int32_t CSRCs(
        uint32_t arrOfCSRC[kRtpCsrcSize]) const = 0;

    







    virtual int32_t SetCSRCs(
        const uint32_t arrOfCSRC[kRtpCsrcSize],
        const uint8_t arrLength) = 0;

    








    virtual int32_t SetCSRCStatus(const bool include) = 0;

    


    virtual int32_t SetRTXSendStatus(int modes, bool set_ssrc,
                                     uint32_t ssrc) = 0;

    
    
    virtual void SetRtxSendPayloadType(int payload_type) = 0;

    


    virtual int32_t RTXSendStatus(int* modes, uint32_t* ssrc,
                                  int* payloadType) const = 0;

    






    virtual int32_t SetSendingStatus(const bool sending) = 0;

    


    virtual bool Sending() const = 0;

    






    virtual int32_t SetSendingMediaStatus(const bool sending) = 0;

    


    virtual bool SendingMedia() const = 0;

    


    virtual void BitrateSent(uint32_t* totalRate,
                             uint32_t* videoRate,
                             uint32_t* fecRate,
                             uint32_t* nackRate) const = 0;

    


    virtual void RegisterVideoBitrateObserver(
        BitrateStatisticsObserver* observer) = 0;
    virtual BitrateStatisticsObserver* GetVideoBitrateObserver() const = 0;

    













    virtual int32_t SendOutgoingData(
        const FrameType frameType,
        const int8_t payloadType,
        const uint32_t timeStamp,
        int64_t capture_time_ms,
        const uint8_t* payloadData,
        const uint32_t payloadSize,
        const RTPFragmentationHeader* fragmentation = NULL,
        const RTPVideoHeader* rtpVideoHdr = NULL) = 0;

    virtual bool TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission) = 0;

    virtual int TimeToSendPadding(int bytes) = 0;

    virtual void RegisterSendFrameCountObserver(
        FrameCountObserver* observer) = 0;
    virtual FrameCountObserver* GetSendFrameCountObserver() const = 0;

    virtual bool GetSendSideDelay(int* avg_send_delay_ms,
                                  int* max_send_delay_ms) const = 0;

    
    virtual void RegisterSendChannelRtpStatisticsCallback(
        StreamDataCountersCallback* callback) = 0;
    virtual StreamDataCountersCallback*
        GetSendChannelRtpStatisticsCallback() const = 0;

    





    


    virtual RTCPMethod RTCP() const = 0;

    






    virtual int32_t SetRTCPStatus(const RTCPMethod method) = 0;

    




    virtual int32_t SetCNAME(const char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual int32_t CNAME(char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual int32_t RemoteCNAME(
        const uint32_t remoteSSRC,
        char cName[RTCP_CNAME_SIZE]) const = 0;

    




    virtual int32_t RemoteNTP(
        uint32_t *ReceivedNTPsecs,
        uint32_t *ReceivedNTPfrac,
        uint32_t *RTCPArrivalTimeSecs,
        uint32_t *RTCPArrivalTimeFrac,
        uint32_t *rtcp_timestamp) const  = 0;

    




    virtual int32_t AddMixedCNAME(
        const uint32_t SSRC,
        const char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual int32_t RemoveMixedCNAME(const uint32_t SSRC) = 0;

    




    virtual int32_t RTT(const uint32_t remoteSSRC,
                        uint16_t* RTT,
                        uint16_t* avgRTT,
                        uint16_t* minRTT,
                        uint16_t* maxRTT) const = 0 ;

    




    virtual int32_t ResetRTT(const uint32_t remoteSSRC)= 0 ;

    





    virtual int32_t GetReportBlockInfo(const uint32_t remote_ssrc,
                                       uint32_t* ntp_high,
                                       uint32_t* ntp_low,
                                       uint32_t* packets_received,
                                       uint64_t* octets_received) const = 0;
    





    virtual int32_t SendRTCP(
        uint32_t rtcpPacketType = kRtcpReport) = 0;

    


    virtual int32_t SendRTCPReferencePictureSelection(
        const uint64_t pictureID) = 0;

    



    virtual int32_t SendRTCPSliceLossIndication(
        const uint8_t pictureID) = 0;

    




    virtual int32_t ResetSendDataCountersRTP() = 0;

    




    virtual int32_t DataCountersRTP(
        uint32_t* bytesSent,
        uint32_t* packetsSent) const = 0;
    




    virtual int32_t RemoteRTCPStat(RTCPSenderInfo* senderInfo) = 0;

    




    virtual int32_t RemoteRTCPStat(
        std::vector<RTCPReportBlock>* receiveBlocks) const = 0;
    




    virtual int32_t AddRTCPReportBlock(
        const uint32_t SSRC,
        const RTCPReportBlock* receiveBlock) = 0;

    




    virtual int32_t RemoveRTCPReportBlock(const uint32_t SSRC) = 0;

    




    virtual int32_t SetRTCPApplicationSpecificData(
        const uint8_t subType,
        const uint32_t name,
        const uint8_t* data,
        const uint16_t length) = 0;
    




    virtual int32_t SetRTCPVoIPMetrics(
        const RTCPVoIPMetric* VoIPMetric) = 0;

    


    virtual void SetRtcpXrRrtrStatus(bool enable) = 0;

    virtual bool RtcpXrRrtrStatus() const = 0;

    


    virtual bool REMB() const = 0;

    virtual int32_t SetREMBStatus(const bool enable) = 0;

    virtual int32_t SetREMBData(const uint32_t bitrate,
                                const uint8_t numberOfSSRC,
                                const uint32_t* SSRC) = 0;

    


    virtual bool IJ() const = 0;

    virtual int32_t SetIJStatus(const bool enable) = 0;

    


    virtual bool TMMBR() const = 0;

    



    virtual int32_t SetTMMBRStatus(const bool enable) = 0;

    



    



    virtual int SelectiveRetransmissions() const = 0;

    











    virtual int SetSelectiveRetransmissions(uint8_t settings) = 0;

    




    virtual int32_t SendNACK(const uint16_t* nackList,
                             const uint16_t size) = 0;

    





    virtual int32_t SetStorePacketsStatus(
        const bool enable,
        const uint16_t numberToStore) = 0;

    
    virtual bool StorePackets() const = 0;

    
    virtual void RegisterSendChannelRtcpStatisticsCallback(
        RtcpStatisticsCallback* callback) = 0;
    virtual RtcpStatisticsCallback*
        GetSendChannelRtcpStatisticsCallback() = 0;

    





    





    virtual int32_t SetAudioPacketSize(
        const uint16_t packetSizeSamples) = 0;

    






    virtual bool SendTelephoneEventActive(
        int8_t& telephoneEvent) const = 0;

    




    virtual int32_t SendTelephoneEventOutband(
        const uint8_t key,
        const uint16_t time_ms,
        const uint8_t level) = 0;

    




    virtual int32_t SetSendREDPayloadType(
        const int8_t payloadType) = 0;

    




     virtual int32_t SendREDPayloadType(
         int8_t& payloadType) const = 0;

     





     virtual int32_t SetRTPAudioLevelIndicationStatus(
         const bool enable,
         const uint8_t ID) = 0;

     




     virtual int32_t GetRTPAudioLevelIndicationStatus(
         bool& enable,
         uint8_t& ID) const = 0;

     







     virtual int32_t SetAudioLevel(const uint8_t level_dBov) = 0;

    





    




    virtual int32_t SetCameraDelay(const int32_t delayMS) = 0;

    


    virtual void SetTargetSendBitrate(
        const std::vector<uint32_t>& stream_bitrates) = 0;

    




    virtual int32_t SetGenericFECStatus(
        const bool enable,
        const uint8_t payloadTypeRED,
        const uint8_t payloadTypeFEC) = 0;

    




    virtual int32_t GenericFECStatus(bool& enable,
                                     uint8_t& payloadTypeRED,
                                     uint8_t& payloadTypeFEC) = 0;


    virtual int32_t SetFecParameters(
        const FecProtectionParams* delta_params,
        const FecProtectionParams* key_params) = 0;

    




    virtual int32_t SetKeyFrameRequestMethod(
        const KeyFrameRequestMethod method) = 0;

    




    virtual int32_t RequestKeyFrame() = 0;
};
}  
#endif 
