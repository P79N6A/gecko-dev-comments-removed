









#include "modules/rtp_rtcp/source/forward_error_correction.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>

#include "modules/rtp_rtcp/source/forward_error_correction_internal.h"
#include "modules/rtp_rtcp/source/rtp_utility.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {


const uint8_t kRtpHeaderSize = 12;


const uint8_t kFecHeaderSize = 10;


const uint8_t kUlpHeaderSizeLBitSet = (2 + kMaskSizeLBitSet);


const uint8_t kUlpHeaderSizeLBitClear = (2 + kMaskSizeLBitClear);


const uint8_t kTransportOverhead = 28;

enum { kMaxFecPackets = ForwardErrorCorrection::kMaxMediaPackets };




class ProtectedPacket : public ForwardErrorCorrection::SortablePacket {
 public:
  scoped_refptr<ForwardErrorCorrection::Packet> pkt;
};

typedef std::list<ProtectedPacket*> ProtectedPacketList;





class FecPacket : public ForwardErrorCorrection::SortablePacket {
 public:
    ProtectedPacketList protectedPktList;
    uint32_t ssrc;  
    scoped_refptr<ForwardErrorCorrection::Packet> pkt;
};

bool ForwardErrorCorrection::SortablePacket::LessThan(
    const SortablePacket* first,
    const SortablePacket* second) {
  return (first->seqNum != second->seqNum &&
      LatestSequenceNumber(first->seqNum, second->seqNum) == second->seqNum);
}

ForwardErrorCorrection::ForwardErrorCorrection(int32_t id)
    : _id(id),
      _generatedFecPackets(kMaxMediaPackets),
      _fecPacketReceived(false) {
}

ForwardErrorCorrection::~ForwardErrorCorrection() {
}


















int32_t ForwardErrorCorrection::GenerateFEC(
    const PacketList& mediaPacketList,
    uint8_t protectionFactor,
    int numImportantPackets,
    bool useUnequalProtection,
    FecMaskType fec_mask_type,
    PacketList* fecPacketList) {
  if (mediaPacketList.empty()) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s media packet list is empty", __FUNCTION__);
    return -1;
  }
  if (!fecPacketList->empty()) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s FEC packet list is not empty", __FUNCTION__);
    return -1;
  }
  const uint16_t numMediaPackets = mediaPacketList.size();
  bool lBit = (numMediaPackets > 8 * kMaskSizeLBitClear);
  int numMaskBytes = lBit ? kMaskSizeLBitSet : kMaskSizeLBitClear;

  if (numMediaPackets > kMaxMediaPackets) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "%s can only protect %d media packets per frame; %d requested",
                 __FUNCTION__, kMaxMediaPackets, numMediaPackets);
    return -1;
  }

  
  
  if (numImportantPackets > numMediaPackets) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
        "Number of important packets (%d) greater than number of media "
        "packets (%d)", numImportantPackets, numMediaPackets);
    return -1;
  }
  if (numImportantPackets < 0) {
    WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                 "Number of important packets (%d) less than zero",
                 numImportantPackets);
    return -1;
  }
  
  PacketList::const_iterator mediaListIt = mediaPacketList.begin();
  while (mediaListIt != mediaPacketList.end()) {
    Packet* mediaPacket = *mediaListIt;
    assert(mediaPacket);

    if (mediaPacket->length < kRtpHeaderSize) {
      WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
                   "%s media packet (%d bytes) is smaller than RTP header",
                   __FUNCTION__, mediaPacket->length);
      return -1;
    }

    
    if (mediaPacket->length + PacketOverhead() + kTransportOverhead >
        IP_PACKET_SIZE) {
      WEBRTC_TRACE(kTraceError, kTraceRtpRtcp, _id,
          "%s media packet (%d bytes) with overhead is larger than MTU(%d)",
          __FUNCTION__, mediaPacket->length, IP_PACKET_SIZE);
      return -1;
    }
    mediaListIt++;
  }

  int numFecPackets = GetNumberOfFecPackets(numMediaPackets, protectionFactor);
  if (numFecPackets == 0) {
    return 0;
  }

  
  for (int i = 0; i < numFecPackets; i++) {
    memset(_generatedFecPackets[i].data, 0, IP_PACKET_SIZE);
    _generatedFecPackets[i].length = 0;  
    
    fecPacketList->push_back(&_generatedFecPackets[i]);
  }

  const internal::PacketMaskTable mask_table(fec_mask_type, numMediaPackets);

  
  
  uint8_t* packetMask = new uint8_t[numFecPackets * kMaskSizeLBitSet];
  memset(packetMask, 0, numFecPackets * numMaskBytes);
  internal::GeneratePacketMasks(numMediaPackets, numFecPackets,
                                numImportantPackets, useUnequalProtection,
                                mask_table, packetMask);

  int numMaskBits = InsertZerosInBitMasks(mediaPacketList, packetMask,
                                          numMaskBytes, numFecPackets);

  lBit = (numMaskBits > 8 * kMaskSizeLBitClear);

  if (numMaskBits < 0) {
    delete [] packetMask;
    return -1;
  }
  if (lBit) {
    numMaskBytes = kMaskSizeLBitSet;
  }

  GenerateFecBitStrings(mediaPacketList, packetMask, numFecPackets, lBit);
  GenerateFecUlpHeaders(mediaPacketList, packetMask, lBit, numFecPackets);

  delete [] packetMask;
  return 0;
}

