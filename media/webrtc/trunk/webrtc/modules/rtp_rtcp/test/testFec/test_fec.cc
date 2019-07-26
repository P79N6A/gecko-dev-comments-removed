














#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <list>

#include "modules/rtp_rtcp/source/fec_private_tables_bursty.h"
#include "modules/rtp_rtcp/source/forward_error_correction.h"
#include "modules/rtp_rtcp/source/forward_error_correction_internal.h"

#include "rtp_utility.h"
#include "testsupport/fileutils.h"



using namespace webrtc;

void ReceivePackets(
    ForwardErrorCorrection::ReceivedPacketList* toDecodeList,
    ForwardErrorCorrection::ReceivedPacketList* receivedPacketList,
    WebRtc_UWord32 numPacketsToDecode, float reorderRate, float duplicateRate);

int main() {
  
  enum { kMaxNumberMediaPackets = 48 };
  enum { kMaxNumberFecPackets = 48 };

  const WebRtc_UWord32 kNumMaskBytesL0 = 2;
  const WebRtc_UWord32 kNumMaskBytesL1 = 6;

  
  const bool kUseUnequalProtection = true;

  
  const FecMaskType kMaskTypes[] = {kFecMaskRandom, kFecMaskBursty};
  const int kNumFecMaskTypes = sizeof(kMaskTypes) / sizeof(*kMaskTypes);
  
  const uint16_t kMaxMediaPackets[] = {kMaxNumberMediaPackets,
      sizeof(kPacketMaskBurstyTbl) / sizeof(*kPacketMaskBurstyTbl)};
  if (kMaxMediaPackets[1] != 12) {
    printf("ERROR: max media packets for bursty mode not equal to 12 \n");
    return -1;
  }

  WebRtc_UWord32 id = 0;
  ForwardErrorCorrection fec(id);

  ForwardErrorCorrection::PacketList mediaPacketList;
  ForwardErrorCorrection::PacketList fecPacketList;
  ForwardErrorCorrection::ReceivedPacketList toDecodeList;
  ForwardErrorCorrection::ReceivedPacketList receivedPacketList;
  ForwardErrorCorrection::RecoveredPacketList recoveredPacketList;
  std::list<WebRtc_UWord8*> fecMaskList;

  ForwardErrorCorrection::Packet* mediaPacket = NULL;
  
  const float lossRate[] = {0.5f};
  const WebRtc_UWord32 lossRateSize = sizeof(lossRate)/sizeof(*lossRate);
  const float reorderRate = 0.1f;
  const float duplicateRate = 0.1f;

  WebRtc_UWord8 mediaLossMask[kMaxNumberMediaPackets];
  WebRtc_UWord8 fecLossMask[kMaxNumberFecPackets];
  WebRtc_UWord8 fecPacketMasks[kMaxNumberFecPackets][kMaxNumberMediaPackets];

  
  
  const unsigned int randomSeed = static_cast<unsigned int>(time(NULL));
  srand(randomSeed);
  std::string filename = webrtc::test::OutputPath() + "randomSeedLog.txt";
  FILE* randomSeedFile = fopen(filename.c_str(), "a");
  fprintf(randomSeedFile, "%u\n", randomSeed);
  fclose(randomSeedFile);
  randomSeedFile = NULL;

  WebRtc_UWord16 seqNum = static_cast<WebRtc_UWord16>(rand());
  WebRtc_UWord32 timeStamp = static_cast<WebRtc_UWord32>(rand());
  const WebRtc_UWord32 ssrc = static_cast<WebRtc_UWord32>(rand());

  
  for (int mask_type_idx = 0; mask_type_idx < kNumFecMaskTypes;
      ++mask_type_idx) {

    for (WebRtc_UWord32 lossRateIdx = 0; lossRateIdx < lossRateSize;
        ++lossRateIdx) {

      printf("Loss rate: %.2f, Mask type %d \n", lossRate[lossRateIdx],
             mask_type_idx);

      const WebRtc_UWord32 packetMaskMax = kMaxMediaPackets[mask_type_idx];
      WebRtc_UWord8* packetMask =
          new WebRtc_UWord8[packetMaskMax * kNumMaskBytesL1];

      FecMaskType fec_mask_type = kMaskTypes[mask_type_idx];

      for (WebRtc_UWord32 numMediaPackets = 1;
          numMediaPackets <= packetMaskMax;
          numMediaPackets++) {
        internal::PacketMaskTable mask_table(fec_mask_type, numMediaPackets);

        for (WebRtc_UWord32 numFecPackets = 1;
            numFecPackets <= numMediaPackets &&
            numFecPackets <= packetMaskMax;
            numFecPackets++) {

          
          
          WebRtc_UWord32 maxNumImpPackets = numMediaPackets / 2 + 1;
          for (WebRtc_UWord32 numImpPackets = 0;
              numImpPackets <= maxNumImpPackets &&
              numImpPackets <= packetMaskMax;
              numImpPackets++) {

            WebRtc_UWord8 protectionFactor = static_cast<WebRtc_UWord8>
            (numFecPackets * 255 / numMediaPackets);

            const WebRtc_UWord32 maskBytesPerFecPacket =
                (numMediaPackets > 16) ? kNumMaskBytesL1 : kNumMaskBytesL0;

            memset(packetMask, 0, numMediaPackets * maskBytesPerFecPacket);

            
            internal::GeneratePacketMasks(numMediaPackets,
                                          numFecPackets,
                                          numImpPackets,
                                          kUseUnequalProtection,
                                          mask_table,
                                          packetMask);

#ifdef VERBOSE_OUTPUT
            printf("%u media packets, %u FEC packets, %u numImpPackets, "
                "loss rate = %.2f \n",
                numMediaPackets, numFecPackets, numImpPackets,
                lossRate[lossRateIdx]);
            printf("Packet mask matrix \n");
#endif

            for (WebRtc_UWord32 i = 0; i < numFecPackets; i++) {
              for (WebRtc_UWord32 j = 0; j < numMediaPackets; j++) {
                const WebRtc_UWord8 byteMask =
                    packetMask[i * maskBytesPerFecPacket + j / 8];
                const WebRtc_UWord32 bitPosition = (7 - j % 8);
                fecPacketMasks[i][j] =
                    (byteMask & (1 << bitPosition)) >> bitPosition;
#ifdef VERBOSE_OUTPUT
                printf("%u ", fecPacketMasks[i][j]);
#endif
              }
#ifdef VERBOSE_OUTPUT
              printf("\n");
#endif
            }
#ifdef VERBOSE_OUTPUT
            printf("\n");
#endif
            
            WebRtc_UWord32 rowLimit = numMediaPackets;
            for (WebRtc_UWord32 i = 0; i < numFecPackets; ++i) {
              WebRtc_UWord32 rowSum = 0;
              for (WebRtc_UWord32 j = 0; j < rowLimit; ++j) {
                rowSum += fecPacketMasks[i][j];
              }
              if (rowSum == 0) {
                printf("ERROR: row is all zero %d \n", i);
                return -1;
              }
            }
            for (WebRtc_UWord32 j = 0; j < rowLimit; ++j) {
              WebRtc_UWord32 columnSum = 0;
              for (WebRtc_UWord32 i = 0; i < numFecPackets; ++i) {
                columnSum += fecPacketMasks[i][j];
              }
              if (columnSum == 0) {
                printf("ERROR: column is all zero %d \n", j);
                return -1;
              }
            }

            
            for (WebRtc_UWord32 i = 0; i < numMediaPackets; ++i)  {
              mediaPacket = new ForwardErrorCorrection::Packet;
              mediaPacketList.push_back(mediaPacket);
              mediaPacket->length =
                  static_cast<WebRtc_UWord16>((static_cast<float>(rand()) /
                      RAND_MAX) * (IP_PACKET_SIZE - 12 -
                          28 - ForwardErrorCorrection::PacketOverhead()));
              if (mediaPacket->length < 12) {
                mediaPacket->length = 12;
              }
              
              mediaPacket->data[0] = static_cast<WebRtc_UWord8>(rand() % 256);
              mediaPacket->data[1] = static_cast<WebRtc_UWord8>(rand() % 256);

              
              
              
              
              
              
              mediaPacket->data[0] |= 0x80;
              mediaPacket->data[0] &= 0xbf;

              
              
              
              
              
              mediaPacket->data[1] &= 0x7f;

              ModuleRTPUtility::AssignUWord16ToBuffer(&mediaPacket->data[2],
                                                      seqNum);
              ModuleRTPUtility::AssignUWord32ToBuffer(&mediaPacket->data[4],
                                                      timeStamp);
              ModuleRTPUtility::AssignUWord32ToBuffer(&mediaPacket->data[8],
                                                      ssrc);
              
              for (WebRtc_Word32 j = 12; j < mediaPacket->length; ++j)  {
                mediaPacket->data[j] =
                    static_cast<WebRtc_UWord8> (rand() % 256);
              }
              seqNum++;
            }
            mediaPacket->data[1] |= 0x80;

            if (fec.GenerateFEC(mediaPacketList, protectionFactor,
                                numImpPackets, kUseUnequalProtection,
                                fec_mask_type, &fecPacketList) != 0) {
              printf("Error: GenerateFEC() failed\n");
              return -1;
            }

            if (fecPacketList.size() != numFecPackets) {
              printf("Error: we requested %u FEC packets, "
                  "but GenerateFEC() produced %u\n",
                  numFecPackets,
                  static_cast<WebRtc_UWord32>(fecPacketList.size()));
              return -1;
            }
            memset(mediaLossMask, 0, sizeof(mediaLossMask));
            ForwardErrorCorrection::PacketList::iterator
                mediaPacketListItem = mediaPacketList.begin();
            ForwardErrorCorrection::ReceivedPacket* receivedPacket;
            WebRtc_UWord32 mediaPacketIdx = 0;

            while (mediaPacketListItem != mediaPacketList.end()) {
              mediaPacket = *mediaPacketListItem;
              
              const float lossRandomVariable = (static_cast<float>(rand()) /
                  (RAND_MAX));

              if (lossRandomVariable >= lossRate[lossRateIdx]) {
                mediaLossMask[mediaPacketIdx] = 1;
                receivedPacket =
                    new ForwardErrorCorrection::ReceivedPacket;
                receivedPacket->pkt = new ForwardErrorCorrection::Packet;
                receivedPacketList.push_back(receivedPacket);

                receivedPacket->pkt->length = mediaPacket->length;
                memcpy(receivedPacket->pkt->data, mediaPacket->data,
                       mediaPacket->length);
                receivedPacket->seqNum =
                    ModuleRTPUtility::BufferToUWord16(&mediaPacket->data[2]);
                receivedPacket->isFec = false;
              }
              mediaPacketIdx++;
              ++mediaPacketListItem;
            }
            memset(fecLossMask, 0, sizeof(fecLossMask));
            ForwardErrorCorrection::PacketList::iterator
                fecPacketListItem = fecPacketList.begin();
            ForwardErrorCorrection::Packet* fecPacket;
            WebRtc_UWord32 fecPacketIdx = 0;
            while (fecPacketListItem != fecPacketList.end()) {
              fecPacket = *fecPacketListItem;
              const float lossRandomVariable =
                  (static_cast<float>(rand()) / (RAND_MAX));
              if (lossRandomVariable >= lossRate[lossRateIdx]) {
                fecLossMask[fecPacketIdx] = 1;
                receivedPacket =
                    new ForwardErrorCorrection::ReceivedPacket;
                receivedPacket->pkt = new ForwardErrorCorrection::Packet;

                receivedPacketList.push_back(receivedPacket);

                receivedPacket->pkt->length = fecPacket->length;
                memcpy(receivedPacket->pkt->data, fecPacket->data,
                       fecPacket->length);

                receivedPacket->seqNum = seqNum;
                receivedPacket->isFec = true;
                receivedPacket->ssrc = ssrc;

                fecMaskList.push_back(fecPacketMasks[fecPacketIdx]);
              }
              ++fecPacketIdx;
              ++seqNum;
              ++fecPacketListItem;
            }

#ifdef VERBOSE_OUTPUT
            printf("Media loss mask:\n");
            for (WebRtc_UWord32 i = 0; i < numMediaPackets; i++) {
              printf("%u ", mediaLossMask[i]);
            }
            printf("\n\n");

            printf("FEC loss mask:\n");
            for (WebRtc_UWord32 i = 0; i < numFecPackets; i++) {
              printf("%u ", fecLossMask[i]);
            }
            printf("\n\n");
#endif

            std::list<WebRtc_UWord8*>::iterator fecMaskIt = fecMaskList.begin();
            WebRtc_UWord8* fecMask;
            while (fecMaskIt != fecMaskList.end()) {
              fecMask = *fecMaskIt;
              WebRtc_UWord32 hammingDist = 0;
              WebRtc_UWord32 recoveryPosition = 0;
              for (WebRtc_UWord32 i = 0; i < numMediaPackets; i++) {
                if (mediaLossMask[i] == 0 && fecMask[i] == 1) {
                  recoveryPosition = i;
                  ++hammingDist;
                }
              }
              std::list<WebRtc_UWord8*>::iterator itemToDelete = fecMaskIt;
              ++fecMaskIt;

              if (hammingDist == 1) {
                
                mediaLossMask[recoveryPosition] = 1;
                fecMaskIt = fecMaskList.begin();
              } else if (hammingDist == 0)  {
                
                fecMaskList.erase(itemToDelete);
              }
            }
#ifdef VERBOSE_OUTPUT
            printf("Recovery mask:\n");
            for (WebRtc_UWord32 i = 0; i < numMediaPackets; ++i) {
              printf("%u ", mediaLossMask[i]);
            }
            printf("\n\n");
#endif
            
            bool fecPacketReceived = false;
            while (!receivedPacketList.empty()) {
              WebRtc_UWord32 numPacketsToDecode = static_cast<WebRtc_UWord32>
                  ((static_cast<float>(rand()) / RAND_MAX) *
                  receivedPacketList.size() + 0.5);
              if (numPacketsToDecode < 1) {
                numPacketsToDecode = 1;
              }
              ReceivePackets(&toDecodeList, &receivedPacketList,
                             numPacketsToDecode, reorderRate, duplicateRate);

              if (fecPacketReceived == false) {
                ForwardErrorCorrection::ReceivedPacketList::iterator
                    toDecodeIt = toDecodeList.begin();
                while (toDecodeIt != toDecodeList.end()) {
                  receivedPacket = *toDecodeIt;
                  if (receivedPacket->isFec) {
                    fecPacketReceived = true;
                  }
                  ++toDecodeIt;
                }
              }
              if (fec.DecodeFEC(&toDecodeList, &recoveredPacketList)
                  != 0) {
                printf("Error: DecodeFEC() failed\n");
                return -1;
              }
              if (!toDecodeList.empty()) {
                printf("Error: received packet list is not empty\n");
                return -1;
              }
            }
            mediaPacketListItem = mediaPacketList.begin();
            mediaPacketIdx = 0;
            while (mediaPacketListItem != mediaPacketList.end()) {
              if (mediaLossMask[mediaPacketIdx] == 1) {
                
                ForwardErrorCorrection::RecoveredPacketList::iterator
                    recoveredPacketListItem = recoveredPacketList.begin();

                if (recoveredPacketListItem == recoveredPacketList.end()) {
                  printf("Error: insufficient number of recovered packets.\n");
                  return -1;
                }
                mediaPacket = *mediaPacketListItem;
                ForwardErrorCorrection::RecoveredPacket* recoveredPacket =
                    *recoveredPacketListItem;

                if (recoveredPacket->pkt->length != mediaPacket->length) {
                  printf("Error: recovered packet length not identical to "
                      "original media packet\n");
                  return -1;
                }
                if (memcmp(recoveredPacket->pkt->data, mediaPacket->data,
                           mediaPacket->length) != 0) {
                  printf("Error: recovered packet payload not identical to "
                      "original media packet\n");
                  return -1;
                }
                delete recoveredPacket;
                recoveredPacketList.pop_front();
              }
              ++mediaPacketIdx;
              ++mediaPacketListItem;
            }
            fec.ResetState(&recoveredPacketList);
            if (!recoveredPacketList.empty()) {
              printf("Error: excessive number of recovered packets.\n");
              printf("\t size is:%u\n",
                     static_cast<WebRtc_UWord32>(recoveredPacketList.size()));
              return -1;
            }
            
            mediaPacketListItem = mediaPacketList.begin();
            while (mediaPacketListItem != mediaPacketList.end()) {
              delete *mediaPacketListItem;
              ++mediaPacketListItem;
              mediaPacketList.pop_front();
            }
            assert(mediaPacketList.empty());

            fecPacketListItem = fecPacketList.begin();
            while (fecPacketListItem != fecPacketList.end()) {
              ++fecPacketListItem;
              fecPacketList.pop_front();
            }

            
            
            ForwardErrorCorrection::ReceivedPacketList::iterator
                receivedPacketIt = receivedPacketList.begin();
            while (receivedPacketIt != receivedPacketList.end()) {
              receivedPacket = *receivedPacketIt;
              delete receivedPacket;
              ++receivedPacketIt;
              receivedPacketList.pop_front();
            }
            assert(receivedPacketList.empty());

            while (!fecMaskList.empty()) {
              fecMaskList.pop_front();
            }
            timeStamp += 90000 / 30;
          }  
        }  
      }  
      delete [] packetMask;
    }  
  }  

  
  fec.ResetState(&recoveredPacketList);
  if (!recoveredPacketList.empty()) {
    printf("Error: recovered packet list is not empty\n");
    return -1;
  }
  printf("\nAll tests passed successfully\n");
  return 0;
}

