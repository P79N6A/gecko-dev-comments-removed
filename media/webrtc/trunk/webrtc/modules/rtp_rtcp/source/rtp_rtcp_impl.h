









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_IMPL_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_IMPL_H_

#include <list>
#include <vector>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_receiver.h"
#include "webrtc/modules/rtp_rtcp/source/rtcp_sender.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_sender.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/gtest_prod_util.h"

#ifdef MATLAB
class MatlabPlot;
#endif

namespace webrtc {

class ModuleRtpRtcpImpl : public RtpRtcp {
 public:
  explicit ModuleRtpRtcpImpl(const RtpRtcp::Configuration& configuration);

  virtual ~ModuleRtpRtcpImpl();

  
  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;

  
  virtual int32_t Process() OVERRIDE;

  

  
  virtual int32_t IncomingRtcpPacket(const uint8_t* incoming_packet,
                                     uint16_t incoming_packet_length) OVERRIDE;

  virtual void SetRemoteSSRC(const uint32_t ssrc);

  

  virtual int32_t RegisterSendPayload(const CodecInst& voice_codec) OVERRIDE;

  virtual int32_t RegisterSendPayload(const VideoCodec& video_codec) OVERRIDE;

  virtual int32_t DeRegisterSendPayload(const int8_t payload_type) OVERRIDE;

  virtual int8_t SendPayloadType() const;

  
  virtual int32_t RegisterSendRtpHeaderExtension(
      const RTPExtensionType type,
      const uint8_t id) OVERRIDE;

  virtual int32_t DeregisterSendRtpHeaderExtension(
      const RTPExtensionType type) OVERRIDE;

  
  virtual uint32_t StartTimestamp() const OVERRIDE;

  
  virtual int32_t SetStartTimestamp(const uint32_t timestamp) OVERRIDE;

  virtual uint16_t SequenceNumber() const OVERRIDE;

  
  virtual int32_t SetSequenceNumber(const uint16_t seq) OVERRIDE;

  virtual uint32_t SSRC() const OVERRIDE;

  
  virtual int32_t SetSSRC(const uint32_t ssrc) OVERRIDE;

  virtual int32_t CSRCs(uint32_t arr_of_csrc[kRtpCsrcSize]) const OVERRIDE;

  virtual int32_t SetCSRCs(const uint32_t arr_of_csrc[kRtpCsrcSize],
                           const uint8_t arr_length) OVERRIDE;

  virtual int32_t SetCSRCStatus(const bool include) OVERRIDE;

  virtual uint32_t PacketCountSent() const;

  virtual int CurrentSendFrequencyHz() const;

  virtual uint32_t ByteCountSent() const;

  virtual int32_t SetRTXSendStatus(const int mode,
                                   const bool set_ssrc,
                                   const uint32_t ssrc) OVERRIDE;

  virtual int32_t RTXSendStatus(int* mode, uint32_t* ssrc,
                                int* payloadType) const OVERRIDE;


  virtual void SetRtxSendPayloadType(int payload_type) OVERRIDE;

  
  virtual int32_t SetSendingStatus(const bool sending) OVERRIDE;

  virtual bool Sending() const OVERRIDE;

  
  virtual int32_t SetSendingMediaStatus(const bool sending) OVERRIDE;

  virtual bool SendingMedia() const OVERRIDE;

  
  
  virtual int32_t SendOutgoingData(
      const FrameType frame_type,
      const int8_t payload_type,
      const uint32_t time_stamp,
      int64_t capture_time_ms,
      const uint8_t* payload_data,
      const uint32_t payload_size,
      const RTPFragmentationHeader* fragmentation = NULL,
      const RTPVideoHeader* rtp_video_hdr = NULL) OVERRIDE;

  virtual bool TimeToSendPacket(uint32_t ssrc,
                                uint16_t sequence_number,
                                int64_t capture_time_ms,
                                bool retransmission) OVERRIDE;
  
  
  virtual int TimeToSendPadding(int bytes) OVERRIDE;

  virtual bool GetSendSideDelay(int* avg_send_delay_ms,
                                int* max_send_delay_ms) const OVERRIDE;

  

  
  virtual RTCPMethod RTCP() const OVERRIDE;

  
  virtual int32_t SetRTCPStatus(const RTCPMethod method) OVERRIDE;

  
  virtual int32_t SetCNAME(const char c_name[RTCP_CNAME_SIZE]) OVERRIDE;

  
  virtual int32_t CNAME(char c_name[RTCP_CNAME_SIZE]) OVERRIDE;

  
  virtual int32_t RemoteCNAME(const uint32_t remote_ssrc,
                              char c_name[RTCP_CNAME_SIZE]) const OVERRIDE;

  
  virtual int32_t RemoteNTP(uint32_t* received_ntp_secs,
                            uint32_t* received_ntp_frac,
                            uint32_t* rtcp_arrival_time_secs,
                            uint32_t* rtcp_arrival_time_frac,
                            uint32_t* rtcp_timestamp) const OVERRIDE;