int ForwardErrorCorrection::GetNumberOfFecPackets(int numMediaPackets,
                                                  int protectionFactor) {
  
  int numFecPackets = (numMediaPackets * protectionFactor + (1 << 7)) >> 8;
  
  if (protectionFactor > 0 && numFecPackets == 0) {
    numFecPackets = 1;
  }
  assert(numFecPackets <= numMediaPackets);
  return numFecPackets;
}

void ForwardErrorCorrection::GenerateFecBitStrings(
    const PacketList& mediaPacketList,
    uint8_t* packetMask,
    int numFecPackets,
    bool lBit) {
  if (mediaPacketList.empty()) {
    return;
  }
  uint8_t mediaPayloadLength[2];
  const int numMaskBytes = lBit ? kMaskSizeLBitSet : kMaskSizeLBitClear;
  const uint16_t ulpHeaderSize = lBit ?
      kUlpHeaderSizeLBitSet : kUlpHeaderSizeLBitClear;
  const uint16_t fecRtpOffset = kFecHeaderSize + ulpHeaderSize - kRtpHeaderSize;

  for (int i = 0; i < numFecPackets; i++) {
    PacketList::const_iterator mediaListIt = mediaPacketList.begin();
    uint32_t pktMaskIdx = i * numMaskBytes;
    uint32_t mediaPktIdx = 0;
    uint16_t fecPacketLength = 0;
    uint16_t prevSeqNum = ParseSequenceNumber((*mediaListIt)->data);
    while (mediaListIt != mediaPacketList.end()) {
      
      if (packetMask[pktMaskIdx] & (1 << (7 - mediaPktIdx))) {
        Packet* mediaPacket = *mediaListIt;

        
        ModuleRTPUtility::AssignUWord16ToBuffer(
            mediaPayloadLength,
            mediaPacket->length - kRtpHeaderSize);

        fecPacketLength = mediaPacket->length + fecRtpOffset;
        
        if (_generatedFecPackets[i].length == 0) {
          
          memcpy(_generatedFecPackets[i].data, mediaPacket->data, 2);
          
          memcpy(&_generatedFecPackets[i].data[4], &mediaPacket->data[4], 4);
          
          memcpy(&_generatedFecPackets[i].data[8], mediaPayloadLength, 2);

          
          memcpy(&_generatedFecPackets[i].data[kFecHeaderSize + ulpHeaderSize],
                 &mediaPacket->data[kRtpHeaderSize],
                 mediaPacket->length - kRtpHeaderSize);
        } else {
          
          _generatedFecPackets[i].data[0] ^= mediaPacket->data[0];
          _generatedFecPackets[i].data[1] ^= mediaPacket->data[1];

          
          for (uint32_t j = 4; j < 8; j++) {
            _generatedFecPackets[i].data[j] ^= mediaPacket->data[j];
          }

          
          _generatedFecPackets[i].data[8] ^= mediaPayloadLength[0];
          _generatedFecPackets[i].data[9] ^= mediaPayloadLength[1];

          
          for (int32_t j = kFecHeaderSize + ulpHeaderSize;
              j < fecPacketLength; j++) {
            _generatedFecPackets[i].data[j] ^=
                mediaPacket->data[j - fecRtpOffset];
          }
        }
        if (fecPacketLength > _generatedFecPackets[i].length) {
          _generatedFecPackets[i].length = fecPacketLength;
        }
      }
      mediaListIt++;
      if (mediaListIt != mediaPacketList.end()) {
        uint16_t seqNum = ParseSequenceNumber((*mediaListIt)->data);
        mediaPktIdx += static_cast<uint16_t>(seqNum - prevSeqNum);
        prevSeqNum = seqNum;
      }
      if (mediaPktIdx == 8) {
        
        mediaPktIdx = 0;
        pktMaskIdx++;
      }
    }
    assert(_generatedFecPackets[i].length);
    
  }
}

