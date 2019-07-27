









#include "webrtc/modules/rtp_rtcp/source/rtcp_packet.h"

#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"
#include "webrtc/system_wrappers/interface/logging.h"

using webrtc::RTCPUtility::kBtDlrr;
using webrtc::RTCPUtility::kBtReceiverReferenceTime;
using webrtc::RTCPUtility::kBtVoipMetric;

using webrtc::RTCPUtility::PT_APP;
using webrtc::RTCPUtility::PT_BYE;
using webrtc::RTCPUtility::PT_IJ;
using webrtc::RTCPUtility::PT_PSFB;
using webrtc::RTCPUtility::PT_RR;
using webrtc::RTCPUtility::PT_RTPFB;
using webrtc::RTCPUtility::PT_SDES;
using webrtc::RTCPUtility::PT_SR;
using webrtc::RTCPUtility::PT_XR;

using webrtc::RTCPUtility::RTCPPacketAPP;
using webrtc::RTCPUtility::RTCPPacketBYE;
using webrtc::RTCPUtility::RTCPPacketPSFBAPP;
using webrtc::RTCPUtility::RTCPPacketPSFBFIR;
using webrtc::RTCPUtility::RTCPPacketPSFBFIRItem;
using webrtc::RTCPUtility::RTCPPacketPSFBPLI;
using webrtc::RTCPUtility::RTCPPacketPSFBREMBItem;
using webrtc::RTCPUtility::RTCPPacketPSFBRPSI;
using webrtc::RTCPUtility::RTCPPacketPSFBSLI;
using webrtc::RTCPUtility::RTCPPacketPSFBSLIItem;
using webrtc::RTCPUtility::RTCPPacketReportBlockItem;
using webrtc::RTCPUtility::RTCPPacketRR;
using webrtc::RTCPUtility::RTCPPacketRTPFBNACK;
using webrtc::RTCPUtility::RTCPPacketRTPFBNACKItem;
using webrtc::RTCPUtility::RTCPPacketRTPFBTMMBN;
using webrtc::RTCPUtility::RTCPPacketRTPFBTMMBNItem;
using webrtc::RTCPUtility::RTCPPacketRTPFBTMMBR;
using webrtc::RTCPUtility::RTCPPacketRTPFBTMMBRItem;
using webrtc::RTCPUtility::RTCPPacketSR;
using webrtc::RTCPUtility::RTCPPacketXRDLRRReportBlockItem;
using webrtc::RTCPUtility::RTCPPacketXRReceiverReferenceTimeItem;
using webrtc::RTCPUtility::RTCPPacketXR;
using webrtc::RTCPUtility::RTCPPacketXRVOIPMetricItem;

