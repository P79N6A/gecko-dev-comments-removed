









#include "webrtc/modules/rtp_rtcp/source/rtp_sender_video.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/producer_fec.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_video_generic.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_h264.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc {
enum { REDForFECHeaderLength = 1 };

struct RtpPacket {
  uint16_t rtpHeaderLength;
  ForwardErrorCorrection::Packet* pkt;
};

RTPSenderVideo::RTPSenderVideo(Clock* clock, RTPSenderInterface* rtpSender)
    : _rtpSender(*rtpSender),
      _sendVideoCritsect(CriticalSectionWrapper::CreateCriticalSection()),
      _videoType(kRtpVideoGeneric),
      _videoCodecInformation(NULL),
      _maxBitrate(0),
      _retransmissionSettings(kRetransmitBaseLayer),

      
      _fec(),
      _fecEnabled(false),
      _payloadTypeRED(-1),
      _payloadTypeFEC(-1),
      _numberFirstPartition(0),
      delta_fec_params_(),
      key_fec_params_(),
      producer_fec_(&_fec),
      _fecOverheadRate(clock, NULL),
      _videoBitrate(clock, NULL) {
  memset(&delta_fec_params_, 0, sizeof(delta_fec_params_));
  memset(&key_fec_params_, 0, sizeof(key_fec_params_));
  delta_fec_params_.max_fec_frames = key_fec_params_.max_fec_frames = 1;
  delta_fec_params_.fec_mask_type = key_fec_params_.fec_mask_type =
      kFecMaskRandom;
}

RTPSenderVideo::~RTPSenderVideo() {
  if (_videoCodecInformation) {
    delete _videoCodecInformation;
  }
  delete _sendVideoCritsect;
}

void RTPSenderVideo::SetVideoCodecType(RtpVideoCodecTypes videoType) {
  CriticalSectionScoped cs(_sendVideoCritsect);
  _videoType = videoType;
}

RtpVideoCodecTypes RTPSenderVideo::VideoCodecType() const {
  return _videoType;
}

int32_t RTPSenderVideo::RegisterVideoPayload(
    const char payloadName[RTP_PAYLOAD_NAME_SIZE],
    const int8_t payloadType,
    const uint32_t maxBitRate,
    RtpUtility::Payload*& payload) {
  CriticalSectionScoped cs(_sendVideoCritsect);

  RtpVideoCodecTypes videoType = kRtpVideoGeneric;
  if (RtpUtility::StringCompare(payloadName, "VP8", 3)) {
    videoType = kRtpVideoVp8;
  } else if (RtpUtility::StringCompare(payloadName, "VP9", 3)) {
    videoType = kRtpVideoVp9;
  } else if (RtpUtility::StringCompare(payloadName, "H264", 4)) {
    videoType = kRtpVideoH264;
  } else if (RtpUtility::StringCompare(payloadName, "I420", 4)) {
    videoType = kRtpVideoGeneric;
  } else {
    videoType = kRtpVideoGeneric;
  }
  payload = new RtpUtility::Payload;
  payload->name[RTP_PAYLOAD_NAME_SIZE - 1] = 0;
  strncpy(payload->name, payloadName, RTP_PAYLOAD_NAME_SIZE - 1);
  payload->typeSpecific.Video.videoCodecType = videoType;
  payload->typeSpecific.Video.maxRate = maxBitRate;
  payload->audio = false;
  return 0;
}

