









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_RECEIVER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_RECEIVER_H_

#include <map>
#include <vector>

#include "typedefs.h"
#include "rtp_utility.h"
#include "rtcp_utility.h"
#include "rtp_rtcp_defines.h"
#include "rtcp_receiver_help.h"
#include "tmmbr_help.h"

namespace webrtc {
class ModuleRtpRtcpImpl;

class RTCPReceiver : public TMMBRHelp
{
public:
    RTCPReceiver(const WebRtc_Word32 id, RtpRtcpClock* clock,
                 ModuleRtpRtcpImpl* owner);
    virtual ~RTCPReceiver();

    void ChangeUniqueId(const WebRtc_Word32 id);

    RTCPMethod Status() const;
    WebRtc_Word32 SetRTCPStatus(const RTCPMethod method);

    WebRtc_Word64 LastReceived();

    void SetSSRC( const WebRtc_UWord32 ssrc);
    void SetRelaySSRC( const WebRtc_UWord32 ssrc);
    WebRtc_Word32 SetRemoteSSRC( const WebRtc_UWord32 ssrc);

    WebRtc_UWord32 RelaySSRC() const;

    void RegisterRtcpObservers(RtcpIntraFrameObserver* intra_frame_callback,
                               RtcpBandwidthObserver* bandwidth_callback,
                               RtcpFeedback* feedback_callback);

    WebRtc_Word32 IncomingRTCPPacket(RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
                                   RTCPUtility::RTCPParserV2 *rtcpParser);