int ForwardErrorCorrection::InsertZerosInBitMasks(
    const PacketList& media_packets,
    uint8_t* packet_mask,
    int num_mask_bytes,
    int num_fec_packets) {
  uint8_t* new_mask = NULL;
  if (media_packets.size() <= 1) {
    return media_packets.size();
  }
  int last_seq_num = ParseSequenceNumber(media_packets.back()->data);
  int first_seq_num = ParseSequenceNumber(media_packets.front()->data);
  int total_missing_seq_nums = static_cast<uint16_t>(last_seq_num -
                                                     first_seq_num) -
                                                     media_packets.size() + 1;
  if (total_missing_seq_nums == 0) {
    
    
    return media_packets.size();
  }
  
  int new_mask_bytes = kMaskSizeLBitClear;
  if (media_packets.size() + total_missing_seq_nums > 8 * kMaskSizeLBitClear) {
    new_mask_bytes = kMaskSizeLBitSet;
  }
  new_mask = new uint8_t[num_fec_packets * kMaskSizeLBitSet];
  memset(new_mask, 0, num_fec_packets * kMaskSizeLBitSet);

  PacketList::const_iterator it = media_packets.begin();
  uint16_t prev_seq_num = first_seq_num;
  ++it;

  
  CopyColumn(new_mask, new_mask_bytes, packet_mask, num_mask_bytes,
             num_fec_packets, 0, 0);
  int new_bit_index = 1;
  int old_bit_index = 1;
  
  for (; it != media_packets.end(); ++it) {
    if (new_bit_index == 8 * kMaskSizeLBitSet) {
      
      break;
    }
    uint16_t seq_num = ParseSequenceNumber((*it)->data);
    const int zeros_to_insert =
        static_cast<uint16_t>(seq_num - prev_seq_num - 1);
    if (zeros_to_insert > 0) {
      InsertZeroColumns(zeros_to_insert, new_mask, new_mask_bytes,
                        num_fec_packets, new_bit_index);
    }
    new_bit_index += zeros_to_insert;
    CopyColumn(new_mask, new_mask_bytes, packet_mask, num_mask_bytes,
               num_fec_packets, new_bit_index, old_bit_index);
    ++new_bit_index;
    ++old_bit_index;
    prev_seq_num = seq_num;
  }
  if (new_bit_index % 8 != 0) {
    
    for (uint16_t row = 0; row < num_fec_packets; ++row) {
      int new_byte_index = row * new_mask_bytes + new_bit_index / 8;
      new_mask[new_byte_index] <<= (7 - (new_bit_index % 8));
    }
  }
  
  memcpy(packet_mask, new_mask, kMaskSizeLBitSet * num_fec_packets);
  delete [] new_mask;
  return new_bit_index;
}

void ForwardErrorCorrection::InsertZeroColumns(int num_zeros,
                                               uint8_t* new_mask,
                                               int new_mask_bytes,
                                               int num_fec_packets,
                                               int new_bit_index) {
  for (uint16_t row = 0; row < num_fec_packets; ++row) {
    const int new_byte_index = row * new_mask_bytes + new_bit_index / 8;
    const int max_shifts = (7 - (new_bit_index % 8));
    new_mask[new_byte_index] <<= std::min(num_zeros, max_shifts);
  }
}