int32_t RTPSenderVideo::SendVideoPacket(uint8_t* data_buffer,
                                        const uint16_t payload_length,
                                        const uint16_t rtp_header_length,
                                        const uint32_t capture_timestamp,
                                        int64_t capture_time_ms,
                                        StorageType storage,
                                        bool protect) {
  if (_fecEnabled) {
    int ret = 0;
    int fec_overhead_sent = 0;
    int video_sent = 0;

    RedPacket* red_packet = producer_fec_.BuildRedPacket(
        data_buffer, payload_length, rtp_header_length, _payloadTypeRED);
    TRACE_EVENT_INSTANT2("webrtc_rtp",
                         "Video::PacketRed",
                         "timestamp",
                         capture_timestamp,
                         "seqnum",
                         _rtpSender.SequenceNumber());
    
    int packet_success =
        _rtpSender.SendToNetwork(red_packet->data(),
                                 red_packet->length() - rtp_header_length,
                                 rtp_header_length,
                                 capture_time_ms,
                                 storage,
                                 PacedSender::kNormalPriority);

    ret |= packet_success;

    if (packet_success == 0) {
      video_sent += red_packet->length();
    }
    delete red_packet;
    red_packet = NULL;

    if (protect) {
      ret = producer_fec_.AddRtpPacketAndGenerateFec(
          data_buffer, payload_length, rtp_header_length);
      if (ret != 0)
        return ret;
    }

    while (producer_fec_.FecAvailable()) {
      red_packet =
          producer_fec_.GetFecPacket(_payloadTypeRED,
                                     _payloadTypeFEC,
                                     _rtpSender.IncrementSequenceNumber(),
                                     rtp_header_length);
      StorageType storage = kDontRetransmit;
      if (_retransmissionSettings & kRetransmitFECPackets) {
        storage = kAllowRetransmission;
      }
      TRACE_EVENT_INSTANT2("webrtc_rtp",
                           "Video::PacketFec",
                           "timestamp",
                           capture_timestamp,
                           "seqnum",
                           _rtpSender.SequenceNumber());
      
      int packet_success =
          _rtpSender.SendToNetwork(red_packet->data(),
                                   red_packet->length() - rtp_header_length,
                                   rtp_header_length,
                                   capture_time_ms,
                                   storage,
                                   PacedSender::kNormalPriority);

      ret |= packet_success;

      if (packet_success == 0) {
        fec_overhead_sent += red_packet->length();
      }
      delete red_packet;
      red_packet = NULL;
    }
    _videoBitrate.Update(video_sent);
    _fecOverheadRate.Update(fec_overhead_sent);
    return ret;
  }
  TRACE_EVENT_INSTANT2("webrtc_rtp",
                       "Video::PacketNormal",
                       "timestamp",
                       capture_timestamp,
                       "seqnum",
                       _rtpSender.SequenceNumber());
  int ret = _rtpSender.SendToNetwork(data_buffer,
                                     payload_length,
                                     rtp_header_length,
                                     capture_time_ms,
                                     storage,
                                     PacedSender::kNormalPriority);
  if (ret == 0) {
    _videoBitrate.Update(payload_length + rtp_header_length);
  }
  return ret;
}

int32_t RTPSenderVideo::SendRTPIntraRequest() {
  
  

  uint16_t length = 8;
  uint8_t data[8];
  data[0] = 0x80;
  data[1] = 192;
  data[2] = 0;
  data[3] = 1;  

  RtpUtility::AssignUWord32ToBuffer(data + 4, _rtpSender.SSRC());

  TRACE_EVENT_INSTANT1("webrtc_rtp",
                       "Video::IntraRequest",
                       "seqnum",
                       _rtpSender.SequenceNumber());
  return _rtpSender.SendToNetwork(
      data, 0, length, -1, kDontStore, PacedSender::kNormalPriority);
}

int32_t RTPSenderVideo::SetGenericFECStatus(const bool enable,
                                            const uint8_t payloadTypeRED,
                                            const uint8_t payloadTypeFEC) {
  _fecEnabled = enable;
  _payloadTypeRED = payloadTypeRED;
  _payloadTypeFEC = payloadTypeFEC;
  memset(&delta_fec_params_, 0, sizeof(delta_fec_params_));
  memset(&key_fec_params_, 0, sizeof(key_fec_params_));
  delta_fec_params_.max_fec_frames = key_fec_params_.max_fec_frames = 1;
  delta_fec_params_.fec_mask_type = key_fec_params_.fec_mask_type =
      kFecMaskRandom;
  return 0;
}

int32_t RTPSenderVideo::GenericFECStatus(bool& enable,
                                         uint8_t& payloadTypeRED,
                                         uint8_t& payloadTypeFEC) const {
  enable = _fecEnabled;
  payloadTypeRED = _payloadTypeRED;
  payloadTypeFEC = _payloadTypeFEC;
  return 0;
}

uint16_t RTPSenderVideo::FECPacketOverhead() const {
  if (_fecEnabled) {
    
    
    
    
    
    return ForwardErrorCorrection::PacketOverhead() + REDForFECHeaderLength +
           (_rtpSender.RTPHeaderLength() - kRtpHeaderSize);
  }
  return 0;
}

int32_t RTPSenderVideo::SetFecParameters(
    const FecProtectionParams* delta_params,
    const FecProtectionParams* key_params) {
  assert(delta_params);
  assert(key_params);
  delta_fec_params_ = *delta_params;
  key_fec_params_ = *key_params;
  return 0;
}

