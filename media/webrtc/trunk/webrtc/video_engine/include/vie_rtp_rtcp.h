




















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_RTP_RTCP_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_RTP_RTCP_H_

#include "webrtc/common_types.h"

namespace webrtc {

class VideoEngine;
struct ReceiveBandwidthEstimatorStats;


enum ViERTCPMode {
  kRtcpNone = 0,
  kRtcpCompound_RFC4585 = 1,
  kRtcpNonCompound_RFC5506 = 2
};


enum ViEKeyFrameRequestMethod {
  kViEKeyFrameRequestNone = 0,
  kViEKeyFrameRequestPliRtcp = 1,
  kViEKeyFrameRequestFirRtp = 2,
  kViEKeyFrameRequestFirRtcp = 3
};

enum StreamType {
  kViEStreamTypeNormal = 0,  
  kViEStreamTypeRtx = 1  
};

enum BandwidthEstimationMode {
  kViEMultiStreamEstimation,
  kViESingleStreamEstimation
};





class WEBRTC_DLLEXPORT ViERTPObserver {
 public:
  
  virtual void IncomingSSRCChanged(const int video_channel,
                                   const unsigned int SSRC) = 0;

  
  
  virtual void IncomingCSRCChanged(const int video_channel,
                                   const unsigned int CSRC,
                                   const bool added) = 0;
 protected:
  virtual ~ViERTPObserver() {}
};






class WEBRTC_DLLEXPORT ViERTCPObserver {
 public:
  
  
  virtual void OnApplicationDataReceived(
      const int video_channel,
      const unsigned char sub_type,
      const unsigned int name,
      const char* data,
      const unsigned short data_length_in_bytes) = 0;
 protected:
  virtual ~ViERTCPObserver() {}
};

struct SenderInfo;

class WEBRTC_DLLEXPORT ViERTP_RTCP {
 public:
  enum { KDefaultDeltaTransmitTimeSeconds = 15 };
  enum { KMaxRTCPCNameLength = 256 };

  
  
  
  static ViERTP_RTCP* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  
  virtual int SetLocalSSRC(const int video_channel,
                           const unsigned int SSRC,
                           const StreamType usage = kViEStreamTypeNormal,
                           const unsigned char simulcast_idx = 0) = 0;

  
  
  virtual int GetLocalSSRC(const int video_channel,
                           unsigned int& SSRC) const = 0;

  
  
  virtual int SetRemoteSSRCType(const int video_channel,
                                const StreamType usage,
                                const unsigned int SSRC) const = 0;

  
  
  virtual int GetRemoteSSRC(const int video_channel,
                            unsigned int& SSRC) const = 0;

  
  virtual int GetRemoteCSRCs(const int video_channel,
                             unsigned int CSRCs[kRtpCsrcSize]) const = 0;

  
  
  virtual int SetRtxSendPayloadType(const int video_channel,
                                    const uint8_t payload_type) = 0;

  virtual int SetRtxReceivePayloadType(const int video_channel,
                                       const uint8_t payload_type) = 0;

  
  
  virtual int SetStartSequenceNumber(const int video_channel,
                                     unsigned short sequence_number) = 0;

  
  
  virtual int SetRTCPStatus(const int video_channel,
                            const ViERTCPMode rtcp_mode) = 0;

  
  virtual int GetRTCPStatus(const int video_channel,
                            ViERTCPMode& rtcp_mode) const = 0;

  
  
  virtual int SetRTCPCName(const int video_channel,
                           const char rtcp_cname[KMaxRTCPCNameLength]) = 0;

  
  
  virtual int GetRTCPCName(const int video_channel,
                           char rtcp_cname[KMaxRTCPCNameLength]) const = 0;

  
  
  virtual int GetRemoteRTCPCName(
      const int video_channel,
      char rtcp_cname[KMaxRTCPCNameLength]) const = 0;