void ForwardErrorCorrection::CopyColumn(uint8_t* new_mask,
                                        int new_mask_bytes,
                                        uint8_t* old_mask,
                                        int old_mask_bytes,
                                        int num_fec_packets,
                                        int new_bit_index,
                                        int old_bit_index) {
  
  
  for (uint16_t row = 0; row < num_fec_packets; ++row) {
    int new_byte_index = row * new_mask_bytes + new_bit_index / 8;
    int old_byte_index = row * old_mask_bytes + old_bit_index / 8;
    new_mask[new_byte_index] |= ((old_mask[old_byte_index] & 0x80) >> 7);
    if (new_bit_index % 8 != 7) {
      new_mask[new_byte_index] <<= 1;
    }
    old_mask[old_byte_index] <<= 1;
  }
}

void ForwardErrorCorrection::GenerateFecUlpHeaders(
    const PacketList& mediaPacketList,
    uint8_t* packetMask,
    bool lBit,
    int numFecPackets) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PacketList::const_iterator mediaListIt = mediaPacketList.begin();
  Packet* mediaPacket = *mediaListIt;
  assert(mediaPacket != NULL);
  int numMaskBytes = lBit ? kMaskSizeLBitSet : kMaskSizeLBitClear;
  const uint16_t ulpHeaderSize = lBit ?
      kUlpHeaderSizeLBitSet : kUlpHeaderSizeLBitClear;

  for (int i = 0; i < numFecPackets; i++) {
    
    _generatedFecPackets[i].data[0] &= 0x7f; 
    if (lBit == 0) {
      _generatedFecPackets[i].data[0] &= 0xbf; 
    } else {
      _generatedFecPackets[i].data[0] |= 0x40; 
    }
    
    
    
    memcpy(&_generatedFecPackets[i].data[2], &mediaPacket->data[2], 2);

    
    
    
    ModuleRTPUtility::AssignUWord16ToBuffer(&_generatedFecPackets[i].data[10],
        _generatedFecPackets[i].length - kFecHeaderSize - ulpHeaderSize);

    
    memcpy(&_generatedFecPackets[i].data[12], &packetMask[i * numMaskBytes],
           numMaskBytes);
  }
}

void ForwardErrorCorrection::ResetState(
    RecoveredPacketList* recoveredPacketList) {
  _fecPacketReceived = false;

  
  while (!recoveredPacketList->empty()) {
    delete recoveredPacketList->front();
    recoveredPacketList->pop_front();
  }
  assert(recoveredPacketList->empty());

  
  while (!_fecPacketList.empty()) {
    FecPacketList::iterator fecPacketListIt = _fecPacketList.begin();
    FecPacket* fecPacket = *fecPacketListIt;
    ProtectedPacketList::iterator protectedPacketListIt;
    protectedPacketListIt = fecPacket->protectedPktList.begin();
    while (protectedPacketListIt != fecPacket->protectedPktList.end()) {
      delete *protectedPacketListIt;
      protectedPacketListIt =
          fecPacket->protectedPktList.erase(protectedPacketListIt);
    }
    assert(fecPacket->protectedPktList.empty());
    delete fecPacket;
    _fecPacketList.pop_front();
  }
  assert(_fecPacketList.empty());
}

void ForwardErrorCorrection::InsertMediaPacket(
    ReceivedPacket* rxPacket,
    RecoveredPacketList* recoveredPacketList) {
  RecoveredPacketList::iterator recoveredPacketListIt =
      recoveredPacketList->begin();

  
  while (recoveredPacketListIt != recoveredPacketList->end()) {
    if (rxPacket->seqNum == (*recoveredPacketListIt)->seqNum) {
      
      
      rxPacket->pkt = NULL;
      return;
    }
    recoveredPacketListIt++;
  }
  RecoveredPacket* recoverdPacketToInsert = new RecoveredPacket;
  recoverdPacketToInsert->wasRecovered = false;
  
  recoverdPacketToInsert->returned = true;
  recoverdPacketToInsert->seqNum = rxPacket->seqNum;
  recoverdPacketToInsert->pkt = rxPacket->pkt;
  recoverdPacketToInsert->pkt->length = rxPacket->pkt->length;

  
  
  recoveredPacketList->push_back(recoverdPacketToInsert);
  recoveredPacketList->sort(SortablePacket::LessThan);
  UpdateCoveringFECPackets(recoverdPacketToInsert);
}