namespace webrtc {
namespace rtcp {
namespace {

const uint32_t kUnusedMediaSourceSsrc0 = 0;

void AssignUWord8(uint8_t* buffer, size_t* offset, uint8_t value) {
  buffer[(*offset)++] = value;
}
void AssignUWord16(uint8_t* buffer, size_t* offset, uint16_t value) {
  RtpUtility::AssignUWord16ToBuffer(buffer + *offset, value);
  *offset += 2;
}
void AssignUWord24(uint8_t* buffer, size_t* offset, uint32_t value) {
  RtpUtility::AssignUWord24ToBuffer(buffer + *offset, value);
  *offset += 3;
}
void AssignUWord32(uint8_t* buffer, size_t* offset, uint32_t value) {
  RtpUtility::AssignUWord32ToBuffer(buffer + *offset, value);
  *offset += 4;
}

void ComputeMantissaAnd6bitBase2Exponent(uint32_t input_base10,
                                         uint8_t bits_mantissa,
                                         uint32_t* mantissa,
                                         uint8_t* exp) {
  
  assert(bits_mantissa <= 32);
  uint32_t mantissa_max = (1 << bits_mantissa) - 1;
  uint8_t exponent = 0;
  for (uint32_t i = 0; i < 64; ++i) {
    if (input_base10 <= (mantissa_max << i)) {
      exponent = i;
      break;
    }
  }
  *exp = exponent;
  *mantissa = (input_base10 >> exponent);
}

size_t BlockToHeaderLength(size_t length_in_bytes) {
  
  assert(length_in_bytes > 0);
  assert(length_in_bytes % 4 == 0);
  return (length_in_bytes / 4) - 1;
}










void CreateHeader(uint8_t count_or_format,  
                  uint8_t packet_type,
                  size_t length,
                  uint8_t* buffer,
                  size_t* pos) {
  assert(length <= 0xffff);
  const uint8_t kVersion = 2;
  AssignUWord8(buffer, pos, (kVersion << 6) + count_or_format);
  AssignUWord8(buffer, pos, packet_type);
  AssignUWord16(buffer, pos, length);
}




















void CreateSenderReport(const RTCPPacketSR& sr,
                        size_t length,
                        uint8_t* buffer,
                        size_t* pos) {
  CreateHeader(sr.NumberOfReportBlocks, PT_SR, length, buffer, pos);
  AssignUWord32(buffer, pos, sr.SenderSSRC);
  AssignUWord32(buffer, pos, sr.NTPMostSignificant);
  AssignUWord32(buffer, pos, sr.NTPLeastSignificant);
  AssignUWord32(buffer, pos, sr.RTPTimestamp);
  AssignUWord32(buffer, pos, sr.SenderPacketCount);
  AssignUWord32(buffer, pos, sr.SenderOctetCount);
}










void CreateReceiverReport(const RTCPPacketRR& rr,
                          size_t length,
                          uint8_t* buffer,
                          size_t* pos) {
  CreateHeader(rr.NumberOfReportBlocks, PT_RR, length, buffer, pos);
  AssignUWord32(buffer, pos, rr.SenderSSRC);
}


















void CreateReportBlocks(const std::vector<RTCPPacketReportBlockItem>& blocks,
                        uint8_t* buffer,
                        size_t* pos) {
  for (std::vector<RTCPPacketReportBlockItem>::const_iterator
       it = blocks.begin(); it != blocks.end(); ++it) {
    AssignUWord32(buffer, pos, (*it).SSRC);
    AssignUWord8(buffer, pos, (*it).FractionLost);
    AssignUWord24(buffer, pos, (*it).CumulativeNumOfPacketsLost);
    AssignUWord32(buffer, pos, (*it).ExtendedHighestSequenceNumber);
    AssignUWord32(buffer, pos, (*it).Jitter);
    AssignUWord32(buffer, pos, (*it).LastSR);
    AssignUWord32(buffer, pos, (*it).DelayLastSR);
  }
}
















void CreateIj(const std::vector<uint32_t>& ij_items,
              uint8_t* buffer,
              size_t* pos) {
  size_t length = ij_items.size();
  CreateHeader(length, PT_IJ, length, buffer, pos);
  for (std::vector<uint32_t>::const_iterator it = ij_items.begin();
       it != ij_items.end(); ++it) {
    AssignUWord32(buffer, pos, *it);
  }
}



























void CreateSdes(const std::vector<Sdes::Chunk>& chunks,
                size_t length,
                uint8_t* buffer,
                size_t* pos) {
  CreateHeader(chunks.size(), PT_SDES, length, buffer, pos);
  const uint8_t kSdesItemType = 1;
  for (std::vector<Sdes::Chunk>::const_iterator it = chunks.begin();
       it != chunks.end(); ++it) {
    AssignUWord32(buffer, pos, (*it).ssrc);
    AssignUWord8(buffer, pos, kSdesItemType);
    AssignUWord8(buffer, pos, (*it).name.length());
    memcpy(buffer + *pos, (*it).name.data(), (*it).name.length());
    *pos += (*it).name.length();
    memset(buffer + *pos, 0, (*it).null_octets);
    *pos += (*it).null_octets;
  }
}














void CreateBye(const RTCPPacketBYE& bye,
               const std::vector<uint32_t>& csrcs,
               size_t length,
               uint8_t* buffer,
               size_t* pos) {
  CreateHeader(length, PT_BYE, length, buffer, pos);
  AssignUWord32(buffer, pos, bye.SenderSSRC);
  for (std::vector<uint32_t>::const_iterator it = csrcs.begin();
       it != csrcs.end(); ++it) {
    AssignUWord32(buffer, pos, *it);
  }
}















void CreateApp(const RTCPPacketAPP& app,
               uint32_t ssrc,
               size_t length,
               uint8_t* buffer,
               size_t* pos) {
  CreateHeader(app.SubType, PT_APP, length, buffer, pos);
  AssignUWord32(buffer, pos, ssrc);
  AssignUWord32(buffer, pos, app.Name);
  memcpy(buffer + *pos, app.Data, app.Size);
  *pos += app.Size;
}






















void CreatePli(const RTCPPacketPSFBPLI& pli,
               size_t length,
               uint8_t* buffer,
               size_t* pos) {
  const uint8_t kFmt = 1;
  CreateHeader(kFmt, PT_PSFB, length, buffer, pos);
  AssignUWord32(buffer, pos, pli.SenderSSRC);
  AssignUWord32(buffer, pos, pli.MediaSSRC);
}











void CreateSli(const RTCPPacketPSFBSLI& sli,
               const RTCPPacketPSFBSLIItem& sli_item,
               size_t length,
               uint8_t* buffer,
               size_t* pos) {
  const uint8_t kFmt = 2;
  CreateHeader(kFmt, PT_PSFB, length, buffer, pos);
  AssignUWord32(buffer, pos, sli.SenderSSRC);
  AssignUWord32(buffer, pos, sli.MediaSSRC);

  AssignUWord8(buffer, pos, sli_item.FirstMB >> 5);
  AssignUWord8(buffer, pos, (sli_item.FirstMB << 3) +
                            ((sli_item.NumberOfMB >> 10) & 0x07));
  AssignUWord8(buffer, pos, sli_item.NumberOfMB >> 2);
  AssignUWord8(buffer, pos, (sli_item.NumberOfMB << 6) + sli_item.PictureId);
}











void CreateNack(const RTCPPacketRTPFBNACK& nack,
                const std::vector<RTCPPacketRTPFBNACKItem>& nack_fields,
                size_t length,
                uint8_t* buffer,
                size_t* pos) {
  const uint8_t kFmt = 1;
  CreateHeader(kFmt, PT_RTPFB, length, buffer, pos);
  AssignUWord32(buffer, pos, nack.SenderSSRC);
  AssignUWord32(buffer, pos, nack.MediaSSRC);
  for (std::vector<RTCPPacketRTPFBNACKItem>::const_iterator
      it = nack_fields.begin(); it != nack_fields.end(); ++it) {
    AssignUWord16(buffer, pos, (*it).PacketID);
    AssignUWord16(buffer, pos, (*it).BitMask);
  }
}













void CreateRpsi(const RTCPPacketPSFBRPSI& rpsi,
                uint8_t padding_bytes,
                size_t length,
                uint8_t* buffer,
                size_t* pos) {
  
  assert(rpsi.NumberOfValidBits % 8 == 0);
  const uint8_t kFmt = 3;
  CreateHeader(kFmt, PT_PSFB, length, buffer, pos);
  AssignUWord32(buffer, pos, rpsi.SenderSSRC);
  AssignUWord32(buffer, pos, rpsi.MediaSSRC);
  AssignUWord8(buffer, pos, padding_bytes * 8);
  AssignUWord8(buffer, pos, rpsi.PayloadType);
  memcpy(buffer + *pos, rpsi.NativeBitString, rpsi.NumberOfValidBits / 8);
  *pos += rpsi.NumberOfValidBits / 8;
  memset(buffer + *pos, 0, padding_bytes);
  *pos += padding_bytes;
}













void CreateFir(const RTCPPacketPSFBFIR& fir,
               const RTCPPacketPSFBFIRItem& fir_item,
               size_t length,
               uint8_t* buffer,
               size_t* pos) {
  const uint8_t kFmt = 4;
  CreateHeader(kFmt, PT_PSFB, length, buffer, pos);
  AssignUWord32(buffer, pos, fir.SenderSSRC);
  AssignUWord32(buffer, pos, kUnusedMediaSourceSsrc0);
  AssignUWord32(buffer, pos, fir_item.SSRC);
  AssignUWord8(buffer, pos, fir_item.CommandSequenceNumber);
  AssignUWord24(buffer, pos, 0);
}

void CreateTmmbrItem(const RTCPPacketRTPFBTMMBRItem& tmmbr_item,
                     uint8_t* buffer,
                     size_t* pos) {
  uint32_t bitrate_bps = tmmbr_item.MaxTotalMediaBitRate * 1000;
  uint32_t mantissa = 0;
  uint8_t exp = 0;
  ComputeMantissaAnd6bitBase2Exponent(bitrate_bps, 17, &mantissa, &exp);

  AssignUWord32(buffer, pos, tmmbr_item.SSRC);
  AssignUWord8(buffer, pos, (exp << 2) + ((mantissa >> 15) & 0x03));
  AssignUWord8(buffer, pos, mantissa >> 7);
  AssignUWord8(buffer, pos, (mantissa << 1) +
                            ((tmmbr_item.MeasuredOverhead >> 8) & 0x01));
  AssignUWord8(buffer, pos, tmmbr_item.MeasuredOverhead);
}













void CreateTmmbr(const RTCPPacketRTPFBTMMBR& tmmbr,
                 const RTCPPacketRTPFBTMMBRItem& tmmbr_item,
                 size_t length,
                 uint8_t* buffer,
                 size_t* pos) {
  const uint8_t kFmt = 3;
  CreateHeader(kFmt, PT_RTPFB, length, buffer, pos);
  AssignUWord32(buffer, pos, tmmbr.SenderSSRC);
  AssignUWord32(buffer, pos, kUnusedMediaSourceSsrc0);
  CreateTmmbrItem(tmmbr_item, buffer, pos);
}













void CreateTmmbn(const RTCPPacketRTPFBTMMBN& tmmbn,
                 const std::vector<RTCPPacketRTPFBTMMBRItem>& tmmbn_items,
                 size_t length,
                 uint8_t* buffer,
                 size_t* pos) {
  const uint8_t kFmt = 4;
  CreateHeader(kFmt, PT_RTPFB, length, buffer, pos);
  AssignUWord32(buffer, pos, tmmbn.SenderSSRC);
  AssignUWord32(buffer, pos, kUnusedMediaSourceSsrc0);
  for (uint8_t i = 0; i < tmmbn_items.size(); ++i) {
    CreateTmmbrItem(tmmbn_items[i], buffer, pos);
  }
}




















void CreateRemb(const RTCPPacketPSFBAPP& remb,
                const RTCPPacketPSFBREMBItem& remb_item,
                size_t length,
                uint8_t* buffer,
                size_t* pos) {
  uint32_t mantissa = 0;
  uint8_t exp = 0;
  ComputeMantissaAnd6bitBase2Exponent(remb_item.BitRate, 18, &mantissa, &exp);

  const uint8_t kFmt = 15;
  CreateHeader(kFmt, PT_PSFB, length, buffer, pos);
  AssignUWord32(buffer, pos, remb.SenderSSRC);
  AssignUWord32(buffer, pos, kUnusedMediaSourceSsrc0);
  AssignUWord8(buffer, pos, 'R');
  AssignUWord8(buffer, pos, 'E');
  AssignUWord8(buffer, pos, 'M');
  AssignUWord8(buffer, pos, 'B');
  AssignUWord8(buffer, pos, remb_item.NumberOfSSRCs);
  AssignUWord8(buffer, pos, (exp << 2) + ((mantissa >> 16) & 0x03));
  AssignUWord8(buffer, pos, mantissa >> 8);
  AssignUWord8(buffer, pos, mantissa);
  for (uint8_t i = 0; i < remb_item.NumberOfSSRCs; ++i) {
    AssignUWord32(buffer, pos, remb_item.SSRCs[i]);
  }
}















void CreateXrHeader(const RTCPPacketXR& header,
                    size_t length,
                    uint8_t* buffer,
                    size_t* pos) {
  CreateHeader(0U, PT_XR, length, buffer, pos);
  AssignUWord32(buffer, pos, header.OriginatorSSRC);
}

void CreateXrBlockHeader(uint8_t block_type,
                         uint16_t block_length,
                         uint8_t* buffer,
                         size_t* pos) {
  AssignUWord8(buffer, pos, block_type);
  AssignUWord8(buffer, pos, 0);
  AssignUWord16(buffer, pos, block_length);
}













void CreateRrtr(const std::vector<RTCPPacketXRReceiverReferenceTimeItem>& rrtrs,
                uint8_t* buffer,
                size_t* pos) {
  const uint16_t kBlockLength = 2;
  for (std::vector<RTCPPacketXRReceiverReferenceTimeItem>::const_iterator it =
       rrtrs.begin(); it != rrtrs.end(); ++it) {
    CreateXrBlockHeader(kBtReceiverReferenceTime, kBlockLength, buffer, pos);
    AssignUWord32(buffer, pos, (*it).NTPMostSignificant);
    AssignUWord32(buffer, pos, (*it).NTPLeastSignificant);
  }
}


















void CreateDlrr(const std::vector<Xr::DlrrBlock>& dlrrs,
                uint8_t* buffer,
                size_t* pos) {
  for (std::vector<Xr::DlrrBlock>::const_iterator it = dlrrs.begin();
       it != dlrrs.end(); ++it) {
    if ((*it).empty()) {
      continue;
    }
    uint16_t block_length = 3 * (*it).size();
    CreateXrBlockHeader(kBtDlrr, block_length, buffer, pos);
    for (Xr::DlrrBlock::const_iterator it_block = (*it).begin();
         it_block != (*it).end(); ++it_block) {
      AssignUWord32(buffer, pos, (*it_block).SSRC);
      AssignUWord32(buffer, pos, (*it_block).LastRR);
      AssignUWord32(buffer, pos, (*it_block).DelayLastRR);
    }
  }
}

























void CreateVoipMetric(const std::vector<RTCPPacketXRVOIPMetricItem>& metrics,
                      uint8_t* buffer,
                      size_t* pos) {
  const uint16_t kBlockLength = 8;
  for (std::vector<RTCPPacketXRVOIPMetricItem>::const_iterator it =
       metrics.begin(); it != metrics.end(); ++it) {
    CreateXrBlockHeader(kBtVoipMetric, kBlockLength, buffer, pos);
    AssignUWord32(buffer, pos, (*it).SSRC);
    AssignUWord8(buffer, pos, (*it).lossRate);
    AssignUWord8(buffer, pos, (*it).discardRate);
    AssignUWord8(buffer, pos, (*it).burstDensity);
    AssignUWord8(buffer, pos, (*it).gapDensity);
    AssignUWord16(buffer, pos, (*it).burstDuration);
    AssignUWord16(buffer, pos, (*it).gapDuration);
    AssignUWord16(buffer, pos, (*it).roundTripDelay);
    AssignUWord16(buffer, pos, (*it).endSystemDelay);
    AssignUWord8(buffer, pos, (*it).signalLevel);
    AssignUWord8(buffer, pos, (*it).noiseLevel);
    AssignUWord8(buffer, pos, (*it).RERL);
    AssignUWord8(buffer, pos, (*it).Gmin);
    AssignUWord8(buffer, pos, (*it).Rfactor);
    AssignUWord8(buffer, pos, (*it).extRfactor);
    AssignUWord8(buffer, pos, (*it).MOSLQ);
    AssignUWord8(buffer, pos, (*it).MOSCQ);
    AssignUWord8(buffer, pos, (*it).RXconfig);
    AssignUWord8(buffer, pos, 0);
    AssignUWord16(buffer, pos, (*it).JBnominal);
    AssignUWord16(buffer, pos, (*it).JBmax);
    AssignUWord16(buffer, pos, (*it).JBabsMax);
  }
}
}  

void RtcpPacket::Append(RtcpPacket* packet) {
  assert(packet);
  appended_packets_.push_back(packet);
}

RawPacket RtcpPacket::Build() const {
  size_t length = 0;
  uint8_t packet[IP_PACKET_SIZE];
  CreateAndAddAppended(packet, &length, IP_PACKET_SIZE);
  return RawPacket(packet, length);
}

void RtcpPacket::Build(uint8_t* packet,
                       size_t* length,
                       size_t max_length) const {
  *length = 0;
  CreateAndAddAppended(packet, length, max_length);
}

void RtcpPacket::CreateAndAddAppended(uint8_t* packet,
                                      size_t* length,
                                      size_t max_length) const {
  Create(packet, length, max_length);
  for (std::vector<RtcpPacket*>::const_iterator it = appended_packets_.begin();
      it != appended_packets_.end(); ++it) {
    (*it)->CreateAndAddAppended(packet, length, max_length);
  }
}

void Empty::Create(uint8_t* packet, size_t* length, size_t max_length) const {
}

void SenderReport::Create(uint8_t* packet,
                          size_t* length,
                          size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateSenderReport(sr_, BlockToHeaderLength(BlockLength()), packet, length);
  CreateReportBlocks(report_blocks_, packet, length);
}

void SenderReport::WithReportBlock(ReportBlock* block) {
  assert(block);
  if (report_blocks_.size() >= kMaxNumberOfReportBlocks) {
    LOG(LS_WARNING) << "Max report blocks reached.";
    return;
  }
  report_blocks_.push_back(block->report_block_);
  sr_.NumberOfReportBlocks = report_blocks_.size();
}

void ReceiverReport::Create(uint8_t* packet,
                            size_t* length,
                            size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateReceiverReport(rr_, BlockToHeaderLength(BlockLength()), packet, length);
  CreateReportBlocks(report_blocks_, packet, length);
}

void ReceiverReport::WithReportBlock(ReportBlock* block) {
  assert(block);
  if (report_blocks_.size() >= kMaxNumberOfReportBlocks) {
    LOG(LS_WARNING) << "Max report blocks reached.";
    return;
  }
  report_blocks_.push_back(block->report_block_);
  rr_.NumberOfReportBlocks = report_blocks_.size();
}

void Ij::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateIj(ij_items_, packet, length);
}

void Ij::WithJitterItem(uint32_t jitter) {
  if (ij_items_.size() >= kMaxNumberOfIjItems) {
    LOG(LS_WARNING) << "Max inter-arrival jitter items reached.";
    return;
  }
  ij_items_.push_back(jitter);
}

void Sdes::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  assert(!chunks_.empty());
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateSdes(chunks_, BlockToHeaderLength(BlockLength()), packet, length);
}

