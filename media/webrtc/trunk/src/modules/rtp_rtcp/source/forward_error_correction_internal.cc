









#include "modules/rtp_rtcp/source/forward_error_correction_internal.h"

#include <cassert>
#include <cstring>

#include "modules/rtp_rtcp/source/fec_private_tables_random.h"
#include "modules/rtp_rtcp/source/fec_private_tables_bursty.h"

namespace {


enum ProtectionMode
{
    kModeNoOverlap,
    kModeOverlap,
    kModeBiasFirstPacket,
};















void FitSubMask(int numMaskBytes,
                int numSubMaskBytes,
                int numRows,
                const uint8_t* subMask,
                uint8_t* packetMask)
{
    if (numMaskBytes == numSubMaskBytes)
    {
        memcpy(packetMask, subMask, numRows * numSubMaskBytes);
    }
    else
    {
        for (int i = 0; i < numRows; i++)
        {
            int pktMaskIdx = i * numMaskBytes;
            int pktMaskIdx2 = i * numSubMaskBytes;
            for (int j = 0; j < numSubMaskBytes; j++)
            {
                packetMask[pktMaskIdx] = subMask[pktMaskIdx2];
                pktMaskIdx++;
                pktMaskIdx2++;
            }
        }
    }
}




















void ShiftFitSubMask(int numMaskBytes,
                     int resMaskBytes,
                     int numColumnShift,
                     int endRow,
                     const uint8_t* subMask,
                     uint8_t* packetMask)
{

    
    const int numBitShifts = (numColumnShift % 8);
    const int numByteShifts = numColumnShift >> 3;

    

    
    for (int i = numColumnShift; i < endRow; i++)
    {
        
        
        int pktMaskIdx = i * numMaskBytes + resMaskBytes - 1 + numByteShifts;
        
        int pktMaskIdx2 =
            (i - numColumnShift) * resMaskBytes + resMaskBytes - 1;

        uint8_t shiftRightCurrByte = 0;
        uint8_t shiftLeftPrevByte = 0;
        uint8_t combNewByte = 0;

        
        
        
        if (numMaskBytes > resMaskBytes)
        {
            shiftLeftPrevByte =
                (subMask[pktMaskIdx2] << (8 - numBitShifts));
            packetMask[pktMaskIdx + 1] = shiftLeftPrevByte;
        }

        
        
        
        for (int j = resMaskBytes - 1; j > 0; j--)
        {
            
            shiftRightCurrByte =
                subMask[pktMaskIdx2] >> numBitShifts;

            
            
            shiftLeftPrevByte =
                (subMask[pktMaskIdx2 - 1] << (8 - numBitShifts));

            
            combNewByte = shiftRightCurrByte | shiftLeftPrevByte;

            
            packetMask[pktMaskIdx] = combNewByte;
            pktMaskIdx--;
            pktMaskIdx2--;
        }
        
        shiftRightCurrByte = subMask[pktMaskIdx2] >> numBitShifts;
        packetMask[pktMaskIdx] = shiftRightCurrByte;

    }
}
}  