void ReceivePackets(
    ForwardErrorCorrection::ReceivedPacketList* toDecodeList,
    ForwardErrorCorrection::ReceivedPacketList* receivedPacketList,
    WebRtc_UWord32 numPacketsToDecode, float reorderRate, float duplicateRate) {
  assert(toDecodeList->empty());
  assert(numPacketsToDecode <= receivedPacketList->size());

  ForwardErrorCorrection::ReceivedPacketList::iterator it;
  for (WebRtc_UWord32 i = 0; i < numPacketsToDecode; i++) {
    it = receivedPacketList->begin();
    
    float randomVariable = static_cast<float>(rand()) / RAND_MAX;
    while (randomVariable < reorderRate) {
      ++it;
      if (it == receivedPacketList->end()) {
        --it;
        break;
      }
      randomVariable = static_cast<float>(rand()) / RAND_MAX;
    }
    ForwardErrorCorrection::ReceivedPacket* receivedPacket = *it;
    toDecodeList->push_back(receivedPacket);

    
    randomVariable = static_cast<float>(rand()) / RAND_MAX;
    while (randomVariable < duplicateRate) {
      ForwardErrorCorrection::ReceivedPacket* duplicatePacket =
          new ForwardErrorCorrection::ReceivedPacket;
      *duplicatePacket = *receivedPacket;
      duplicatePacket->pkt = new ForwardErrorCorrection::Packet;
      memcpy(duplicatePacket->pkt->data, receivedPacket->pkt->data,
             receivedPacket->pkt->length);
      duplicatePacket->pkt->length = receivedPacket->pkt->length;

      toDecodeList->push_back(duplicatePacket);
      randomVariable = static_cast<float>(rand()) / RAND_MAX;
    }
    receivedPacketList->erase(it);
  }
}