int32_t RTPSenderVideo::SendVideo(const RtpVideoCodecTypes videoType,
                                  const FrameType frameType,
                                  const int8_t payloadType,
                                  const uint32_t captureTimeStamp,
                                  int64_t capture_time_ms,
                                  const uint8_t* payloadData,
                                  const uint32_t payloadSize,
                                  const RTPFragmentationHeader* fragmentation,
                                  VideoCodecInformation* codecInfo,
                                  const RTPVideoTypeHeader* rtpTypeHdr) {
  if (payloadSize == 0) {
    return -1;
  }

  if (frameType == kVideoFrameKey) {
    producer_fec_.SetFecParameters(&key_fec_params_, _numberFirstPartition);
  } else {
    producer_fec_.SetFecParameters(&delta_fec_params_, _numberFirstPartition);
  }

  
  
  _numberFirstPartition = 0;

  return Send(videoType,
              frameType,
              payloadType,
              captureTimeStamp,
              capture_time_ms,
              payloadData,
              payloadSize,
              fragmentation,
              rtpTypeHdr)
             ? 0
             : -1;
}

VideoCodecInformation* RTPSenderVideo::CodecInformationVideo() {
  return _videoCodecInformation;
}

void RTPSenderVideo::SetMaxConfiguredBitrateVideo(const uint32_t maxBitrate) {
  _maxBitrate = maxBitrate;
}

uint32_t RTPSenderVideo::MaxConfiguredBitrateVideo() const {
  return _maxBitrate;
}

bool RTPSenderVideo::Send(const RtpVideoCodecTypes videoType,
                          const FrameType frameType,
                          const int8_t payloadType,
                          const uint32_t captureTimeStamp,
                          int64_t capture_time_ms,
                          const uint8_t* payloadData,
                          const uint32_t payloadSize,
                          const RTPFragmentationHeader* fragmentation,
                          const RTPVideoTypeHeader* rtpTypeHdr) {
  uint16_t rtp_header_length = _rtpSender.RTPHeaderLength();
  int32_t payload_bytes_to_send = payloadSize;
  const uint8_t* data = payloadData;
  size_t max_payload_length = _rtpSender.MaxDataPayloadLength();

  scoped_ptr<RtpPacketizer> packetizer(RtpPacketizer::Create(
      videoType, max_payload_length, rtpTypeHdr, frameType));

  
  
  
  const RTPFragmentationHeader* frag =
      (videoType == kRtpVideoVp8 || videoType == kRtpVideoVp9) ? NULL : fragmentation;

  packetizer->SetPayloadData(data, payload_bytes_to_send, frag);

  bool last = false;
  while (!last) {
    uint8_t dataBuffer[IP_PACKET_SIZE] = {0};
    size_t payload_bytes_in_packet = 0;
    if (!packetizer->NextPacket(
            &dataBuffer[rtp_header_length], &payload_bytes_in_packet, &last)) {
      return false;
    }

    
    
    _rtpSender.BuildRTPheader(
        dataBuffer, payloadType, last, captureTimeStamp, capture_time_ms);
    if (SendVideoPacket(dataBuffer,
                        payload_bytes_in_packet,
                        rtp_header_length,
                        captureTimeStamp,
                        capture_time_ms,
                        packetizer->GetStorageType(_retransmissionSettings),
                        packetizer->GetProtectionType() == kProtectedPacket)) {
      LOG(LS_WARNING) << packetizer->ToString()
                      << " failed to send packet number "
                      << _rtpSender.SequenceNumber();
    }
  }

  TRACE_EVENT_ASYNC_END1(
      "webrtc", "Video", capture_time_ms, "timestamp", _rtpSender.Timestamp());
  return true;
}

void RTPSenderVideo::ProcessBitrate() {
  _videoBitrate.Process();
  _fecOverheadRate.Process();
}

uint32_t RTPSenderVideo::VideoBitrateSent() const {
  return _videoBitrate.BitrateLast();
}

uint32_t RTPSenderVideo::FecOverheadRate() const {
  return _fecOverheadRate.BitrateLast();
}

int RTPSenderVideo::SelectiveRetransmissions() const {
  return _retransmissionSettings;
}

int RTPSenderVideo::SetSelectiveRetransmissions(uint8_t settings) {
  _retransmissionSettings = settings;
  return 0;
}

}  