  virtual int GetRemoteRTCPReceiverInfo(const int video_channel,
                                        uint32_t& NTPHigh,
                                        uint32_t& NTPLow,
                                        uint32_t& receivedPacketCount,
                                        uint64_t& receivedOctetCount,
                                        uint32_t* jitter,
                                        uint16_t* fractionLost,
                                        uint32_t* cumulativeLost,
                                        int32_t* rttMs) const = 0;

  
  virtual int SendApplicationDefinedRTCPPacket(
      const int video_channel,
      const unsigned char sub_type,
      unsigned int name,
      const char* data,
      unsigned short data_length_in_bytes) = 0;

  
  
  
  
  virtual int SetNACKStatus(const int video_channel, const bool enable) = 0;

  
  
  
  
  virtual int SetFECStatus(const int video_channel,
                           const bool enable,
                           const unsigned char payload_typeRED,
                           const unsigned char payload_typeFEC) = 0;

  
  
  
  
  
  
  
  
  virtual int SetHybridNACKFECStatus(const int video_channel,
                                     const bool enable,
                                     const unsigned char payload_typeRED,
                                     const unsigned char payload_typeFEC) = 0;

  
  
  
  virtual int SetSenderBufferingMode(int video_channel,
                                     int target_delay_ms) = 0;
  
  
  virtual int SetReceiverBufferingMode(int video_channel,
                                       int target_delay_ms) = 0;

  
  virtual int SetKeyFrameRequestMethod(
    const int video_channel, const ViEKeyFrameRequestMethod method) = 0;

  
  
  virtual int SetTMMBRStatus(const int video_channel, const bool enable) = 0;

  
  
  
  virtual int SetRembStatus(int video_channel,
                            bool sender,
                            bool receiver) = 0;

  
  
  virtual int SetSendTimestampOffsetStatus(int video_channel,
                                           bool enable,
                                           int id) = 0;

  virtual int SetReceiveTimestampOffsetStatus(int video_channel,
                                              bool enable,
                                              int id) = 0;

  
  
  virtual int SetSendAbsoluteSendTimeStatus(int video_channel,
                                            bool enable,
                                            int id) = 0;

  
  
  virtual int SetReceiveAbsoluteSendTimeStatus(int video_channel,
                                               bool enable,
                                               int id) = 0;

  
  
  
  virtual int SetRtcpXrRrtrStatus(int video_channel, bool enable) { return -1; }

  
  
  
  virtual int SetTransmissionSmoothingStatus(int video_channel,
                                             bool enable) = 0;

  
  
  virtual int GetReceiveChannelRtcpStatistics(const int video_channel,
                                              RtcpStatistics& basic_stats,
                                              int& rtt_ms) const = 0;

  
  
  virtual int GetSendChannelRtcpStatistics(const int video_channel,
                                           RtcpStatistics& basic_stats,
                                           int& rtt_ms) const = 0;

  
  
  virtual int GetReceivedRTCPStatistics(const int video_channel,
                                unsigned short& fraction_lost,
                                unsigned int& cumulative_lost,
                                unsigned int& extended_max,
                                unsigned int& jitter,
                                int& rtt_ms) const {
    RtcpStatistics stats;
    int ret_code = GetReceiveChannelRtcpStatistics(video_channel,
                                             stats,
                                             rtt_ms);
    fraction_lost = stats.fraction_lost;
    cumulative_lost = stats.cumulative_lost;
    extended_max = stats.extended_max_sequence_number;
    jitter = stats.jitter;
    return ret_code;
  }
  virtual int GetSentRTCPStatistics(const int video_channel,
                            unsigned short& fraction_lost,
                            unsigned int& cumulative_lost,
                            unsigned int& extended_max,
                            unsigned int& jitter,
                            int& rtt_ms) const {
    RtcpStatistics stats;
    int ret_code = GetSendChannelRtcpStatistics(video_channel,
                                                stats,
                                                rtt_ms);
    fraction_lost = stats.fraction_lost;
    cumulative_lost = stats.cumulative_lost;
    extended_max = stats.extended_max_sequence_number;
    jitter = stats.jitter;
    return ret_code;
  }


