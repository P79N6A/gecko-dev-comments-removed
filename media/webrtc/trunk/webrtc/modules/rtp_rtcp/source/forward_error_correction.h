









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_H_

#include <list>
#include <vector>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/system_wrappers/interface/ref_count.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class FecPacket;





class ForwardErrorCorrection {
 public:
  
  static const unsigned int kMaxMediaPackets = 48u;

  
  
  
  
  class Packet {
   public:
    Packet() : length(0), data(), ref_count_(0) {}
    virtual ~Packet() {}

    
    virtual int32_t AddRef();

    
    
    virtual int32_t Release();

    uint16_t length;               
    uint8_t data[IP_PACKET_SIZE];  

   private:
    int32_t ref_count_;  
  };

  
  class SortablePacket {
   public:
    
    static bool LessThan(const SortablePacket* first,
                         const SortablePacket* second);

    uint16_t seq_num;
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  class ReceivedPacket : public SortablePacket {
   public:
    ReceivedPacket();
    ~ReceivedPacket();

    uint32_t ssrc;  
                    
    bool is_fec;    
                    
    scoped_refptr<Packet> pkt;  
  };

  
  
  
  class RecoveredPacket : public SortablePacket {
   public:
    RecoveredPacket();
    ~RecoveredPacket();

    bool was_recovered;  
                         
                         
    bool returned;  
                    
    uint8_t length_recovery[2];  
                                 
    scoped_refptr<Packet> pkt;   
  };

  typedef std::list<Packet*> PacketList;
  typedef std::list<ReceivedPacket*> ReceivedPacketList;
  typedef std::list<RecoveredPacket*> RecoveredPacketList;

  
  ForwardErrorCorrection(int32_t id);

  virtual ~ForwardErrorCorrection();

  



































  int32_t GenerateFEC(const PacketList& media_packet_list,
                      uint8_t protection_factor, int num_important_packets,
                      bool use_unequal_protection, FecMaskType fec_mask_type,
                      PacketList* fec_packet_list);

  



























  int32_t DecodeFEC(ReceivedPacketList* received_packet_list,
                    RecoveredPacketList* recovered_packet_list);

  
  
  int GetNumberOfFecPackets(int num_media_packets, int protection_factor);

  
  
  
  static uint16_t PacketOverhead();

  
  
  void ResetState(RecoveredPacketList* recovered_packet_list);

 private:
  typedef std::list<FecPacket*> FecPacketList;

  void GenerateFecUlpHeaders(const PacketList& media_packet_list,
                             uint8_t* packet_mask, bool l_bit,
                             int num_fec_packets);

  
  
  
  
  
  
  int InsertZerosInBitMasks(const PacketList& media_packets,
                            uint8_t* packet_mask, int num_mask_bytes,
                            int num_fec_packets);

  
  
  
  
  static void InsertZeroColumns(int num_zeros, uint8_t* new_mask,
                                int new_mask_bytes, int num_fec_packets,
                                int new_bit_index);

  
  
  
  
  
  
  
  
  static void CopyColumn(uint8_t* new_mask, int new_mask_bytes,
                         uint8_t* old_mask, int old_mask_bytes,
                         int num_fec_packets, int new_bit_index,
                         int old_bit_index);

  void GenerateFecBitStrings(const PacketList& media_packet_list,
                             uint8_t* packet_mask, int num_fec_packets,
                             bool l_bit);

  
  void InsertPackets(ReceivedPacketList* received_packet_list,
                     RecoveredPacketList* recovered_packet_list);

  
  void InsertMediaPacket(ReceivedPacket* rx_packet,
                         RecoveredPacketList* recovered_packet_list);

  
  
  
  
  
  void UpdateCoveringFECPackets(RecoveredPacket* packet);

  
  void InsertFECPacket(ReceivedPacket* rx_packet,
                       const RecoveredPacketList* recovered_packet_list);

  
  static void AssignRecoveredPackets(
      FecPacket* fec_packet, const RecoveredPacketList* recovered_packets);

  
  void InsertRecoveredPacket(RecoveredPacket* rec_packet_to_insert,
                             RecoveredPacketList* recovered_packet_list);

  
  void AttemptRecover(RecoveredPacketList* recovered_packet_list);

  
  static void InitRecovery(const FecPacket* fec_packet,
                           RecoveredPacket* recovered);

  
  
  static void XorPackets(const Packet* src_packet, RecoveredPacket* dst_packet);

  
  static void FinishRecovery(RecoveredPacket* recovered);

  
  void RecoverPacket(const FecPacket* fec_packet,
                     RecoveredPacket* rec_packet_to_insert);

  
  
  
  
  static int NumCoveredPacketsMissing(const FecPacket* fec_packet);

  static void DiscardFECPacket(FecPacket* fec_packet);
  static void DiscardOldPackets(RecoveredPacketList* recovered_packet_list);
  static uint16_t ParseSequenceNumber(uint8_t* packet);

  int32_t id_;
  std::vector<Packet> generated_fec_packets_;
  FecPacketList fec_packet_list_;
  bool fec_packet_received_;
};
}  
#endif  