void Sdes::WithCName(uint32_t ssrc, std::string cname) {
  assert(cname.length() <= 0xff);
  if (chunks_.size() >= kMaxNumberOfChunks) {
    LOG(LS_WARNING) << "Max SDES chunks reached.";
    return;
  }
  
  
  
  int null_octets = 4 - ((2 + cname.length()) % 4);
  Chunk chunk;
  chunk.ssrc = ssrc;
  chunk.name = cname;
  chunk.null_octets = null_octets;
  chunks_.push_back(chunk);
}

size_t Sdes::BlockLength() const {
  
  
  
  size_t length = kHeaderLength;
  for (std::vector<Chunk>::const_iterator it = chunks_.begin();
       it != chunks_.end(); ++it) {
    length += 6 + (*it).name.length() + (*it).null_octets;
  }
  assert(length % 4 == 0);
  return length;
}

void Bye::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateBye(bye_, csrcs_, BlockToHeaderLength(BlockLength()), packet, length);
}

void Bye::WithCsrc(uint32_t csrc) {
  if (csrcs_.size() >= kMaxNumberOfCsrcs) {
    LOG(LS_WARNING) << "Max CSRC size reached.";
    return;
  }
  csrcs_.push_back(csrc);
}

void App::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateApp(app_, ssrc_, BlockToHeaderLength(BlockLength()), packet, length);
}

