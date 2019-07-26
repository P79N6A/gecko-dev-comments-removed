









#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RTCP_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RTP_RTCP_H_

#include <vector>

#include "modules/interface/module.h"
#include "modules/rtp_rtcp/interface/rtp_rtcp_defines.h"

namespace webrtc {

class PacedSender;
class RemoteBitrateEstimator;
class RemoteBitrateObserver;
class Transport;

class RtpRtcp : public Module {
 public:
  struct Configuration {
    Configuration()
        : id(-1),
          audio(false),
          clock(NULL),
          default_module(NULL),
          incoming_data(NULL),
          incoming_messages(NULL),
          outgoing_transport(NULL),
          rtcp_feedback(NULL),
          intra_frame_callback(NULL),
          bandwidth_callback(NULL),
          rtt_observer(NULL),
          audio_messages(NULL),
          remote_bitrate_estimator(NULL),
          paced_sender(NULL) {
    }
   





















    int32_t id;
    bool audio;
    RtpRtcpClock* clock;
    RtpRtcp* default_module;
    RtpData* incoming_data;
    RtpFeedback* incoming_messages;
    Transport* outgoing_transport;
    RtcpFeedback* rtcp_feedback;
    RtcpIntraFrameObserver* intra_frame_callback;
    RtcpBandwidthObserver* bandwidth_callback;
    RtcpRttObserver* rtt_observer;
    RtpAudioFeedback* audio_messages;
    RemoteBitrateEstimator* remote_bitrate_estimator;
    PacedSender* paced_sender;
  };
  




  static RtpRtcp* CreateRtpRtcp(const RtpRtcp::Configuration& configuration);

  





    







    virtual WebRtc_Word32 SetPacketTimeout(
        const WebRtc_UWord32 RTPtimeoutMS,
        const WebRtc_UWord32 RTCPtimeoutMS) = 0;

    








    virtual WebRtc_Word32 SetPeriodicDeadOrAliveStatus(
        const bool enable,
        const WebRtc_UWord8 sampleTimeSeconds) = 0;

    








    virtual WebRtc_Word32 PeriodicDeadOrAliveStatus(
        bool& enable,
        WebRtc_UWord8& sampleTimeSeconds) = 0;

    




    virtual WebRtc_Word32 RegisterReceivePayload(
        const CodecInst& voiceCodec) = 0;

    




    virtual WebRtc_Word32 RegisterReceivePayload(
        const VideoCodec& videoCodec) = 0;

    




    virtual WebRtc_Word32 ReceivePayloadType(
        const CodecInst& voiceCodec,
        WebRtc_Word8* plType) = 0;

    




    virtual WebRtc_Word32 ReceivePayloadType(
        const VideoCodec& videoCodec,
        WebRtc_Word8* plType) = 0;

    






    virtual WebRtc_Word32 DeRegisterReceivePayload(
        const WebRtc_Word8 payloadType) = 0;

   




    virtual WebRtc_Word32 RegisterReceiveRtpHeaderExtension(
        const RTPExtensionType type,
        const WebRtc_UWord8 id) = 0;

    virtual WebRtc_Word32 DeregisterReceiveRtpHeaderExtension(
        const RTPExtensionType type) = 0;

    


    virtual WebRtc_UWord32 RemoteTimestamp() const = 0;

    


    virtual int64_t LocalTimeOfRemoteTimeStamp() const = 0;

    






    virtual WebRtc_Word32 EstimatedRemoteTimeStamp(
        WebRtc_UWord32& timestamp) const = 0;

    


    virtual WebRtc_UWord32 RemoteSSRC() const = 0;

    






    virtual WebRtc_Word32 RemoteCSRCs(
        WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize]) const  = 0;

    






    virtual WebRtc_Word32 SSRCFilter(WebRtc_UWord32& allowedSSRC) const = 0;

    






    virtual WebRtc_Word32 SetSSRCFilter(const bool enable,
                                        const WebRtc_UWord32 allowedSSRC) = 0;

    