void ForwardErrorCorrection::UpdateCoveringFECPackets(RecoveredPacket* packet) {
  for (FecPacketList::iterator it = _fecPacketList.begin();
      it != _fecPacketList.end(); ++it) {
    
    ProtectedPacketList::iterator protected_it = std::lower_bound(
        (*it)->protectedPktList.begin(),
        (*it)->protectedPktList.end(),
        packet,
        SortablePacket::LessThan);
    if (protected_it != (*it)->protectedPktList.end() &&
        (*protected_it)->seqNum == packet->seqNum) {
      
      (*protected_it)->pkt = packet->pkt;
    }
  }
}

void ForwardErrorCorrection::InsertFECPacket(
    ReceivedPacket* rxPacket,
    const RecoveredPacketList* recoveredPacketList) {
  _fecPacketReceived = true;

  
  FecPacketList::iterator fecPacketListIt = _fecPacketList.begin();
  while (fecPacketListIt != _fecPacketList.end()) {
    if (rxPacket->seqNum == (*fecPacketListIt)->seqNum) {
      
      rxPacket->pkt = NULL;
      return;
    }
    fecPacketListIt++;
  }
  FecPacket* fecPacket = new FecPacket;
  fecPacket->pkt = rxPacket->pkt;
  fecPacket->seqNum = rxPacket->seqNum;
  fecPacket->ssrc = rxPacket->ssrc;

  const uint16_t seqNumBase = ModuleRTPUtility::BufferToUWord16(
      &fecPacket->pkt->data[2]);
  const uint16_t maskSizeBytes = (fecPacket->pkt->data[0] & 0x40) ?
      kMaskSizeLBitSet : kMaskSizeLBitClear;  

  for (uint16_t byteIdx = 0; byteIdx < maskSizeBytes; byteIdx++) {
    uint8_t packetMask = fecPacket->pkt->data[12 + byteIdx];
    for (uint16_t bitIdx = 0; bitIdx < 8; bitIdx++) {
      if (packetMask & (1 << (7 - bitIdx))) {
        ProtectedPacket* protectedPacket = new ProtectedPacket;
        fecPacket->protectedPktList.push_back(protectedPacket);
        
        protectedPacket->seqNum = static_cast<uint16_t>(seqNumBase +
            (byteIdx << 3) + bitIdx);
        protectedPacket->pkt = NULL;
      }
    }
  }
  if (fecPacket->protectedPktList.empty()) {
    
    WEBRTC_TRACE(kTraceWarning, kTraceRtpRtcp, _id,
                 "FEC packet %u has an all-zero packet mask.",
                 fecPacket->seqNum, __FUNCTION__);
    delete fecPacket;
  } else {
    AssignRecoveredPackets(fecPacket,
                           recoveredPacketList);
    
    
    _fecPacketList.push_back(fecPacket);
    _fecPacketList.sort(SortablePacket::LessThan);
    if (_fecPacketList.size() > kMaxFecPackets) {
      DiscardFECPacket(_fecPacketList.front());
      _fecPacketList.pop_front();
    }
    assert(_fecPacketList.size() <= kMaxFecPackets);
  }
}

void ForwardErrorCorrection::AssignRecoveredPackets(
    FecPacket* fec_packet,
    const RecoveredPacketList* recovered_packets) {
  
  
  ProtectedPacketList* not_recovered = &fec_packet->protectedPktList;
  RecoveredPacketList already_recovered;
  std::set_intersection(
      recovered_packets->begin(), recovered_packets->end(),
      not_recovered->begin(), not_recovered->end(),
      std::inserter(already_recovered, already_recovered.end()),
      SortablePacket::LessThan);
  
  
  ProtectedPacketList::iterator not_recovered_it = not_recovered->begin();
  for (RecoveredPacketList::iterator it = already_recovered.begin();
      it != already_recovered.end(); ++it) {
    
    while ((*not_recovered_it)->seqNum != (*it)->seqNum)
      ++not_recovered_it;
    (*not_recovered_it)->pkt = (*it)->pkt;
  }
}