void Pli::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreatePli(pli_, BlockToHeaderLength(BlockLength()), packet, length);
}

void Sli::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateSli(sli_, sli_item_, BlockToHeaderLength(BlockLength()), packet,
            length);
}

void Nack::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  assert(!nack_fields_.empty());
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateNack(nack_, nack_fields_, BlockToHeaderLength(BlockLength()), packet,
             length);
}

void Nack::WithList(const uint16_t* nack_list, int length) {
  assert(nack_list);
  assert(nack_fields_.empty());
  int i = 0;
  while (i < length) {
    uint16_t pid = nack_list[i++];
    
    uint16_t bitmask = 0;
    while (i < length) {
      int shift = static_cast<uint16_t>(nack_list[i] - pid) - 1;
      if (shift >= 0 && shift <= 15) {
        bitmask |= (1 << shift);
        ++i;
      } else {
        break;
      }
    }
    RTCPUtility::RTCPPacketRTPFBNACKItem item;
    item.PacketID = pid;
    item.BitMask = bitmask;
    nack_fields_.push_back(item);
  }
}

void Rpsi::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  assert(rpsi_.NumberOfValidBits > 0);
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateRpsi(rpsi_, padding_bytes_, BlockToHeaderLength(BlockLength()), packet,
             length);
}

