









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_IMPL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_IMPL_H_

#include <list>

#include "modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "modules/rtp_rtcp/source/rtcp_receiver.h"
#include "modules/rtp_rtcp/source/rtcp_sender.h"
#include "modules/rtp_rtcp/source/rtp_receiver.h"
#include "modules/rtp_rtcp/source/rtp_sender.h"
#include "system_wrappers/interface/scoped_ptr.h"

#ifdef MATLAB
class MatlabPlot;
#endif

namespace webrtc {

class ModuleRtpRtcpImpl : public RtpRtcp {
 public:
    explicit ModuleRtpRtcpImpl(const RtpRtcp::Configuration& configuration);

    virtual ~ModuleRtpRtcpImpl();

    
    virtual WebRtc_Word32 TimeUntilNextProcess();

    
    virtual WebRtc_Word32 Process();

    


    
    virtual WebRtc_Word32 SetPacketTimeout(const WebRtc_UWord32 RTPtimeoutMS,
                                           const WebRtc_UWord32 RTCPtimeoutMS);

    
    virtual WebRtc_Word32 SetPeriodicDeadOrAliveStatus(
        const bool enable,
        const WebRtc_UWord8 sampleTimeSeconds);

    
    virtual WebRtc_Word32 PeriodicDeadOrAliveStatus(
        bool &enable,
        WebRtc_UWord8 &sampleTimeSeconds);

    virtual WebRtc_Word32 RegisterReceivePayload(const CodecInst& voiceCodec);

    virtual WebRtc_Word32 RegisterReceivePayload(const VideoCodec& videoCodec);

    virtual WebRtc_Word32 ReceivePayloadType(const CodecInst& voiceCodec,
                                             WebRtc_Word8* plType);

    virtual WebRtc_Word32 ReceivePayloadType(const VideoCodec& videoCodec,
                                             WebRtc_Word8* plType);

    virtual WebRtc_Word32 DeRegisterReceivePayload(
        const WebRtc_Word8 payloadType);

    
    virtual WebRtc_Word32 RegisterReceiveRtpHeaderExtension(
        const RTPExtensionType type,
        const WebRtc_UWord8 id);

    virtual WebRtc_Word32 DeregisterReceiveRtpHeaderExtension(
        const RTPExtensionType type);

    
    virtual WebRtc_Word32 SSRCFilter(WebRtc_UWord32& allowedSSRC) const;

    
    virtual WebRtc_Word32 SetSSRCFilter(const bool enable, const WebRtc_UWord32 allowedSSRC);

    
    virtual WebRtc_UWord32 RemoteTimestamp() const;

    
    virtual int64_t LocalTimeOfRemoteTimeStamp() const;

    
    virtual WebRtc_Word32 EstimatedRemoteTimeStamp(WebRtc_UWord32& timestamp) const;

    virtual WebRtc_UWord32 RemoteSSRC() const;

    virtual WebRtc_Word32 RemoteCSRCs( WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize]) const ;

    virtual WebRtc_Word32 SetRTXReceiveStatus(const bool enable,
                                              const WebRtc_UWord32 SSRC);

    virtual WebRtc_Word32 RTXReceiveStatus(bool* enable,
                                           WebRtc_UWord32* SSRC) const;

    
    virtual WebRtc_Word32 IncomingPacket( const WebRtc_UWord8* incomingPacket,
                                        const WebRtc_UWord16 packetLength);

    


    virtual WebRtc_Word32 RegisterSendPayload(const CodecInst& voiceCodec);

    virtual WebRtc_Word32 RegisterSendPayload(const VideoCodec& videoCodec);

    virtual WebRtc_Word32 DeRegisterSendPayload(const WebRtc_Word8 payloadType);

    virtual WebRtc_Word8 SendPayloadType() const;

    
    virtual WebRtc_Word32 RegisterSendRtpHeaderExtension(
        const RTPExtensionType type,
        const WebRtc_UWord8 id);

    virtual WebRtc_Word32 DeregisterSendRtpHeaderExtension(
        const RTPExtensionType type);

    
    virtual WebRtc_UWord32 StartTimestamp() const;

    
    virtual WebRtc_Word32 SetStartTimestamp(const WebRtc_UWord32 timestamp);

    virtual WebRtc_UWord16 SequenceNumber() const;

    
    virtual WebRtc_Word32 SetSequenceNumber(const WebRtc_UWord16 seq);

    virtual WebRtc_UWord32 SSRC() const;

    
    virtual WebRtc_Word32 SetSSRC(const WebRtc_UWord32 ssrc);