void ForwardErrorCorrection::InsertPackets(
    ReceivedPacketList* receivedPacketList,
    RecoveredPacketList* recoveredPacketList) {

  while (!receivedPacketList->empty()) {
    ReceivedPacket* rxPacket = receivedPacketList->front();

    if (rxPacket->isFec) {
      InsertFECPacket(rxPacket, recoveredPacketList);
    } else {
      
      InsertMediaPacket(rxPacket, recoveredPacketList);
    }
    
    delete rxPacket;
    receivedPacketList->pop_front();
  }
  assert(receivedPacketList->empty());
  DiscardOldPackets(recoveredPacketList);
}

void ForwardErrorCorrection::InitRecovery(
    const FecPacket* fec_packet,
    RecoveredPacket* recovered) {
  
  const uint16_t ulpHeaderSize = fec_packet->pkt->data[0] & 0x40 ?
      kUlpHeaderSizeLBitSet : kUlpHeaderSizeLBitClear;  
  recovered->pkt = new Packet;
  memset(recovered->pkt->data, 0, IP_PACKET_SIZE);
  recovered->returned = false;
  recovered->wasRecovered = true;
  uint8_t protectionLength[2];
  
  memcpy(protectionLength, &fec_packet->pkt->data[10], 2);
  
  memcpy(&recovered->pkt->data[kRtpHeaderSize],
         &fec_packet->pkt->data[kFecHeaderSize + ulpHeaderSize],
         ModuleRTPUtility::BufferToUWord16(protectionLength));
  
  memcpy(recovered->length_recovery, &fec_packet->pkt->data[8], 2);
  
  memcpy(recovered->pkt->data, fec_packet->pkt->data, 2);
  
  memcpy(&recovered->pkt->data[4], &fec_packet->pkt->data[4], 4);
  
  ModuleRTPUtility::AssignUWord32ToBuffer(&recovered->pkt->data[8],
                                          fec_packet->ssrc);
}

void ForwardErrorCorrection::FinishRecovery(RecoveredPacket* recovered) {
  
  recovered->pkt->data[0] |= 0x80;  
  recovered->pkt->data[0] &= 0xbf;  

  
  ModuleRTPUtility::AssignUWord16ToBuffer(&recovered->pkt->data[2],
                                          recovered->seqNum);
  
  recovered->pkt->length = ModuleRTPUtility::BufferToUWord16(
      recovered->length_recovery) + kRtpHeaderSize;
}

void ForwardErrorCorrection::XorPackets(const Packet* src_packet,
                                        RecoveredPacket* dst_packet) {
  
  for (uint32_t i = 0; i < 2; i++) {
    dst_packet->pkt->data[i] ^= src_packet->data[i];
  }
  
  for (uint32_t i = 4; i < 8; i++) {
    dst_packet->pkt->data[i] ^= src_packet->data[i];
  }
  
  uint8_t mediaPayloadLength[2];
  ModuleRTPUtility::AssignUWord16ToBuffer(
      mediaPayloadLength,
      src_packet->length - kRtpHeaderSize);
  dst_packet->length_recovery[0] ^= mediaPayloadLength[0];
  dst_packet->length_recovery[1] ^= mediaPayloadLength[1];

  
  
  for (int32_t i = kRtpHeaderSize; i < src_packet->length; i++) {
    dst_packet->pkt->data[i] ^= src_packet->data[i];
  }
}

void ForwardErrorCorrection::RecoverPacket(
    const FecPacket* fecPacket,
    RecoveredPacket* recPacketToInsert) {
  InitRecovery(fecPacket, recPacketToInsert);
  ProtectedPacketList::const_iterator protected_it =
      fecPacket->protectedPktList.begin();
  while (protected_it != fecPacket->protectedPktList.end()) {
    if ((*protected_it)->pkt == NULL) {
      
      recPacketToInsert->seqNum = (*protected_it)->seqNum;
    } else {
      XorPackets((*protected_it)->pkt, recPacketToInsert);
    }
    ++protected_it;
  }
  FinishRecovery(recPacketToInsert);
}