  virtual int32_t AddMixedCNAME(const uint32_t ssrc,
                                const char c_name[RTCP_CNAME_SIZE]) OVERRIDE;

  virtual int32_t RemoveMixedCNAME(const uint32_t ssrc) OVERRIDE;

  
  virtual int32_t RTT(const uint32_t remote_ssrc,
                      uint16_t* rtt,
                      uint16_t* avg_rtt,
                      uint16_t* min_rtt,
                      uint16_t* max_rtt) const OVERRIDE;

  
  virtual int32_t ResetRTT(const uint32_t remote_ssrc) OVERRIDE;

  
  
  virtual int32_t SendRTCP(uint32_t rtcp_packet_type = kRtcpReport) OVERRIDE;

  virtual int32_t ResetSendDataCountersRTP() OVERRIDE;

  
  virtual int32_t DataCountersRTP(uint32_t* bytes_sent,
                                  uint32_t* packets_sent) const OVERRIDE;

  
  virtual int32_t RemoteRTCPStat(RTCPSenderInfo* sender_info) OVERRIDE;

  
  virtual int32_t RemoteRTCPStat(
      std::vector<RTCPReportBlock>* receive_blocks) const OVERRIDE;

  
  virtual int32_t AddRTCPReportBlock(
    const uint32_t ssrc, const RTCPReportBlock* receive_block) OVERRIDE;

  virtual int32_t RemoveRTCPReportBlock(const uint32_t ssrc) OVERRIDE;

  
  virtual bool REMB() const OVERRIDE;

  virtual int32_t SetREMBStatus(const bool enable) OVERRIDE;

  virtual int32_t SetREMBData(const uint32_t bitrate,
                              const uint8_t number_of_ssrc,
                              const uint32_t* ssrc) OVERRIDE;

  
  virtual bool IJ() const OVERRIDE;

  virtual int32_t SetIJStatus(const bool enable) OVERRIDE;

  
  virtual bool TMMBR() const OVERRIDE;

  virtual int32_t SetTMMBRStatus(const bool enable) OVERRIDE;

  int32_t SetTMMBN(const TMMBRSet* bounding_set);

  virtual uint16_t MaxPayloadLength() const OVERRIDE;

  virtual uint16_t MaxDataPayloadLength() const OVERRIDE;

  virtual int32_t SetMaxTransferUnit(const uint16_t size) OVERRIDE;

  virtual int32_t SetTransportOverhead(
      const bool tcp,
      const bool ipv6,
      const uint8_t authentication_overhead = 0) OVERRIDE;

  

  virtual int SelectiveRetransmissions() const OVERRIDE;

  virtual int SetSelectiveRetransmissions(uint8_t settings) OVERRIDE;

  
  virtual int32_t SendNACK(const uint16_t* nack_list,
                           const uint16_t size) OVERRIDE;

  
  
  virtual int32_t SetStorePacketsStatus(
      const bool enable, const uint16_t number_to_store) OVERRIDE;

  virtual bool StorePackets() const OVERRIDE;

  
  virtual void RegisterSendChannelRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback) OVERRIDE;
  virtual RtcpStatisticsCallback*
      GetSendChannelRtcpStatisticsCallback() OVERRIDE;

  
  virtual int32_t SetRTCPApplicationSpecificData(
      const uint8_t sub_type,
      const uint32_t name,
      const uint8_t* data,
      const uint16_t length) OVERRIDE;

  
  virtual int32_t SetRTCPVoIPMetrics(const RTCPVoIPMetric* VoIPMetric) OVERRIDE;

  
  virtual void SetRtcpXrRrtrStatus(bool enable) OVERRIDE;

  virtual bool RtcpXrRrtrStatus() const OVERRIDE;

  

  
  
  virtual int32_t SetAudioPacketSize(
      const uint16_t packet_size_samples) OVERRIDE;

  virtual bool SendTelephoneEventActive(int8_t& telephone_event) const OVERRIDE;

  
  virtual int32_t SendTelephoneEventOutband(const uint8_t key,
                                            const uint16_t time_ms,
                                            const uint8_t level) OVERRIDE;

  
  virtual int32_t SetSendREDPayloadType(const int8_t payload_type) OVERRIDE;

  
  virtual int32_t SendREDPayloadType(int8_t& payload_type) const OVERRIDE;

  
  virtual int32_t SetRTPAudioLevelIndicationStatus(
      const bool enable, const uint8_t id) OVERRIDE;

  
  virtual int32_t GetRTPAudioLevelIndicationStatus(
      bool& enable, uint8_t& id) const OVERRIDE;

  
  
  virtual int32_t SetAudioLevel(const uint8_t level_d_bov) OVERRIDE;

  

  virtual RtpVideoCodecTypes SendVideoCodec() const;