    virtual WebRtc_Word32 CSRCs( WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize]) const ;

    virtual WebRtc_Word32 SetCSRCs( const WebRtc_UWord32 arrOfCSRC[kRtpCsrcSize],
                                  const WebRtc_UWord8 arrLength);

    virtual WebRtc_Word32 SetCSRCStatus(const bool include);

    virtual WebRtc_UWord32 PacketCountSent() const;

    virtual int CurrentSendFrequencyHz() const;

    virtual WebRtc_UWord32 ByteCountSent() const;

    virtual WebRtc_Word32 SetRTXSendStatus(const bool enable,
                                           const bool setSSRC,
                                           const WebRtc_UWord32 SSRC);

    virtual WebRtc_Word32 RTXSendStatus(bool* enable,
                                        WebRtc_UWord32* SSRC) const;

    
    virtual WebRtc_Word32 SetSendingStatus(const bool sending);

    virtual bool Sending() const;

    
    virtual WebRtc_Word32 SetSendingMediaStatus(const bool sending);

    virtual bool SendingMedia() const;

    
    virtual WebRtc_Word32 SendOutgoingData(
        const FrameType frameType,
        const WebRtc_Word8 payloadType,
        const WebRtc_UWord32 timeStamp,
        int64_t capture_time_ms,
        const WebRtc_UWord8* payloadData,
        const WebRtc_UWord32 payloadSize,
        const RTPFragmentationHeader* fragmentation = NULL,
        const RTPVideoHeader* rtpVideoHdr = NULL);

    virtual void TimeToSendPacket(uint32_t ssrc, uint16_t sequence_number,
                                  int64_t capture_time_ms);
    



    
    virtual RTCPMethod RTCP() const;

    
    virtual WebRtc_Word32 SetRTCPStatus(const RTCPMethod method);

    
    virtual WebRtc_Word32 SetCNAME(const char cName[RTCP_CNAME_SIZE]);

    
    virtual WebRtc_Word32 CNAME(char cName[RTCP_CNAME_SIZE]);

    
    virtual WebRtc_Word32 RemoteCNAME(const WebRtc_UWord32 remoteSSRC,
                                      char cName[RTCP_CNAME_SIZE]) const;

    
    virtual WebRtc_Word32 RemoteNTP(WebRtc_UWord32 *ReceivedNTPsecs,
                                  WebRtc_UWord32 *ReceivedNTPfrac,
                                  WebRtc_UWord32 *RTCPArrivalTimeSecs,
                                  WebRtc_UWord32 *RTCPArrivalTimeFrac,
                                  WebRtc_UWord32 *rtcp_timestamp) const;

    virtual WebRtc_Word32 AddMixedCNAME(const WebRtc_UWord32 SSRC,
                                        const char cName[RTCP_CNAME_SIZE]);

    virtual WebRtc_Word32 RemoveMixedCNAME(const WebRtc_UWord32 SSRC);

    
    virtual WebRtc_Word32 RTT(const WebRtc_UWord32 remoteSSRC,
                            WebRtc_UWord16* RTT,
                            WebRtc_UWord16* avgRTT,
                            WebRtc_UWord16* minRTT,
                            WebRtc_UWord16* maxRTT) const;

    
    virtual WebRtc_Word32 ResetRTT(const WebRtc_UWord32 remoteSSRC);

    virtual void SetRtt(uint32_t rtt);

    
    
    virtual WebRtc_Word32 SendRTCP(WebRtc_UWord32 rtcpPacketType = kRtcpReport);

    
    virtual WebRtc_Word32 StatisticsRTP(WebRtc_UWord8  *fraction_lost,
                                      WebRtc_UWord32 *cum_lost,
                                      WebRtc_UWord32 *ext_max,
                                      WebRtc_UWord32 *jitter,
                                      WebRtc_UWord32 *max_jitter = NULL) const;

    
    virtual WebRtc_Word32 ResetStatisticsRTP();

    virtual WebRtc_Word32 ResetReceiveDataCountersRTP();

    virtual WebRtc_Word32 ResetSendDataCountersRTP();

    
    virtual WebRtc_Word32 DataCountersRTP(WebRtc_UWord32 *bytesSent,
                                          WebRtc_UWord32 *packetsSent,
                                          WebRtc_UWord32 *bytesReceived,
                                          WebRtc_UWord32 *packetsReceived) const;

    virtual WebRtc_Word32 ReportBlockStatistics(
        WebRtc_UWord8 *fraction_lost,
        WebRtc_UWord32 *cum_lost,
        WebRtc_UWord32 *ext_max,
        WebRtc_UWord32 *jitter,
        WebRtc_UWord32 *jitter_transmission_time_offset);

    
    virtual WebRtc_Word32 RemoteRTCPStat( RTCPSenderInfo* senderInfo);

    
    virtual WebRtc_Word32 RemoteRTCPStat(
        std::vector<RTCPReportBlock>* receiveBlocks) const;

    
    virtual WebRtc_Word32 AddRTCPReportBlock(const WebRtc_UWord32 SSRC,
                                           const RTCPReportBlock* receiveBlock);

    virtual WebRtc_Word32 RemoveRTCPReportBlock(const WebRtc_UWord32 SSRC);

    


    virtual bool REMB() const;

    virtual WebRtc_Word32 SetREMBStatus(const bool enable);

    virtual WebRtc_Word32 SetREMBData(const WebRtc_UWord32 bitrate,
                                      const WebRtc_UWord8 numberOfSSRC,
                                      const WebRtc_UWord32* SSRC);

    


    virtual bool IJ() const;

    virtual WebRtc_Word32 SetIJStatus(const bool enable);

    


    virtual bool TMMBR() const ;

    virtual WebRtc_Word32 SetTMMBRStatus(const bool enable);

    WebRtc_Word32 SetTMMBN(const TMMBRSet* boundingSet);

    virtual WebRtc_UWord16 MaxPayloadLength() const;

    virtual WebRtc_UWord16 MaxDataPayloadLength() const;

    virtual WebRtc_Word32 SetMaxTransferUnit(const WebRtc_UWord16 size);

    virtual WebRtc_Word32 SetTransportOverhead(const bool TCP,
                                             const bool IPV6,
                                             const WebRtc_UWord8 authenticationOverhead = 0);

    



    
    virtual NACKMethod NACK() const ;

    
    virtual WebRtc_Word32 SetNACKStatus(const NACKMethod method);

    virtual int SelectiveRetransmissions() const;

    virtual int SetSelectiveRetransmissions(uint8_t settings);

    
    virtual WebRtc_Word32 SendNACK(const WebRtc_UWord16* nackList,
                                   const WebRtc_UWord16 size);

    
    virtual WebRtc_Word32 SetStorePacketsStatus(const bool enable, const WebRtc_UWord16 numberToStore = 200);

    


    virtual WebRtc_Word32 SetRTCPApplicationSpecificData(const WebRtc_UWord8 subType,
                                                       const WebRtc_UWord32 name,
                                                       const WebRtc_UWord8* data,
                                                       const WebRtc_UWord16 length);
    


    virtual WebRtc_Word32 SetRTCPVoIPMetrics(const RTCPVoIPMetric* VoIPMetric);

    



    
    virtual WebRtc_Word32 SetAudioPacketSize(const WebRtc_UWord16 packetSizeSamples);

    
    virtual WebRtc_Word32 SetTelephoneEventStatus(const bool enable,
                                                const bool forwardToDecoder,
                                                const bool detectEndOfTone = false);

    
    virtual bool TelephoneEvent() const;

    
    virtual bool TelephoneEventForwardToDecoder() const;

    virtual bool SendTelephoneEventActive(WebRtc_Word8& telephoneEvent) const;

    
    virtual WebRtc_Word32 SendTelephoneEventOutband(const WebRtc_UWord8 key,
                                                  const WebRtc_UWord16 time_ms,
                                                  const WebRtc_UWord8 level);

    
    virtual WebRtc_Word32 SetSendREDPayloadType(const WebRtc_Word8 payloadType);

    
    virtual WebRtc_Word32 SendREDPayloadType(WebRtc_Word8& payloadType) const;

    
    virtual WebRtc_Word32 SetRTPAudioLevelIndicationStatus(const bool enable,
                                                         const WebRtc_UWord8 ID);

    
    virtual WebRtc_Word32 GetRTPAudioLevelIndicationStatus(bool& enable,
                                                         WebRtc_UWord8& ID) const;

    
    virtual WebRtc_Word32 SetAudioLevel(const WebRtc_UWord8 level_dBov);

    


    virtual RtpVideoCodecTypes ReceivedVideoCodec() const;

    virtual RtpVideoCodecTypes SendVideoCodec() const;

    virtual WebRtc_Word32 SendRTCPSliceLossIndication(const WebRtc_UWord8 pictureID);

    
    virtual WebRtc_Word32 SetKeyFrameRequestMethod(const KeyFrameRequestMethod method);

    
    virtual WebRtc_Word32 RequestKeyFrame();

    virtual WebRtc_Word32 SetCameraDelay(const WebRtc_Word32 delayMS);

    virtual void SetTargetSendBitrate(const WebRtc_UWord32 bitrate);

    virtual WebRtc_Word32 SetGenericFECStatus(const bool enable,
                                            const WebRtc_UWord8 payloadTypeRED,
                                            const WebRtc_UWord8 payloadTypeFEC);

    virtual WebRtc_Word32 GenericFECStatus(bool& enable,
                                         WebRtc_UWord8& payloadTypeRED,
                                         WebRtc_UWord8& payloadTypeFEC);

    virtual WebRtc_Word32 SetFecParameters(
        const FecProtectionParams* delta_params,
        const FecProtectionParams* key_params);

    virtual WebRtc_Word32 LastReceivedNTP(WebRtc_UWord32& NTPsecs,
                                          WebRtc_UWord32& NTPfrac,
                                          WebRtc_UWord32& remoteSR);

    virtual WebRtc_Word32 BoundingSet(bool &tmmbrOwner,
                                      TMMBRSet*& boundingSetRec);

    virtual void BitrateSent(WebRtc_UWord32* totalRate,
                             WebRtc_UWord32* videoRate,
                             WebRtc_UWord32* fecRate,
                             WebRtc_UWord32* nackRate) const;

    virtual int EstimatedReceiveBandwidth(
        WebRtc_UWord32* available_bandwidth) const;

    virtual void SetRemoteSSRC(const WebRtc_UWord32 SSRC);

    virtual WebRtc_UWord32 SendTimeOfSendReport(const WebRtc_UWord32 sendReport);

    
    virtual WebRtc_Word32 SendRTCPReferencePictureSelection(const WebRtc_UWord64 pictureID);

    void OnReceivedTMMBR();

    
    void OnRequestIntraFrame();

    
    void OnReceivedSliceLossIndication(const WebRtc_UWord8 pictureID);

    
    void OnReceivedReferencePictureSelectionIndication(
        const WebRtc_UWord64 pitureID);

    void OnReceivedNACK(const WebRtc_UWord16 nackSequenceNumbersLength,
                        const WebRtc_UWord16* nackSequenceNumbers);

    void OnRequestSendReport();

    
    
    void OwnsClock() { _owns_clock = true; }