void Rpsi::WithPictureId(uint64_t picture_id) {
  const uint32_t kPidBits = 7;
  const uint64_t k7MsbZeroMask = 0x1ffffffffffffffULL;
  uint8_t required_bytes = 0;
  uint64_t shifted_pid = picture_id;
  do {
    ++required_bytes;
    shifted_pid = (shifted_pid >> kPidBits) & k7MsbZeroMask;
  } while (shifted_pid > 0);

  
  
  int pos = 0;
  for (int i = required_bytes - 1; i > 0; i--) {
    rpsi_.NativeBitString[pos++] =
        0x80 | static_cast<uint8_t>(picture_id >> (i * kPidBits));
  }
  rpsi_.NativeBitString[pos++] = static_cast<uint8_t>(picture_id & 0x7f);
  rpsi_.NumberOfValidBits = pos * 8;

  
  padding_bytes_ = 4 - ((2 + required_bytes) % 4);
  if (padding_bytes_ == 4) {
    padding_bytes_ = 0;
  }
}

void Fir::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateFir(fir_, fir_item_, BlockToHeaderLength(BlockLength()), packet,
            length);
}

void Remb::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateRemb(remb_, remb_item_, BlockToHeaderLength(BlockLength()), packet,
             length);
}

void Remb::AppliesTo(uint32_t ssrc) {
  if (remb_item_.NumberOfSSRCs >= kMaxNumberOfSsrcs) {
    LOG(LS_WARNING) << "Max number of REMB feedback SSRCs reached.";
    return;
  }
  remb_item_.SSRCs[remb_item_.NumberOfSSRCs++] = ssrc;
}