void ForwardErrorCorrection::AttemptRecover(
    RecoveredPacketList* recoveredPacketList) {
  FecPacketList::iterator fecPacketListIt = _fecPacketList.begin();
  while (fecPacketListIt != _fecPacketList.end()) {
    
    int packets_missing = NumCoveredPacketsMissing(*fecPacketListIt);

    
   if (packets_missing == 1) {
      
      RecoveredPacket* packetToInsert = new RecoveredPacket;
      packetToInsert->pkt = NULL;
      RecoverPacket(*fecPacketListIt, packetToInsert);

      
      
      
      
      
      recoveredPacketList->push_back(packetToInsert);
      recoveredPacketList->sort(SortablePacket::LessThan);
      UpdateCoveringFECPackets(packetToInsert);
      DiscardOldPackets(recoveredPacketList);
      DiscardFECPacket(*fecPacketListIt);
      fecPacketListIt = _fecPacketList.erase(fecPacketListIt);

      
      
      
      fecPacketListIt = _fecPacketList.begin();
    } else if (packets_missing == 0) {
        
        
        DiscardFECPacket(*fecPacketListIt);
        fecPacketListIt = _fecPacketList.erase(fecPacketListIt);
    } else {
      fecPacketListIt++;
    }
  }
}

int ForwardErrorCorrection::NumCoveredPacketsMissing(
    const FecPacket* fec_packet) {
  int packets_missing = 0;
  ProtectedPacketList::const_iterator it = fec_packet->protectedPktList.begin();
  for (; it != fec_packet->protectedPktList.end(); ++it) {
    if ((*it)->pkt == NULL) {
      ++packets_missing;
      if (packets_missing > 1) {
        break;  
      }
    }
  }
  return packets_missing;
}

void ForwardErrorCorrection::DiscardFECPacket(FecPacket* fec_packet) {
  while (!fec_packet->protectedPktList.empty()) {
    delete fec_packet->protectedPktList.front();
    fec_packet->protectedPktList.pop_front();
  }
  assert(fec_packet->protectedPktList.empty());
  delete fec_packet;
}

void ForwardErrorCorrection::DiscardOldPackets(
    RecoveredPacketList* recoveredPacketList) {
  while (recoveredPacketList->size() > kMaxMediaPackets) {
    ForwardErrorCorrection::RecoveredPacket* packet =
        recoveredPacketList->front();
    delete packet;
    recoveredPacketList->pop_front();
  }
  assert(recoveredPacketList->size() <= kMaxMediaPackets);
}

uint16_t ForwardErrorCorrection::ParseSequenceNumber(uint8_t* packet) {
  return (packet[2] << 8) + packet[3];
}

int32_t ForwardErrorCorrection::DecodeFEC(
    ReceivedPacketList* receivedPacketList,
    RecoveredPacketList* recoveredPacketList) {
  
  
  if (recoveredPacketList->size() == kMaxMediaPackets) {
    const unsigned int seq_num_diff = abs(
        static_cast<int>(receivedPacketList->front()->seqNum)  -
        static_cast<int>(recoveredPacketList->back()->seqNum));
    if (seq_num_diff > kMaxMediaPackets) {
      
      
      ResetState(recoveredPacketList);
    }
  }
  InsertPackets(receivedPacketList, recoveredPacketList);
  AttemptRecover(recoveredPacketList);
  return 0;
}

uint16_t ForwardErrorCorrection::PacketOverhead() {
  return kFecHeaderSize + kUlpHeaderSizeLBitSet;
}

uint16_t ForwardErrorCorrection::LatestSequenceNumber(uint16_t first,
                                                      uint16_t second) {
  bool wrap = (first < 0x00ff && second > 0xff00) ||
          (first > 0xff00 && second < 0x00ff);
  if (second > first && !wrap)
    return second;
  else if (second <= first && !wrap)
    return first;
  else if (second < first && wrap)
    return second;
  else
    return first;
}

} 