  virtual int32_t SendRTCPSliceLossIndication(
      const uint8_t picture_id) OVERRIDE;

  
  virtual int32_t SetKeyFrameRequestMethod(
      const KeyFrameRequestMethod method) OVERRIDE;

  
  virtual int32_t RequestKeyFrame() OVERRIDE;

  virtual int32_t SetCameraDelay(const int32_t delay_ms) OVERRIDE;

  virtual void SetTargetSendBitrate(
      const std::vector<uint32_t>& stream_bitrates) OVERRIDE;

  virtual int32_t SetGenericFECStatus(
      const bool enable,
      const uint8_t payload_type_red,
      const uint8_t payload_type_fec) OVERRIDE;

  virtual int32_t GenericFECStatus(
      bool& enable,
      uint8_t& payload_type_red,
      uint8_t& payload_type_fec) OVERRIDE;

  virtual int32_t SetFecParameters(
      const FecProtectionParams* delta_params,
      const FecProtectionParams* key_params) OVERRIDE;

  virtual int32_t LastReceivedNTP(uint32_t& NTPsecs,
                                  uint32_t& NTPfrac,
                                  uint32_t& remote_sr);

  virtual bool LastReceivedXrReferenceTimeInfo(RtcpReceiveTimeInfo* info) const;

  virtual int32_t BoundingSet(bool& tmmbr_owner, TMMBRSet*& bounding_set_rec);

  virtual void BitrateSent(uint32_t* total_rate,
                           uint32_t* video_rate,
                           uint32_t* fec_rate,
                           uint32_t* nackRate) const OVERRIDE;

  virtual void RegisterVideoBitrateObserver(BitrateStatisticsObserver* observer)
      OVERRIDE;

  virtual BitrateStatisticsObserver* GetVideoBitrateObserver() const OVERRIDE;

  virtual uint32_t SendTimeOfSendReport(const uint32_t send_report);

  virtual bool SendTimeOfXrRrReport(uint32_t mid_ntp, int64_t* time_ms) const;

  
  virtual int32_t SendRTCPReferencePictureSelection(
      const uint64_t picture_id) OVERRIDE;

  virtual void RegisterSendChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback);
  virtual StreamDataCountersCallback*
      GetSendChannelRtpStatisticsCallback() const;

  void OnReceivedTMMBR();

  
  void OnRequestIntraFrame();

  
  void OnReceivedSliceLossIndication(const uint8_t picture_id);

  
  void OnReceivedReferencePictureSelectionIndication(
      const uint64_t picture_id);

  void OnReceivedNACK(const std::list<uint16_t>& nack_sequence_numbers);

  void OnRequestSendReport();

  virtual void RegisterSendFrameCountObserver(
      FrameCountObserver* observer) OVERRIDE;
  virtual FrameCountObserver* GetSendFrameCountObserver() const OVERRIDE;

 protected:
  void RegisterChildModule(RtpRtcp* module);

  void DeRegisterChildModule(RtpRtcp* module);

  bool UpdateRTCPReceiveInformationTimers();

  uint32_t BitrateReceivedNow() const;

  
  uint16_t RemoteSequenceNumber() const;

  
  uint32_t LastSendReport(uint32_t& last_rtcptime);

  RTPSender                 rtp_sender_;

  RTCPSender                rtcp_sender_;
  RTCPReceiver              rtcp_receiver_;

  Clock*                    clock_;

 private:
  FRIEND_TEST_ALL_PREFIXES(RtpRtcpImplTest, Rtt);
  FRIEND_TEST_ALL_PREFIXES(RtpRtcpImplTest, RttForReceiverOnly);
  int64_t RtcpReportInterval();
  void SetRtcpReceiverSsrcs(uint32_t main_ssrc);

  void set_rtt_ms(uint32_t rtt_ms);
  uint32_t rtt_ms() const;

  bool IsDefaultModule() const;

  int32_t             id_;
  const bool                audio_;
  bool                      collision_detected_;
  int64_t             last_process_time_;
  int64_t             last_bitrate_process_time_;
  int64_t             last_rtt_process_time_;
  uint16_t            packet_overhead_;

  scoped_ptr<CriticalSectionWrapper> critical_section_module_ptrs_;
  scoped_ptr<CriticalSectionWrapper> critical_section_module_ptrs_feedback_;
  ModuleRtpRtcpImpl*            default_module_;
  std::list<ModuleRtpRtcpImpl*> child_modules_;

  
  NACKMethod            nack_method_;
  uint32_t        nack_last_time_sent_full_;
  uint16_t        nack_last_seq_number_sent_;

  bool                  simulcast_;
  VideoCodec            send_video_codec_;
  KeyFrameRequestMethod key_frame_req_method_;

  RemoteBitrateEstimator* remote_bitrate_;

#ifdef MATLAB
  MatlabPlot*           plot1_;
#endif

  RtcpRttStats* rtt_stats_;

  
  scoped_ptr<CriticalSectionWrapper> critical_section_rtt_;
  uint32_t rtt_ms_;
};

}  

#endif  
