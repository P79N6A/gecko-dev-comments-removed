









#ifndef WEBRTC_MODULES_VIDEO_CODING_SESSION_INFO_H_
#define WEBRTC_MODULES_VIDEO_CODING_SESSION_INFO_H_

#include <list>

#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/source/packet.h"
#include "typedefs.h"  

namespace webrtc {

class VCMSessionInfo {
 public:
  VCMSessionInfo();

  void UpdateDataPointers(const uint8_t* old_base_ptr,
                          const uint8_t* new_base_ptr);
  
  
  
  int BuildHardNackList(int* seq_num_list,
                        int seq_num_list_length);

  
  
  int BuildSoftNackList(int* seq_num_list,
                        int seq_num_list_length,
                        int rtt_ms);
  void Reset();
  int InsertPacket(const VCMPacket& packet,
                   uint8_t* frame_buffer,
                   bool enable_decodable_state,
                   int rtt_ms);
  bool complete() const;
  bool decodable() const;

  
  
  
  int BuildVP8FragmentationHeader(uint8_t* frame_buffer,
                                  int frame_buffer_length,
                                  RTPFragmentationHeader* fragmentation);

  
  
  
  
  int MakeDecodable();
  int SessionLength() const;
  bool HaveLastPacket() const;
  bool session_nack() const;
  webrtc::FrameType FrameType() const { return frame_type_; }
  int LowSequenceNumber() const;

  
  int HighSequenceNumber() const;
  int PictureId() const;
  int TemporalId() const;
  bool LayerSync() const;
  int Tl0PicId() const;
  bool NonReference() const;
  void SetPreviousFrameLoss() { previous_frame_loss_ = true; }
  bool PreviousFrameLoss() const { return previous_frame_loss_; }

  
  
  int packets_not_decodable() const;

 private:
  enum { kMaxVP8Partitions = 9 };

  typedef std::list<VCMPacket> PacketList;
  typedef PacketList::iterator PacketIterator;
  typedef PacketList::const_iterator PacketIteratorConst;
  typedef PacketList::reverse_iterator ReversePacketIterator;

  void InformOfEmptyPacket(uint16_t seq_num);

  
  
  
  
  
  PacketIterator FindNextPartitionBeginning(PacketIterator it,
                                            int* packets_skipped) const;

  
  
  PacketIterator FindPartitionEnd(PacketIterator it) const;
  static bool InSequence(const PacketIterator& it,
                         const PacketIterator& prev_it);
  static int PacketsMissing(const PacketIterator& packet_it,
                            const PacketIterator& prev_packet_it);
  int InsertBuffer(uint8_t* frame_buffer,
                   PacketIterator packetIterator);
  void ShiftSubsequentPackets(PacketIterator it, int steps_to_shift);
  PacketIterator FindNaluEnd(PacketIterator packet_iter) const;
  
  
  int DeletePacketData(PacketIterator start,
                       PacketIterator end);
  void UpdateCompleteSession();

  
  
  void UpdateDecodableSession(int rtt_ms);

  
  
  
  
  int ClearOutEmptyPacketSequenceNumbers(int* seq_num_list,
                                         int seq_num_list_length,
                                         int index) const;

  
  bool session_nack_;
  bool complete_;
  bool decodable_;
  webrtc::FrameType frame_type_;
  bool previous_frame_loss_;
  
  PacketList packets_;
  int empty_seq_num_low_;
  int empty_seq_num_high_;
  
  int packets_not_decodable_;
};

}  

#endif