void Tmmbr::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateTmmbr(tmmbr_, tmmbr_item_, BlockToHeaderLength(BlockLength()), packet,
              length);
}

void Tmmbn::WithTmmbr(uint32_t ssrc, uint32_t bitrate_kbps, uint16_t overhead) {
  assert(overhead <= 0x1ff);
  if (tmmbn_items_.size() >= kMaxNumberOfTmmbrs) {
    LOG(LS_WARNING) << "Max TMMBN size reached.";
    return;
  }
  RTCPPacketRTPFBTMMBRItem tmmbn_item;
  tmmbn_item.SSRC = ssrc;
  tmmbn_item.MaxTotalMediaBitRate = bitrate_kbps;
  tmmbn_item.MeasuredOverhead = overhead;
  tmmbn_items_.push_back(tmmbn_item);
}

void Tmmbn::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateTmmbn(tmmbn_, tmmbn_items_, BlockToHeaderLength(BlockLength()), packet,
              length);
}

void Xr::Create(uint8_t* packet, size_t* length, size_t max_length) const {
  if (*length + BlockLength() > max_length) {
    LOG(LS_WARNING) << "Max packet size reached.";
    return;
  }
  CreateXrHeader(xr_header_, BlockToHeaderLength(BlockLength()), packet,
                 length);
  CreateRrtr(rrtr_blocks_, packet, length);
  CreateDlrr(dlrr_blocks_, packet, length);
  CreateVoipMetric(voip_metric_blocks_, packet, length);
}

