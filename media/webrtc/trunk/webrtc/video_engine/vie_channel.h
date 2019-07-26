









#ifndef WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_H_
#define WEBRTC_VIDEO_ENGINE_VIE_CHANNEL_H_

#include <list>

#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_frame_provider_base.h"
#include "webrtc/video_engine/vie_receiver.h"
#include "webrtc/video_engine/vie_sender.h"
#include "webrtc/video_engine/vie_sync_module.h"

namespace webrtc {

class CallStatsObserver;
class ChannelStatsObserver;
class Config;
class CriticalSectionWrapper;
class EncodedImageCallback;
class I420FrameCallback;
class PacedSender;
class ProcessThread;
class RtcpRttStats;
class RtpRtcp;
class ThreadWrapper;
class ViEDecoderObserver;
class ViEEffectFilter;
class ViERTCPObserver;
class ViERTPObserver;
class VideoCodingModule;
class VideoDecoder;
class VideoRenderCallback;
class VoEVideoSync;

struct SenderInfo;

class ViEChannel
    : public VCMFrameTypeCallback,
      public VCMReceiveCallback,
      public VCMReceiveStatisticsCallback,
      public VCMDecoderTimingCallback,
      public VCMPacketRequestCallback,
      public RtcpFeedback,
      public RtpFeedback,
      public ViEFrameProviderBase {
 public:
  friend class ChannelStatsObserver;

  ViEChannel(int32_t channel_id,
             int32_t engine_id,
             uint32_t number_of_cores,
             const Config& config,
             ProcessThread& module_process_thread,
             RtcpIntraFrameObserver* intra_frame_observer,
             RtcpBandwidthObserver* bandwidth_observer,
             RemoteBitrateEstimator* remote_bitrate_estimator,
             RtcpRttStats* rtt_stats,
             PacedSender* paced_sender,
             RtpRtcp* default_rtp_rtcp,
             bool sender);
  ~ViEChannel();

  int32_t Init();

  
  
  int32_t SetSendCodec(const VideoCodec& video_codec, bool new_stream = true);
  int32_t SetReceiveCodec(const VideoCodec& video_codec);
  int32_t GetReceiveCodec(VideoCodec* video_codec);
  int32_t RegisterCodecObserver(ViEDecoderObserver* observer);
  
  
  
  
  int32_t RegisterExternalDecoder(const uint8_t pl_type,
                                  VideoDecoder* decoder,
                                  bool buffered_rendering,
                                  int32_t render_delay);
  int32_t DeRegisterExternalDecoder(const uint8_t pl_type);
  int32_t ReceiveCodecStatistics(uint32_t* num_key_frames,
                                 uint32_t* num_delta_frames);
  uint32_t DiscardedPackets() const;

  
  int ReceiveDelay() const;

  
  int32_t WaitForKeyFrame(bool wait);

  
  
  
  int32_t SetSignalPacketLossStatus(bool enable, bool only_key_frames);

  int32_t SetRTCPMode(const RTCPMethod rtcp_mode);
  int32_t GetRTCPMode(RTCPMethod* rtcp_mode);
  int32_t SetNACKStatus(const bool enable);
  int32_t SetFECStatus(const bool enable,
                       const unsigned char payload_typeRED,
                       const unsigned char payload_typeFEC);
  int32_t SetHybridNACKFECStatus(const bool enable,
                                 const unsigned char payload_typeRED,
                                 const unsigned char payload_typeFEC);
  int SetSenderBufferingMode(int target_delay_ms);
  int SetReceiverBufferingMode(int target_delay_ms);
  int32_t SetKeyFrameRequestMethod(const KeyFrameRequestMethod method);
  bool EnableRemb(bool enable);
  int SetSendTimestampOffsetStatus(bool enable, int id);
  int SetReceiveTimestampOffsetStatus(bool enable, int id);
  int SetSendAbsoluteSendTimeStatus(bool enable, int id);
  int SetReceiveAbsoluteSendTimeStatus(bool enable, int id);
  bool GetReceiveAbsoluteSendTimeStatus() const;
  void SetRtcpXrRrtrStatus(bool enable);
  void SetTransmissionSmoothingStatus(bool enable);
  int32_t EnableTMMBR(const bool enable);
  int32_t EnableKeyFrameRequestCallback(const bool enable);

  
  int32_t SetSSRC(const uint32_t SSRC,
                  const StreamType usage,
                  const unsigned char simulcast_idx);

  
  int32_t GetLocalSSRC(uint8_t idx, unsigned int* ssrc);

  
  int32_t GetRemoteSSRC(uint32_t* ssrc);

  
  int32_t GetRemoteCSRC(uint32_t CSRCs[kRtpCsrcSize]);

  int SetRtxSendPayloadType(int payload_type);
  void SetRtxReceivePayloadType(int payload_type);

  
  int32_t SetStartSequenceNumber(uint16_t sequence_number);

  
  int32_t SetRTCPCName(const char rtcp_cname[]);

  
  int32_t GetRTCPCName(char rtcp_cname[]);

  
  int32_t GetRemoteRTCPCName(char rtcp_cname[]);

  int32_t RegisterRtpObserver(ViERTPObserver* observer);
  int32_t RegisterRtcpObserver(ViERTCPObserver* observer);
  int32_t SendApplicationDefinedRTCPPacket(
      const uint8_t sub_type,
      uint32_t name,
      const uint8_t* data,
      uint16_t data_length_in_bytes);

  
  
  int32_t GetRemoteRTCPReceiverInfo(uint32_t& NTPHigh, uint32_t& NTPLow,
                                    uint32_t& receivedPacketCount,
                                    uint64_t& receivedOctetCount,
                                    uint32_t* jitterSamples,
                                    uint16_t* fractionLost,
                                    uint32_t* cumulativeLost,
                                    int32_t* rttMs);

  
  int32_t GetSendRtcpStatistics(uint16_t* fraction_lost,
                                uint32_t* cumulative_lost,
                                uint32_t* extended_max,
                                uint32_t* jitter_samples,
                                int32_t* rtt_ms);

  
  void RegisterSendChannelRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback);

  
  int32_t GetReceivedRtcpStatistics(uint16_t* fraction_lost,
                                    uint32_t* cumulative_lost,
                                    uint32_t* extended_max,
                                    uint32_t* jitter_samples,
                                    int32_t* rtt_ms);

  
  void RegisterReceiveChannelRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback);

  
  int32_t GetRtpStatistics(uint32_t* bytes_sent,
                           uint32_t* packets_sent,
                           uint32_t* bytes_received,
                           uint32_t* packets_received) const;

  
  void RegisterSendChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback);

  
  void RegisterReceiveChannelRtpStatisticsCallback(
      StreamDataCountersCallback* callback);


  int32_t GetRemoteRTCPSenderInfo(SenderInfo* sender_info) const;

  void GetBandwidthUsage(uint32_t* total_bitrate_sent,
                         uint32_t* video_bitrate_sent,
                         uint32_t* fec_bitrate_sent,
                         uint32_t* nackBitrateSent) const;
  bool GetSendSideDelay(int* avg_send_delay, int* max_send_delay) const;
  void GetEstimatedReceiveBandwidth(uint32_t* estimated_bandwidth) const;
  void GetReceiveBandwidthEstimatorStats(
      ReceiveBandwidthEstimatorStats* output) const;

  
  void RegisterSendBitrateObserver(BitrateStatisticsObserver* observer);

  int32_t StartRTPDump(const char file_nameUTF8[1024],
                       RTPDirections direction);
  int32_t StopRTPDump(RTPDirections direction);

  
  
  virtual void OnApplicationDataReceived(const int32_t id,
                                         const uint8_t sub_type,
                                         const uint32_t name,
                                         const uint16_t length,
                                         const uint8_t* data);
  
  virtual int32_t OnInitializeDecoder(
      const int32_t id,
      const int8_t payload_type,
      const char payload_name[RTP_PAYLOAD_NAME_SIZE],
      const int frequency,
      const uint8_t channels,
      const uint32_t rate);
  virtual void OnIncomingSSRCChanged(const int32_t id,
                                     const uint32_t ssrc);
  virtual void OnIncomingCSRCChanged(const int32_t id,
                                     const uint32_t CSRC,
                                     const bool added);
  virtual void ResetStatistics(uint32_t);

  int32_t SetLocalReceiver(const uint16_t rtp_port,
                           const uint16_t rtcp_port,
                           const char* ip_address);
  int32_t GetLocalReceiver(uint16_t* rtp_port,
                           uint16_t* rtcp_port,
                           char* ip_address) const;
  int32_t SetSendDestination(const char* ip_address,
                             const uint16_t rtp_port,
                             const uint16_t rtcp_port,
                             const uint16_t source_rtp_port,
                             const uint16_t source_rtcp_port);
  int32_t GetSendDestination(char* ip_address,
                             uint16_t* rtp_port,
                             uint16_t* rtcp_port,
                             uint16_t* source_rtp_port,
                             uint16_t* source_rtcp_port) const;
  int32_t GetSourceInfo(uint16_t* rtp_port,
                        uint16_t* rtcp_port,
                        char* ip_address,
                        uint32_t ip_address_length);

  int32_t SetRemoteSSRCType(const StreamType usage, const uint32_t SSRC);

  int32_t StartSend();
  int32_t StopSend();
  bool Sending();
  int32_t StartReceive();
  int32_t StopReceive();

  int32_t RegisterSendTransport(Transport* transport);
  int32_t DeregisterSendTransport();

  
  int32_t ReceivedRTPPacket(const void* rtp_packet,
                            const int32_t rtp_packet_length,
                            const PacketTime& packet_time);

  
  int32_t ReceivedRTCPPacket(const void* rtcp_packet,
                             const int32_t rtcp_packet_length);

  
  
  int32_t SetMTU(uint16_t mtu);

  
  
  uint16_t MaxDataPayloadLength() const;
  int32_t SetMaxPacketBurstSize(uint16_t max_number_of_packets);
  int32_t SetPacketBurstSpreadState(bool enable, const uint16_t frame_periodMS);

  int32_t EnableColorEnhancement(bool enable);

  
  RtpRtcp* rtp_rtcp();

  CallStatsObserver* GetStatsObserver();

  
  virtual int32_t FrameToRender(I420VideoFrame& video_frame);  

  
  virtual int32_t ReceivedDecodedReferenceFrame(
      const uint64_t picture_id);

  
  virtual void IncomingCodecChanged(const VideoCodec& codec);

  
  virtual int32_t OnReceiveStatisticsUpdate(const uint32_t bit_rate,
                                    const uint32_t frame_rate);

  
  virtual void OnDecoderTiming(int decode_ms,
                               int max_decode_ms,
                               int current_delay_ms,
                               int target_delay_ms,
                               int jitter_buffer_ms,
                               int min_playout_delay_ms,
                               int render_delay_ms);

  
  virtual int32_t RequestKeyFrame();

  
  virtual int32_t SliceLossIndicationRequest(
      const uint64_t picture_id);

  
  virtual int32_t ResendPackets(const uint16_t* sequence_numbers,
                                uint16_t length);

  int32_t SetVoiceChannel(int32_t ve_channel_id,
                          VoEVideoSync* ve_sync_interface);
  int32_t VoiceChannel();

  
  virtual int FrameCallbackChanged() {return -1;}

  int32_t RegisterEffectFilter(ViEEffectFilter* effect_filter);

  
  void RegisterPreRenderCallback(I420FrameCallback* pre_render_callback);
  void RegisterPreDecodeImageCallback(
      EncodedImageCallback* pre_decode_callback);

  void RegisterSendFrameCountObserver(FrameCountObserver* observer);

 protected:
  static bool ChannelDecodeThreadFunction(void* obj);
  bool ChannelDecodeProcess();

  void OnRttUpdate(uint32_t rtt);

 private:
  
  int32_t StartDecodeThread();
  int32_t StopDecodeThread();

  int32_t ProcessNACKRequest(const bool enable);
  int32_t ProcessFECRequest(const bool enable,
                            const unsigned char payload_typeRED,
                            const unsigned char payload_typeFEC);
  
  int GetRequiredNackListSize(int target_delay_ms);

  int32_t channel_id_;
  int32_t engine_id_;
  uint32_t number_of_cores_;
  uint8_t num_socket_threads_;

  
  scoped_ptr<CriticalSectionWrapper> callback_cs_;
  scoped_ptr<CriticalSectionWrapper> rtp_rtcp_cs_;

  RtpRtcp* default_rtp_rtcp_;

  
  scoped_ptr<RtpRtcp> rtp_rtcp_;
  std::list<RtpRtcp*> simulcast_rtp_rtcp_;
  std::list<RtpRtcp*> removed_rtp_rtcp_;
  VideoCodingModule& vcm_;
  ViEReceiver vie_receiver_;
  ViESender vie_sender_;
  ViESyncModule vie_sync_;

  
  scoped_ptr<ChannelStatsObserver> stats_observer_;

  
  ProcessThread& module_process_thread_;
  ViEDecoderObserver* codec_observer_;
  bool do_key_frame_callbackRequest_;
  ViERTPObserver* rtp_observer_;
  ViERTCPObserver* rtcp_observer_;
  RtcpIntraFrameObserver* intra_frame_observer_;
  RtcpRttStats* rtt_stats_;
  PacedSender* paced_sender_;

  scoped_ptr<RtcpBandwidthObserver> bandwidth_observer_;
  int send_timestamp_extension_id_;
  int absolute_send_time_extension_id_;

  Transport* external_transport_;

  bool decoder_reset_;
  
  VideoCodec receive_codec_;
  bool wait_for_key_frame_;
  ThreadWrapper* decode_thread_;

  ViEEffectFilter* effect_filter_;
  bool color_enhancement_;

  
  uint16_t mtu_;
  const bool sender_;

  int nack_history_size_sender_;
  int max_nack_reordering_threshold_;
  I420FrameCallback* pre_render_callback_;
  const Config& config_;
};

}  

#endif  