protected:
    void RegisterChildModule(RtpRtcp* module);

    void DeRegisterChildModule(RtpRtcp* module);

    bool UpdateRTCPReceiveInformationTimers();

    void ProcessDeadOrAliveTimer();

    WebRtc_UWord32 BitrateReceivedNow() const;

    
    WebRtc_UWord16 RemoteSequenceNumber() const;

    
    WebRtc_UWord32 LastSendReport(WebRtc_UWord32& lastRTCPTime);

    RTPSender                 _rtpSender;
    RTPReceiver               _rtpReceiver;

    RTCPSender                _rtcpSender;
    RTCPReceiver              _rtcpReceiver;

    bool                      _owns_clock;
    RtpRtcpClock&             _clock;
private:
    int64_t RtcpReportInterval();

    WebRtc_Word32             _id;
    const bool                _audio;
    bool                      _collisionDetected;
    WebRtc_Word64             _lastProcessTime;
    WebRtc_Word64             _lastBitrateProcessTime;
    WebRtc_Word64             _lastPacketTimeoutProcessTime;
    WebRtc_UWord16            _packetOverHead;

    scoped_ptr<CriticalSectionWrapper> _criticalSectionModulePtrs;
    scoped_ptr<CriticalSectionWrapper> _criticalSectionModulePtrsFeedback;
    ModuleRtpRtcpImpl*            _defaultModule;
    std::list<ModuleRtpRtcpImpl*> _childModules;

    
    bool                  _deadOrAliveActive;
    WebRtc_UWord32        _deadOrAliveTimeoutMS;
    WebRtc_Word64        _deadOrAliveLastTimer;
    
    NACKMethod            _nackMethod;
    WebRtc_UWord32        _nackLastTimeSent;
    WebRtc_UWord16        _nackLastSeqNumberSent;

    bool                  _simulcast;
    VideoCodec            _sendVideoCodec;
    KeyFrameRequestMethod _keyFrameReqMethod;

    RemoteBitrateEstimator* remote_bitrate_;

    RtcpRttObserver* rtt_observer_;

#ifdef MATLAB
    MatlabPlot*           _plot1;
#endif
};
} 
#endif 