  virtual int RegisterSendChannelRtcpStatisticsCallback(
      int video_channel, RtcpStatisticsCallback* callback) = 0;

  virtual int DeregisterSendChannelRtcpStatisticsCallback(
      int video_channel, RtcpStatisticsCallback* callback) = 0;

  virtual int RegisterReceiveChannelRtcpStatisticsCallback(
      int video_channel, RtcpStatisticsCallback* callback) = 0;

  virtual int DeregisterReceiveChannelRtcpStatisticsCallback(
      int video_channel, RtcpStatisticsCallback* callback) = 0;

  
  virtual int GetRtpStatistics(const int video_channel,
                               StreamDataCounters& sent,
                               StreamDataCounters& received) const = 0;

  
  
  virtual int GetRTPStatistics(const int video_channel,
                       unsigned int& bytes_sent,
                       unsigned int& packets_sent,
                       unsigned int& bytes_received,
                       unsigned int& packets_received) const {
    StreamDataCounters sent;
    StreamDataCounters received;
    int ret_code = GetRtpStatistics(video_channel, sent, received);
    bytes_sent = sent.bytes;
    packets_sent = sent.packets;
    bytes_received = received.bytes;
    packets_received = received.packets;
    return ret_code;
  }

  virtual int RegisterSendChannelRtpStatisticsCallback(
      int video_channel, StreamDataCountersCallback* callback) = 0;

  virtual int DeregisterSendChannelRtpStatisticsCallback(
      int video_channel, StreamDataCountersCallback* callback) = 0;

  virtual int RegisterReceiveChannelRtpStatisticsCallback(
      int video_channel, StreamDataCountersCallback* callback) = 0;

  virtual int DeregisterReceiveChannelRtpStatisticsCallback(
      int video_channel, StreamDataCountersCallback* callback) = 0;

  
  virtual int GetRemoteRTCPSenderInfo(const int video_channel,
                                      SenderInfo* sender_info) const = 0;

  
  
  virtual int GetBandwidthUsage(const int video_channel,
                                unsigned int& total_bitrate_sent,
                                unsigned int& video_bitrate_sent,
                                unsigned int& fec_bitrate_sent,
                                unsigned int& nackBitrateSent) const = 0;

  
  virtual int RegisterSendBitrateObserver(
      int video_channel,
      BitrateStatisticsObserver* observer) = 0;

  virtual int DeregisterSendBitrateObserver(
      int video_channel,
      BitrateStatisticsObserver* observer) = 0;

  
  
  virtual int GetEstimatedSendBandwidth(
      const int video_channel,
      unsigned int* estimated_bandwidth) const = 0;

  
  
  
  virtual int GetEstimatedReceiveBandwidth(
      const int video_channel,
      unsigned int* estimated_bandwidth) const = 0;

  
  
  
  virtual int GetReceiveBandwidthEstimatorStats(
      const int video_channel,
      ReceiveBandwidthEstimatorStats* output) const { return -1; }

  
  
  
  
  virtual int StartRTPDump(const int video_channel,
                           const char file_nameUTF8[1024],
                           RTPDirections direction) = 0;

  
  
  virtual int StopRTPDump(const int video_channel,
                          RTPDirections direction) = 0;

  
  virtual int RegisterRTPObserver(const int video_channel,
                                  ViERTPObserver& observer) = 0;

  
  virtual int DeregisterRTPObserver(const int video_channel) = 0;

  
  virtual int RegisterRTCPObserver(const int video_channel,
                                   ViERTCPObserver& observer) = 0;

  
  virtual int DeregisterRTCPObserver(const int video_channel) = 0;

  
  virtual int RegisterSendFrameCountObserver(
      int video_channel, FrameCountObserver* observer) = 0;

  
  virtual int DeregisterSendFrameCountObserver(
      int video_channel, FrameCountObserver* observer) = 0;

 protected:
  virtual ~ViERTP_RTCP() {}
};

}  

#endif
