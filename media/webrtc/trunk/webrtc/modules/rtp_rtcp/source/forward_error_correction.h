









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_FORWARD_ERROR_CORRECTION_H_

#include <list>
#include <vector>

#include "modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "system_wrappers/interface/ref_count.h"
#include "system_wrappers/interface/scoped_refptr.h"
#include "typedefs.h"

namespace webrtc {


class FecPacket;







class ForwardErrorCorrection {
 public:
  
  static const unsigned int kMaxMediaPackets = 48u;

  
  
  
  
  class Packet {
   public:
    Packet() : length(0), data(), ref_count_(0) {}
    virtual ~Packet() {}

    
    virtual int32_t AddRef() {
      return ++ref_count_;
    }

    
    
    virtual int32_t Release() {
      int32_t ref_count;
      ref_count = --ref_count_;
      if (ref_count == 0)
        delete this;
      return ref_count;
    }

    uint16_t length;  
    uint8_t data[IP_PACKET_SIZE];  

   private:
    int32_t ref_count_;  
  };

  
  class SortablePacket {
   public:
    
    static bool LessThan(const SortablePacket* first,
                         const SortablePacket* second);

    uint16_t seqNum;
  };

  

















  
  class ReceivedPacket : public SortablePacket {
   public:
    uint32_t ssrc;  
                    
    bool isFec;  
                 
    scoped_refptr<Packet> pkt;  
  };

  



  
  class RecoveredPacket : public SortablePacket {
   public:
    bool wasRecovered;  
                        
                        
    bool returned;  
                    
    uint8_t length_recovery[2];  
                                 
    scoped_refptr<Packet> pkt;  
  };

  typedef std::list<Packet*> PacketList;
  typedef std::list<ReceivedPacket*> ReceivedPacketList;
  typedef std::list<RecoveredPacket*> RecoveredPacketList;

  


  ForwardErrorCorrection(int32_t id);

  virtual ~ForwardErrorCorrection();

  



































  int32_t GenerateFEC(const PacketList& mediaPacketList,
                      uint8_t protectionFactor,
                      int numImportantPackets,
                      bool useUnequalProtection,
                      FecMaskType fec_mask_type,
                      PacketList* fecPacketList);

  



























  int32_t DecodeFEC(ReceivedPacketList* receivedPacketList,
                    RecoveredPacketList* recoveredPacketList);

  
  
  int GetNumberOfFecPackets(int numMediaPackets,
                            int protectionFactor);

  




  static uint16_t PacketOverhead();

  
  
  void ResetState(RecoveredPacketList* recoveredPacketList);

 private:
  typedef std::list<FecPacket*> FecPacketList;

  void GenerateFecUlpHeaders(const PacketList& mediaPacketList,
                             uint8_t* packetMask,
                             bool lBit,
                             int numFecPackets);

  
  
  
  
  
  
  int InsertZerosInBitMasks(const PacketList& media_packets,
                            uint8_t* packet_mask,
                            int num_mask_bytes,
                            int num_fec_packets);

  
  
  
  
  static void InsertZeroColumns(int num_zeros,
                                uint8_t* new_mask,
                                int new_mask_bytes,
                                int num_fec_packets,
                                int new_bit_index);

  
  
  
  
  
  
  
  
  static void CopyColumn(uint8_t* new_mask,
                         int new_mask_bytes,
                         uint8_t* old_mask,
                         int old_mask_bytes,
                         int num_fec_packets,
                         int new_bit_index,
                         int old_bit_index);

  void GenerateFecBitStrings(const PacketList& mediaPacketList,
                             uint8_t* packetMask,
                             int numFecPackets,
                             bool lBit);

  
  void InsertPackets(ReceivedPacketList* receivedPacketList,
                     RecoveredPacketList* recoveredPacketList);

  
  void InsertMediaPacket(ReceivedPacket* rxPacket,
                         RecoveredPacketList* recoveredPacketList);

  
  
  
  
  
  void UpdateCoveringFECPackets(RecoveredPacket* packet);

  
  void InsertFECPacket(ReceivedPacket* rxPacket,
                       const RecoveredPacketList* recoveredPacketList);

  
  static void AssignRecoveredPackets(
      FecPacket* fec_packet,
      const RecoveredPacketList* recovered_packets);

  
  void InsertRecoveredPacket(
      RecoveredPacket* recPacketToInsert,
      RecoveredPacketList* recoveredPacketList);

  
  void AttemptRecover(RecoveredPacketList* recoveredPacketList);

  
  static  void InitRecovery(const FecPacket* fec_packet,
                            RecoveredPacket* recovered);

  
  
  static void XorPackets(const Packet* src_packet,
                         RecoveredPacket* dst_packet);

  
  static  void FinishRecovery(RecoveredPacket* recovered);

  
  void RecoverPacket(const FecPacket* fecPacket,
                     RecoveredPacket* recPacketToInsert);

  
  
  
  
  static int NumCoveredPacketsMissing(const FecPacket* fec_packet);

  static uint16_t LatestSequenceNumber(uint16_t first,
                                       uint16_t second);

  static void DiscardFECPacket(FecPacket* fec_packet);
  static void DiscardOldPackets(RecoveredPacketList* recoveredPacketList);
  static uint16_t ParseSequenceNumber(uint8_t* packet);

  int32_t _id;
  std::vector<Packet> _generatedFecPackets;
  FecPacketList _fecPacketList;
  bool _fecPacketReceived;
};
} 
#endif 
