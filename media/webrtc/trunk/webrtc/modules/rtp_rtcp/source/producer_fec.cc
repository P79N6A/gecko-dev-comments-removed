









#include "modules/rtp_rtcp/source/producer_fec.h"

#include "modules/rtp_rtcp/source/forward_error_correction.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"

namespace webrtc {

enum { kREDForFECHeaderLength = 1 };



enum { kMaxExcessOverhead = 50 };  



enum { kMinimumMediaPackets = 4 };



enum { kHighProtectionThreshold = 80 };  



struct RtpPacket {
  WebRtc_UWord16 rtpHeaderLength;
  ForwardErrorCorrection::Packet* pkt;
};

RedPacket::RedPacket(int length)
    : data_(new uint8_t[length]),
      length_(length),
      header_length_(0) {
}

RedPacket::~RedPacket() {
  delete [] data_;
}

void RedPacket::CreateHeader(const uint8_t* rtp_header, int header_length,
                             int red_pl_type, int pl_type) {
  assert(header_length + kREDForFECHeaderLength <= length_);
  memcpy(data_, rtp_header, header_length);
  
  data_[1] &= 0x80;
  data_[1] += red_pl_type;
  
  
  data_[header_length] = pl_type;
  header_length_ = header_length + kREDForFECHeaderLength;
}

void RedPacket::SetSeqNum(int seq_num) {
  assert(seq_num >= 0 && seq_num < (1<<16));
  ModuleRTPUtility::AssignUWord16ToBuffer(&data_[2], seq_num);
}

void RedPacket::AssignPayload(const uint8_t* payload, int length) {
  assert(header_length_ + length <= length_);
  memcpy(data_ + header_length_, payload, length);
}

void RedPacket::ClearMarkerBit() {
  data_[1] &= 0x7F;
}

uint8_t* RedPacket::data() const {
  return data_;
}

int RedPacket::length() const {
  return length_;
}

ProducerFec::ProducerFec(ForwardErrorCorrection* fec)
    : fec_(fec),
      media_packets_fec_(),
      fec_packets_(),
      num_frames_(0),
      incomplete_frame_(false),
      num_first_partition_(0),
      minimum_media_packets_fec_(1),
      params_(),
      new_params_() {
  memset(&params_, 0, sizeof(params_));
  memset(&new_params_, 0, sizeof(new_params_));
}

ProducerFec::~ProducerFec() {
  DeletePackets();
}

void ProducerFec::SetFecParameters(const FecProtectionParams* params,
                                   int num_first_partition) {
  
  assert(params->fec_rate >= 0 && params->fec_rate < 256);
  if (num_first_partition >
      static_cast<int>(ForwardErrorCorrection::kMaxMediaPackets)) {
      num_first_partition =
          ForwardErrorCorrection::kMaxMediaPackets;
  }
  
  
  new_params_ = *params;
  num_first_partition_ = num_first_partition;
  if (params->fec_rate > kHighProtectionThreshold) {
    minimum_media_packets_fec_ = kMinimumMediaPackets;
  } else {
    minimum_media_packets_fec_ = 1;
  }
}

RedPacket* ProducerFec::BuildRedPacket(const uint8_t* data_buffer,
                                       int payload_length,
                                       int rtp_header_length,
                                       int red_pl_type) {
  RedPacket* red_packet = new RedPacket(payload_length +
                                        kREDForFECHeaderLength +
                                        rtp_header_length);
  int pl_type = data_buffer[1] & 0x7f;
  red_packet->CreateHeader(data_buffer, rtp_header_length,
                           red_pl_type, pl_type);
  red_packet->AssignPayload(data_buffer + rtp_header_length, payload_length);
  return red_packet;
}

int ProducerFec::AddRtpPacketAndGenerateFec(const uint8_t* data_buffer,
                                            int payload_length,
                                            int rtp_header_length) {
  assert(fec_packets_.empty());
  if (media_packets_fec_.empty()) {
    params_ = new_params_;
  }
  incomplete_frame_ = true;
  const bool marker_bit = (data_buffer[1] & kRtpMarkerBitMask) ? true : false;
  if (media_packets_fec_.size() < ForwardErrorCorrection::kMaxMediaPackets) {
    
    ForwardErrorCorrection::Packet* packet = new ForwardErrorCorrection::Packet;
    packet->length = payload_length + rtp_header_length;
    memcpy(packet->data, data_buffer, packet->length);
    media_packets_fec_.push_back(packet);
  }
  if (marker_bit) {
    ++num_frames_;
    incomplete_frame_ = false;
  }
  
  
  
  
  if (!incomplete_frame_ &&
      (num_frames_ == params_.max_fec_frames ||
          (ExcessOverheadBelowMax() && MinimumMediaPacketsReached()))) {
    assert(num_first_partition_ <=
           static_cast<int>(ForwardErrorCorrection::kMaxMediaPackets));
    int ret = fec_->GenerateFEC(media_packets_fec_,
                                params_.fec_rate,
                                num_first_partition_,
                                params_.use_uep_protection,
                                params_.fec_mask_type,
                                &fec_packets_);
    if (fec_packets_.empty()) {
      num_frames_ = 0;
      DeletePackets();
    }
    return ret;
  }
  return 0;
}






bool ProducerFec::ExcessOverheadBelowMax() {
  return ((Overhead() - params_.fec_rate) < kMaxExcessOverhead);
}





bool ProducerFec::MinimumMediaPacketsReached() {
  float avg_num_packets_frame = static_cast<float>(media_packets_fec_.size()) /
                                num_frames_;
  if (avg_num_packets_frame < 2.0f) {
  return (static_cast<int>(media_packets_fec_.size()) >=
      minimum_media_packets_fec_);
  } else {
    
    return (static_cast<int>(media_packets_fec_.size()) >=
        minimum_media_packets_fec_ + 1);
  }
}

bool ProducerFec::FecAvailable() const {
  return (fec_packets_.size() > 0);
}

RedPacket* ProducerFec::GetFecPacket(int red_pl_type,
                                     int fec_pl_type,
                                     uint16_t seq_num,
                                     int rtp_header_length) {
  if (fec_packets_.empty())
    return NULL;
  
  
  
  ForwardErrorCorrection::Packet* packet_to_send = fec_packets_.front();
  ForwardErrorCorrection::Packet* last_media_packet = media_packets_fec_.back();
  RedPacket* return_packet = new RedPacket(packet_to_send->length +
                                           kREDForFECHeaderLength +
                                           rtp_header_length);
  return_packet->CreateHeader(last_media_packet->data,
                              rtp_header_length,
                              red_pl_type,
                              fec_pl_type);
  return_packet->SetSeqNum(seq_num);
  return_packet->ClearMarkerBit();
  return_packet->AssignPayload(packet_to_send->data, packet_to_send->length);
  fec_packets_.pop_front();
  if (fec_packets_.empty()) {
    
    DeletePackets();
    num_frames_ = 0;
  }
  return return_packet;
}

int ProducerFec::Overhead() const {
  
  
  
  
  assert(!media_packets_fec_.empty());
  int num_fec_packets = fec_->GetNumberOfFecPackets(media_packets_fec_.size(),
                                                    params_.fec_rate);
  
  return (num_fec_packets << 8) / media_packets_fec_.size();
}

void ProducerFec::DeletePackets() {
  while (!media_packets_fec_.empty()) {
    delete media_packets_fec_.front();
    media_packets_fec_.pop_front();
  }
  assert(media_packets_fec_.empty());
}

}  