void Xr::WithRrtr(Rrtr* rrtr) {
  assert(rrtr);
  if (rrtr_blocks_.size() >= kMaxNumberOfRrtrBlocks) {
    LOG(LS_WARNING) << "Max RRTR blocks reached.";
    return;
  }
  rrtr_blocks_.push_back(rrtr->rrtr_block_);
}

void Xr::WithDlrr(Dlrr* dlrr) {
  assert(dlrr);
  if (dlrr_blocks_.size() >= kMaxNumberOfDlrrBlocks) {
    LOG(LS_WARNING) << "Max DLRR blocks reached.";
    return;
  }
  dlrr_blocks_.push_back(dlrr->dlrr_block_);
}

void Xr::WithVoipMetric(VoipMetric* voip_metric) {
  assert(voip_metric);
  if (voip_metric_blocks_.size() >= kMaxNumberOfVoipMetricBlocks) {
    LOG(LS_WARNING) << "Max Voip Metric blocks reached.";
    return;
  }
  voip_metric_blocks_.push_back(voip_metric->metric_);
}

size_t Xr::DlrrLength() const {
  const size_t kBlockHeaderLen = 4;
  const size_t kSubBlockLen = 12;
  size_t length = 0;
  for (std::vector<DlrrBlock>::const_iterator it = dlrr_blocks_.begin();
       it != dlrr_blocks_.end(); ++it) {
    if (!(*it).empty()) {
      length += kBlockHeaderLen + kSubBlockLen * (*it).size();
    }
  }
  return length;
}

void Dlrr::WithDlrrItem(uint32_t ssrc,
                        uint32_t last_rr,
                        uint32_t delay_last_rr) {
  if (dlrr_block_.size() >= kMaxNumberOfDlrrItems) {
    LOG(LS_WARNING) << "Max DLRR items reached.";
    return;
  }
  RTCPPacketXRDLRRReportBlockItem dlrr;
  dlrr.SSRC = ssrc;
  dlrr.LastRR = last_rr;
  dlrr.DelayLastRR = delay_last_rr;
  dlrr_block_.push_back(dlrr);
}

}  
}  