    void TriggerCallbacksFromRTCPPacket(RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    
    WebRtc_Word32 CNAME(const WebRtc_UWord32 remoteSSRC,
                        char cName[RTCP_CNAME_SIZE]) const;

    
    WebRtc_Word32 NTP(WebRtc_UWord32 *ReceivedNTPsecs,
                      WebRtc_UWord32 *ReceivedNTPfrac,
                      WebRtc_UWord32 *RTCPArrivalTimeSecs,
                      WebRtc_UWord32 *RTCPArrivalTimeFrac,
                      WebRtc_UWord32 *rtcp_timestamp) const;

    
    WebRtc_Word32 RTT(const WebRtc_UWord32 remoteSSRC,
                      WebRtc_UWord16* RTT,
                      WebRtc_UWord16* avgRTT,
                      WebRtc_UWord16* minRTT,
                      WebRtc_UWord16* maxRTT) const;

    WebRtc_UWord16 RTT() const;

    int SetRTT(WebRtc_UWord16 rtt);

    WebRtc_Word32 ResetRTT(const WebRtc_UWord32 remoteSSRC);

    WebRtc_Word32 SenderInfoReceived(RTCPSenderInfo* senderInfo) const;

    
    WebRtc_Word32 StatisticsReceived(
        std::vector<RTCPReportBlock>* receiveBlocks) const;

    
    
    bool RtcpRrTimeout(int64_t rtcp_interval_ms);

    
    
    
    
    bool RtcpRrSequenceNumberTimeout(int64_t rtcp_interval_ms);

    
    WebRtc_Word32 TMMBRReceived(const WebRtc_UWord32 size,
                                const WebRtc_UWord32 accNumCandidates,
                                TMMBRSet* candidateSet) const;

    bool UpdateRTCPReceiveInformationTimers();

    WebRtc_Word32 BoundingSet(bool &tmmbrOwner, TMMBRSet* boundingSetRec);

    WebRtc_Word32 UpdateTMMBR();

    WebRtc_Word32 SetPacketTimeout(const WebRtc_UWord32 timeoutMS);
    void PacketTimeout();

protected:
    RTCPHelp::RTCPReportBlockInformation* CreateReportBlockInformation(const WebRtc_UWord32 remoteSSRC);
    RTCPHelp::RTCPReportBlockInformation* GetReportBlockInformation(const WebRtc_UWord32 remoteSSRC) const;

    RTCPUtility::RTCPCnameInformation* CreateCnameInformation(const WebRtc_UWord32 remoteSSRC);
    RTCPUtility::RTCPCnameInformation* GetCnameInformation(const WebRtc_UWord32 remoteSSRC) const;

    RTCPHelp::RTCPReceiveInformation* CreateReceiveInformation(const WebRtc_UWord32 remoteSSRC);
    RTCPHelp::RTCPReceiveInformation* GetReceiveInformation(const WebRtc_UWord32 remoteSSRC);

    void UpdateReceiveInformation( RTCPHelp::RTCPReceiveInformation& receiveInformation);

    void HandleSenderReceiverReport(RTCPUtility::RTCPParserV2& rtcpParser,
                                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleReportBlock(const RTCPUtility::RTCPPacket& rtcpPacket,
                           RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
                           const WebRtc_UWord32 remoteSSRC,
                           const WebRtc_UWord8 numberOfReportBlocks);

    void HandleSDES(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandleSDESChunk(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandleXRVOIPMetric(RTCPUtility::RTCPParserV2& rtcpParser,
                            RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleNACK(RTCPUtility::RTCPParserV2& rtcpParser,
                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleNACKItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleBYE(RTCPUtility::RTCPParserV2& rtcpParser);

    void HandlePLI(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSLI(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSLIItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleRPSI(RTCPUtility::RTCPParserV2& rtcpParser,
                    RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandlePsfbApp(RTCPUtility::RTCPParserV2& rtcpParser,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleREMBItem(RTCPUtility::RTCPParserV2& rtcpParser,
                        RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleIJ(RTCPUtility::RTCPParserV2& rtcpParser,
                  RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleIJItem(const RTCPUtility::RTCPPacket& rtcpPacket,
                      RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBR(RTCPUtility::RTCPParserV2& rtcpParser,
                     RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBRItem(RTCPHelp::RTCPReceiveInformation& receiveInfo,
                         const RTCPUtility::RTCPPacket& rtcpPacket,
                         RTCPHelp::RTCPPacketInformation& rtcpPacketInformation,
                         const WebRtc_UWord32 senderSSRC);

    void HandleTMMBN(RTCPUtility::RTCPParserV2& rtcpParser,
                     RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleSR_REQ(RTCPUtility::RTCPParserV2& rtcpParser,
                      RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleTMMBNItem(RTCPHelp::RTCPReceiveInformation& receiveInfo,
                         const RTCPUtility::RTCPPacket& rtcpPacket);

    void HandleFIR(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleFIRItem(RTCPHelp::RTCPReceiveInformation* receiveInfo,
                       const RTCPUtility::RTCPPacket& rtcpPacket,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleAPP(RTCPUtility::RTCPParserV2& rtcpParser,
                   RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

    void HandleAPPItem(RTCPUtility::RTCPParserV2& rtcpParser,
                       RTCPHelp::RTCPPacketInformation& rtcpPacketInformation);

 private:
  WebRtc_Word32           _id;
  RtpRtcpClock&           _clock;
  RTCPMethod              _method;
  WebRtc_Word64           _lastReceived;
  ModuleRtpRtcpImpl&      _rtpRtcp;

  CriticalSectionWrapper* _criticalSectionFeedbacks;
  RtcpFeedback*           _cbRtcpFeedback;
  RtcpBandwidthObserver*  _cbRtcpBandwidthObserver;
  RtcpIntraFrameObserver* _cbRtcpIntraFrameObserver;

  CriticalSectionWrapper* _criticalSectionRTCPReceiver;
  WebRtc_UWord32          _SSRC;
  WebRtc_UWord32          _remoteSSRC;

  
  RTCPSenderInfo _remoteSenderInfo;
  
  WebRtc_UWord32 _lastReceivedSRNTPsecs;
  WebRtc_UWord32 _lastReceivedSRNTPfrac;

  
  std::map<WebRtc_UWord32, RTCPHelp::RTCPReportBlockInformation*>
      _receivedReportBlockMap;
  std::map<WebRtc_UWord32, RTCPHelp::RTCPReceiveInformation*>
      _receivedInfoMap;
  std::map<WebRtc_UWord32, RTCPUtility::RTCPCnameInformation*>
      _receivedCnameMap;

  WebRtc_UWord32            _packetTimeOutMS;

  
  int64_t _lastReceivedRrMs;

  
  
  int64_t _lastIncreasedSequenceNumberMs;

  
  
  WebRtc_UWord16 _rtt;

};
} 
#endif 