namespace webrtc {
namespace internal {

PacketMaskTable::PacketMaskTable(FecMaskType fec_mask_type,
                                 int num_media_packets)
    : fec_mask_type_(InitMaskType(fec_mask_type, num_media_packets)),
      fec_packet_mask_table_(InitMaskTable(fec_mask_type_)) {
}





FecMaskType PacketMaskTable::InitMaskType(FecMaskType fec_mask_type,
                                          int num_media_packets) {
  
  assert(num_media_packets <= static_cast<int>(sizeof(kPacketMaskRandomTbl) /
                                               sizeof(*kPacketMaskRandomTbl)));
  switch (fec_mask_type) {
    case kFecMaskRandom: {
      return kFecMaskRandom;
    }
    case kFecMaskBursty: {
      int max_media_packets = static_cast<int>(sizeof(kPacketMaskBurstyTbl) /
                                               sizeof(*kPacketMaskBurstyTbl));
      if (num_media_packets > max_media_packets) {
        return kFecMaskRandom;
      } else {
        return kFecMaskBursty;
      }
    }
  }
  assert(false);
  return kFecMaskRandom;
}



const uint8_t*** PacketMaskTable::InitMaskTable(FecMaskType fec_mask_type) {
  switch (fec_mask_type) {
    case kFecMaskRandom: {
      return kPacketMaskRandomTbl;
    }
    case kFecMaskBursty: {
      return kPacketMaskBurstyTbl;
    }
  }
  assert(false);
  return kPacketMaskRandomTbl;
}


void RemainingPacketProtection(int numMediaPackets,
                               int numFecRemaining,
                               int numFecForImpPackets,
                               int numMaskBytes,
                               ProtectionMode mode,
                               uint8_t* packetMask,
                               const PacketMaskTable& mask_table)
{
    if (mode == kModeNoOverlap)
    {
        

        const int lBit =
            (numMediaPackets - numFecForImpPackets) > 16 ? 1 : 0;

        const int resMaskBytes =
            (lBit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

        const uint8_t* packetMaskSub21 =
            mask_table.fec_packet_mask_table()
            [numMediaPackets - numFecForImpPackets - 1]
            [numFecRemaining - 1];

        ShiftFitSubMask(numMaskBytes, resMaskBytes, numFecForImpPackets,
                        (numFecForImpPackets + numFecRemaining),
                        packetMaskSub21, packetMask);

    }
    else if (mode == kModeOverlap || mode == kModeBiasFirstPacket)
    {
        

        const uint8_t* packetMaskSub22 =
            mask_table.fec_packet_mask_table()
            [numMediaPackets - 1][numFecRemaining - 1];

        FitSubMask(numMaskBytes, numMaskBytes, numFecRemaining, packetMaskSub22,
                   &packetMask[numFecForImpPackets * numMaskBytes]);

        if (mode == kModeBiasFirstPacket)
        {
            for (int i = 0; i < numFecRemaining; i++)
            {
                int pktMaskIdx = i * numMaskBytes;
                packetMask[pktMaskIdx] = packetMask[pktMaskIdx] | (1 << 7);
            }
        }
    }
    else
    {
        assert(false);
    }

}


void ImportantPacketProtection(int numFecForImpPackets,
                               int numImpPackets,
                               int numMaskBytes,
                               uint8_t* packetMask,
                               const PacketMaskTable& mask_table)
{
    const int lBit = numImpPackets > 16 ? 1 : 0;
    const int numImpMaskBytes =
        (lBit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

    
    const uint8_t* packetMaskSub1 =
        mask_table.fec_packet_mask_table()
        [numImpPackets - 1][numFecForImpPackets - 1];

    FitSubMask(numMaskBytes, numImpMaskBytes,
               numFecForImpPackets, packetMaskSub1, packetMask);

}




int SetProtectionAllocation(int numMediaPackets,
                            int numFecPackets,
                            int numImpPackets)
{

    

    
    float allocPar = 0.5;
    int maxNumFecForImp = allocPar * numFecPackets;

    int numFecForImpPackets = (numImpPackets < maxNumFecForImp) ?
        numImpPackets : maxNumFecForImp;

    
    if (numFecPackets == 1 && (numMediaPackets > 2 * numImpPackets))
    {
        numFecForImpPackets = 0;
    }

    return numFecForImpPackets;
}














































void UnequalProtectionMask(int numMediaPackets,
                           int numFecPackets,
                           int numImpPackets,
                           int numMaskBytes,
                           uint8_t* packetMask,
                           const PacketMaskTable& mask_table)
{

    
    

    ProtectionMode mode = kModeOverlap;
    int numFecForImpPackets = 0;

    if (mode != kModeBiasFirstPacket)
    {
        numFecForImpPackets = SetProtectionAllocation(numMediaPackets,
                                                      numFecPackets,
                                                      numImpPackets);
    }

    int numFecRemaining = numFecPackets - numFecForImpPackets;
    

    
    
    
    if (numFecForImpPackets > 0)
    {
        ImportantPacketProtection(numFecForImpPackets, numImpPackets,
                                  numMaskBytes, packetMask,
                                  mask_table);
    }

    
    
    
    if (numFecRemaining > 0)
    {
        RemainingPacketProtection(numMediaPackets, numFecRemaining,
                                  numFecForImpPackets, numMaskBytes,
                                  mode, packetMask, mask_table);
    }

}

void GeneratePacketMasks(int numMediaPackets,
                         int numFecPackets,
                         int numImpPackets,
                         bool useUnequalProtection,
                         const PacketMaskTable& mask_table,
                         uint8_t* packetMask)
{
    assert(numMediaPackets > 0);
    assert(numFecPackets <= numMediaPackets && numFecPackets > 0);
    assert(numImpPackets <= numMediaPackets && numImpPackets >= 0);

    int lBit = numMediaPackets > 16 ? 1 : 0;
    const int numMaskBytes =
        (lBit == 1) ? kMaskSizeLBitSet : kMaskSizeLBitClear;

    
    if (!useUnequalProtection || numImpPackets == 0)
    {
        
        
        
        memcpy(packetMask,
               mask_table.fec_packet_mask_table()[numMediaPackets - 1]
                                                 [numFecPackets - 1],
               numFecPackets * numMaskBytes);
    }
    else  
    {
        UnequalProtectionMask(numMediaPackets, numFecPackets, numImpPackets,
                              numMaskBytes, packetMask, mask_table);

    } 
} 

}  
}  