    virtual WebRtc_Word32 SetRTXReceiveStatus(const bool enable,
                                              const WebRtc_UWord32 SSRC) = 0;

    


    virtual WebRtc_Word32 RTXReceiveStatus(bool* enable,
                                           WebRtc_UWord32* SSRC) const = 0;

    







    virtual WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incomingPacket,
                                         const WebRtc_UWord16 packetLength) = 0;

    





    






    virtual WebRtc_Word32 SetMaxTransferUnit(const WebRtc_UWord16 size) = 0;

    










    virtual WebRtc_Word32 SetTransportOverhead(
        const bool TCP,
        const bool IPV6,
        const WebRtc_UWord8 authenticationOverhead = 0) = 0;

    







    virtual WebRtc_UWord16 MaxPayloadLength() const = 0;

    







    virtual WebRtc_UWord16 MaxDataPayloadLength() const = 0;

    




    virtual WebRtc_Word32 RegisterSendPayload(
        const CodecInst& voiceCodec) = 0;

    




    virtual WebRtc_Word32 RegisterSendPayload(
        const VideoCodec& videoCodec) = 0;

    






    virtual WebRtc_Word32 DeRegisterSendPayload(
        const WebRtc_Word8 payloadType) = 0;

   




    virtual WebRtc_Word32 RegisterSendRtpHeaderExtension(
        const RTPExtensionType type,
        const WebRtc_UWord8 id) = 0;

    virtual WebRtc_Word32 DeregisterSendRtpHeaderExtension(
        const RTPExtensionType type) = 0;

    


    virtual WebRtc_UWord32 StartTimestamp() const = 0;

    






    virtual WebRtc_Word32 SetStartTimestamp(
        const WebRtc_UWord32 timestamp) = 0;

    


    virtual WebRtc_UWord16 SequenceNumber() const = 0;

    




    virtual WebRtc_Word32 SetSequenceNumber(const WebRtc_UWord16 seq) = 0;

    


    virtual WebRtc_UWord32 SSRC() const = 0;

    




    virtual WebRtc_Word32 SetSSRC(const WebRtc_UWord32 ssrc) = 0;

    






    virtual WebRtc_Word32 CSRCs(
        WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize]) const = 0;

    







    virtual WebRtc_Word32 SetCSRCs(
        const WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize],
        const WebRtc_UWord8 arrLength) = 0;

    








    virtual WebRtc_Word32 SetCSRCStatus(const bool include) = 0;

    


    virtual WebRtc_Word32 SetRTXSendStatus(const bool enable,
                                           const bool setSSRC,
                                           const WebRtc_UWord32 SSRC) = 0;

    


    virtual WebRtc_Word32 RTXSendStatus(bool* enable,
                                        WebRtc_UWord32* SSRC) const = 0;

    






    virtual WebRtc_Word32 SetSendingStatus(const bool sending) = 0;

    


    virtual bool Sending() const = 0;

    






    virtual WebRtc_Word32 SetSendingMediaStatus(const bool sending) = 0;

    


    virtual bool SendingMedia() const = 0;

    


    virtual void BitrateSent(WebRtc_UWord32* totalRate,
                             WebRtc_UWord32* videoRate,
                             WebRtc_UWord32* fecRate,
                             WebRtc_UWord32* nackRate) const = 0;

    


    virtual int EstimatedReceiveBandwidth(
        WebRtc_UWord32* available_bandwidth) const = 0;

    













    virtual WebRtc_Word32 SendOutgoingData(
        const FrameType frameType,
        const WebRtc_Word8 payloadType,
        const WebRtc_UWord32 timeStamp,
        int64_t capture_time_ms,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader* fragmentation = NULL,
        const RTPVideoHeader* rtpVideoHdr = NULL) = 0;

    virtual void TimeToSendPacket(uint32_t ssrc, uint16_t sequence_number,
                                  int64_t capture_time_ms) = 0;

    





    


    virtual RTCPMethod RTCP() const = 0;

    






    virtual WebRtc_Word32 SetRTCPStatus(const RTCPMethod method) = 0;

    




    virtual WebRtc_Word32 SetCNAME(const char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual WebRtc_Word32 CNAME(char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual WebRtc_Word32 RemoteCNAME(
        const WebRtc_UWord32 remoteSSRC,
        char cName[RTCP_CNAME_SIZE]) const = 0;

    




    virtual WebRtc_Word32 RemoteNTP(
        WebRtc_UWord32 *ReceivedNTPsecs,
        WebRtc_UWord32 *ReceivedNTPfrac,
        WebRtc_UWord32 *RTCPArrivalTimeSecs,
        WebRtc_UWord32 *RTCPArrivalTimeFrac,
        WebRtc_UWord32 *rtcp_timestamp) const  = 0;

    




    virtual WebRtc_Word32 AddMixedCNAME(
        const WebRtc_UWord32 SSRC,
        const char cName[RTCP_CNAME_SIZE]) = 0;

    




    virtual WebRtc_Word32 RemoveMixedCNAME(const WebRtc_UWord32 SSRC) = 0;

    




    virtual WebRtc_Word32 RTT(const WebRtc_UWord32 remoteSSRC,
                              WebRtc_UWord16* RTT,
                              WebRtc_UWord16* avgRTT,
                              WebRtc_UWord16* minRTT,
                              WebRtc_UWord16* maxRTT) const = 0 ;

    




    virtual WebRtc_Word32 ResetRTT(const WebRtc_UWord32 remoteSSRC)= 0 ;

    



    virtual void SetRtt(uint32_t rtt) = 0;

    





    virtual WebRtc_Word32 SendRTCP(
        WebRtc_UWord32 rtcpPacketType = kRtcpReport) = 0;

    


    virtual WebRtc_Word32 SendRTCPReferencePictureSelection(
        const WebRtc_UWord64 pictureID) = 0;

    



    virtual WebRtc_Word32 SendRTCPSliceLossIndication(
        const WebRtc_UWord8 pictureID) = 0;

    




    virtual WebRtc_Word32 ResetStatisticsRTP() = 0;

    




    virtual WebRtc_Word32 StatisticsRTP(
        WebRtc_UWord8* fraction_lost,  
        WebRtc_UWord32* cum_lost,      
        WebRtc_UWord32* ext_max,       
        WebRtc_UWord32* jitter,
        WebRtc_UWord32* max_jitter = NULL) const = 0;

    




    virtual WebRtc_Word32 ResetReceiveDataCountersRTP() = 0;

    




    virtual WebRtc_Word32 ResetSendDataCountersRTP() = 0;

    




    virtual WebRtc_Word32 DataCountersRTP(
        WebRtc_UWord32* bytesSent,
        WebRtc_UWord32* packetsSent,
        WebRtc_UWord32* bytesReceived,
        WebRtc_UWord32* packetsReceived) const = 0;
    




    virtual WebRtc_Word32 RemoteRTCPStat(RTCPSenderInfo* senderInfo) = 0;

    




    virtual WebRtc_Word32 RemoteRTCPStat(
        std::vector<RTCPReportBlock>* receiveBlocks) const = 0;
    




    virtual WebRtc_Word32 AddRTCPReportBlock(
        const WebRtc_UWord32 SSRC,
        const RTCPReportBlock* receiveBlock) = 0;

    




    virtual WebRtc_Word32 RemoveRTCPReportBlock(const WebRtc_UWord32 SSRC) = 0;

    




    virtual WebRtc_Word32 SetRTCPApplicationSpecificData(
        const WebRtc_UWord8 subType,
        const WebRtc_UWord32 name,
        const WebRtc_UWord8* data,
        const WebRtc_UWord16 length) = 0;
    




    virtual WebRtc_Word32 SetRTCPVoIPMetrics(
        const RTCPVoIPMetric* VoIPMetric) = 0;

    


    virtual bool REMB() const = 0;

    virtual WebRtc_Word32 SetREMBStatus(const bool enable) = 0;

    virtual WebRtc_Word32 SetREMBData(const WebRtc_UWord32 bitrate,
                                      const WebRtc_UWord8 numberOfSSRC,
                                      const WebRtc_UWord32* SSRC) = 0;

    


    virtual bool IJ() const = 0;

    virtual WebRtc_Word32 SetIJStatus(const bool enable) = 0;

    


    virtual bool TMMBR() const = 0;

    



    virtual WebRtc_Word32 SetTMMBRStatus(const bool enable) = 0;

    


    virtual NACKMethod NACK() const  = 0;

    




    virtual WebRtc_Word32 SetNACKStatus(const NACKMethod method) = 0;

    



    virtual int SelectiveRetransmissions() const = 0;

    











    virtual int SetSelectiveRetransmissions(uint8_t settings) = 0;

    




    virtual WebRtc_Word32 SendNACK(const WebRtc_UWord16* nackList,
                                   const WebRtc_UWord16 size) = 0;

    





    virtual WebRtc_Word32 SetStorePacketsStatus(
        const bool enable,
        const WebRtc_UWord16 numberToStore = 200) = 0;

    





    





    virtual WebRtc_Word32 SetAudioPacketSize(
        const WebRtc_UWord16 packetSizeSamples) = 0;

    




    virtual WebRtc_Word32 SetTelephoneEventStatus(
        const bool enable,
        const bool forwardToDecoder,
        const bool detectEndOfTone = false) = 0;

    


    virtual bool TelephoneEvent() const = 0;

    



    virtual bool TelephoneEventForwardToDecoder() const = 0;

    






    virtual bool SendTelephoneEventActive(
        WebRtc_Word8& telephoneEvent) const = 0;

    




    virtual WebRtc_Word32 SendTelephoneEventOutband(
        const WebRtc_UWord8 key,
        const WebRtc_UWord16 time_ms,
        const WebRtc_UWord8 level) = 0;

    




    virtual WebRtc_Word32 SetSendREDPayloadType(
        const WebRtc_Word8 payloadType) = 0;

    




     virtual WebRtc_Word32 SendREDPayloadType(
         WebRtc_Word8& payloadType) const = 0;

     





     virtual WebRtc_Word32 SetRTPAudioLevelIndicationStatus(
         const bool enable,
         const WebRtc_UWord8 ID) = 0;

     




     virtual WebRtc_Word32 GetRTPAudioLevelIndicationStatus(
         bool& enable,
         WebRtc_UWord8& ID) const = 0;

     







     virtual WebRtc_Word32 SetAudioLevel(const WebRtc_UWord8 level_dBov) = 0;

    





    




    virtual WebRtc_Word32 SetCameraDelay(const WebRtc_Word32 delayMS) = 0;

    


    virtual void SetTargetSendBitrate(const WebRtc_UWord32 bitrate) = 0;

    




    virtual WebRtc_Word32 SetGenericFECStatus(
        const bool enable,
        const WebRtc_UWord8 payloadTypeRED,
        const WebRtc_UWord8 payloadTypeFEC) = 0;

    




    virtual WebRtc_Word32 GenericFECStatus(bool& enable,
                                           WebRtc_UWord8& payloadTypeRED,
                                           WebRtc_UWord8& payloadTypeFEC) = 0;


    virtual WebRtc_Word32 SetFecParameters(
        const FecProtectionParams* delta_params,
        const FecProtectionParams* key_params) = 0;

    




    virtual WebRtc_Word32 SetKeyFrameRequestMethod(
        const KeyFrameRequestMethod method) = 0;

    




    virtual WebRtc_Word32 RequestKeyFrame() = 0;
};
} 
#endif 
