









#include "modules/rtp_rtcp/source/forward_error_correction.h"

#include <gtest/gtest.h>
#include <list>

#include "rtp_utility.h"

using webrtc::ForwardErrorCorrection;


const uint8_t kRtpHeaderSize = 12;


const uint8_t kTransportOverhead = 28;


const uint8_t kMaxNumberMediaPackets = ForwardErrorCorrection::kMaxMediaPackets;

typedef std::list<ForwardErrorCorrection::Packet*> PacketList;
typedef std::list<ForwardErrorCorrection::ReceivedPacket*> ReceivedPacketList;
typedef std::list<ForwardErrorCorrection::RecoveredPacket*> RecoveredPacketList;

template<typename T> void ClearList(std::list<T*>* my_list) {
  T* packet = NULL;
  while (!my_list->empty()) {
    packet = my_list->front();
    delete packet;
    my_list->pop_front();
  }
}

class RtpFecTest : public ::testing::Test {
 protected:
  RtpFecTest()
      :  fec_(new ForwardErrorCorrection(0)),
         ssrc_(rand()),
         fec_seq_num_(0) {
  }

  ForwardErrorCorrection* fec_;
  int ssrc_;
  uint16_t fec_seq_num_;

  PacketList media_packet_list_;
  PacketList fec_packet_list_;
  ReceivedPacketList received_packet_list_;
  RecoveredPacketList recovered_packet_list_;

  
  
  int media_loss_mask_[kMaxNumberMediaPackets];

  
  
  int fec_loss_mask_[kMaxNumberMediaPackets];

  
  
  
  int ConstructMediaPacketsSeqNum(int num_media_packets,
                                  int start_seq_num);
  int ConstructMediaPackets(int num_media_packets);

  
  void NetworkReceivedPackets();

  
  
  
  
  void ReceivedPackets(
      const PacketList& packet_list,
      int* loss_mask,
      bool is_fec);

  
  bool IsRecoveryComplete();

  
  void FreeRecoveredPacketList();

  
  void TearDown();
};

TEST_F(RtpFecTest, HandleIncorrectInputs) {
  int num_important_packets = 0;
  bool use_unequal_protection =  false;
  uint8_t protection_factor = 60;

  
  EXPECT_EQ(-1, fec_->GenerateFEC(media_packet_list_,
                                  protection_factor,
                                  num_important_packets,
                                  use_unequal_protection,
                                  &fec_packet_list_));

  int num_media_packets = 10;
  ConstructMediaPackets(num_media_packets);

  num_important_packets = -1;
  
  EXPECT_EQ(-1, fec_->GenerateFEC(media_packet_list_,
                                  protection_factor,
                                  num_important_packets,
                                  use_unequal_protection,
                                  &fec_packet_list_));

  num_important_packets = 12;
  
  EXPECT_EQ(-1, fec_->GenerateFEC(media_packet_list_,
                                  protection_factor,
                                  num_important_packets,
                                  use_unequal_protection,
                                  &fec_packet_list_));

  num_media_packets = kMaxNumberMediaPackets + 1;
  ConstructMediaPackets(num_media_packets);

  num_important_packets = 0;
  
  EXPECT_EQ(-1, fec_->GenerateFEC(media_packet_list_,
                                  protection_factor,
                                  num_important_packets,
                                  use_unequal_protection,
                                  &fec_packet_list_));
}

TEST_F(RtpFecTest, FecRecoveryNoLoss) {
  const int num_important_packets = 0;
  const bool use_unequal_protection =  false;
  const int num_media_packets = 4;
  uint8_t protection_factor = 60;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(1, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryWithLoss) {
  const int num_important_packets = 0;
  const bool use_unequal_protection = false;
  const int num_media_packets = 4;
  uint8_t protection_factor = 60;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(1, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[1] = 1;
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryWithLoss50perc) {
  const int num_important_packets = 0;
  const bool use_unequal_protection =  false;
  const int num_media_packets = 4;
  const uint8_t protection_factor = 255;

  
  

  
  
  
  
  
  

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(4, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  fec_loss_mask_[2] = 1;
  media_loss_mask_[0] = 1;
  media_loss_mask_[2] = 1;
  media_loss_mask_[3] = 1;

  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 1, sizeof(fec_loss_mask_));
  media_loss_mask_[0] = 1;
  media_loss_mask_[1] = 1;
  media_loss_mask_[2] = 1;
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryNoLossUep) {
  const int num_important_packets = 2;
  const bool use_unequal_protection =  true;
  const int num_media_packets = 4;
  const uint8_t protection_factor = 60;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(1, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryWithLossUep) {
  const int num_important_packets = 2;
  const bool use_unequal_protection =  true;
  const int num_media_packets = 4;
  const uint8_t protection_factor = 60;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(1, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[1] = 1;
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryWithLoss50percUep) {
  const int num_important_packets = 1;
  const bool use_unequal_protection =  true;
  const int num_media_packets = 4;
  const uint8_t protection_factor = 255;

  
  

  
  
  
  
  
  

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  EXPECT_EQ(0, fec_->GenerateFEC(media_packet_list_,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(4, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  fec_loss_mask_[1] = 1;
  media_loss_mask_[0] = 1;
  media_loss_mask_[2] = 1;
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  fec_loss_mask_[2] = 1;
  media_loss_mask_[0] = 1;
  media_loss_mask_[2] = 1;
  media_loss_mask_[3] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryNonConsecutivePackets) {
  const int num_important_packets = 0;
  const bool use_unequal_protection = false;
  const int num_media_packets = 5;
  uint8_t protection_factor = 60;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  
  
  PacketList protected_media_packets;
  int i = 0;
  for (PacketList::iterator it = media_packet_list_.begin();
      it != media_packet_list_.end(); ++it, ++i) {
    if (i % 2 == 0)
      protected_media_packets.push_back(*it);
  }

  EXPECT_EQ(0, fec_->GenerateFEC(protected_media_packets,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(1, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[2] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[1] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[0] = 1;
  media_loss_mask_[2] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryNonConsecutivePacketsExtension) {
  const int num_important_packets = 0;
  const bool use_unequal_protection = false;
  const int num_media_packets = 21;
  uint8_t protection_factor = 127;

  fec_seq_num_ = ConstructMediaPackets(num_media_packets);

  
  
  PacketList protected_media_packets;
  int i = 0;
  for (PacketList::iterator it = media_packet_list_.begin();
      it != media_packet_list_.end(); ++it, ++i) {
    if (i % 2 == 0)
      protected_media_packets.push_back(*it);
  }

  
  
  
  EXPECT_EQ(0, fec_->GenerateFEC(protected_media_packets,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(5, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 1] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 2] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 11] = 1;
  media_loss_mask_[num_media_packets - 9] = 1;
  media_loss_mask_[num_media_packets - 7] = 1;
  media_loss_mask_[num_media_packets - 5] = 1;
  media_loss_mask_[num_media_packets - 3] = 1;
  media_loss_mask_[num_media_packets - 1] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}

TEST_F(RtpFecTest, FecRecoveryNonConsecutivePacketsWrap) {
  const int num_important_packets = 0;
  const bool use_unequal_protection = false;
  const int num_media_packets = 21;
  uint8_t protection_factor = 127;

  fec_seq_num_ = ConstructMediaPacketsSeqNum(num_media_packets, 0xFFFF - 5);

  
  
  PacketList protected_media_packets;
  int i = 0;
  for (PacketList::iterator it = media_packet_list_.begin();
      it != media_packet_list_.end(); ++it, ++i) {
    if (i % 2 == 0)
      protected_media_packets.push_back(*it);
  }

  
  
  
  EXPECT_EQ(0, fec_->GenerateFEC(protected_media_packets,
                                 protection_factor,
                                 num_important_packets,
                                 use_unequal_protection,
                                 &fec_packet_list_));

  
  EXPECT_EQ(5, static_cast<int>(fec_packet_list_.size()));

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 1] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_TRUE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 2] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
  FreeRecoveredPacketList();

  
  memset(media_loss_mask_, 0, sizeof(media_loss_mask_));
  memset(fec_loss_mask_, 0, sizeof(fec_loss_mask_));
  media_loss_mask_[num_media_packets - 11] = 1;
  media_loss_mask_[num_media_packets - 9] = 1;
  media_loss_mask_[num_media_packets - 7] = 1;
  media_loss_mask_[num_media_packets - 5] = 1;
  media_loss_mask_[num_media_packets - 3] = 1;
  media_loss_mask_[num_media_packets - 1] = 1;
  NetworkReceivedPackets();

  EXPECT_EQ(0, fec_->DecodeFEC(&received_packet_list_ ,
                               &recovered_packet_list_));

  
  EXPECT_FALSE(IsRecoveryComplete());
}



void RtpFecTest::TearDown() {
  fec_->ResetState(&recovered_packet_list_);
  delete fec_;
  FreeRecoveredPacketList();
  ClearList(&media_packet_list_);
  EXPECT_TRUE(media_packet_list_.empty());
}

void RtpFecTest::FreeRecoveredPacketList() {
  ClearList(&recovered_packet_list_);
}

bool RtpFecTest::IsRecoveryComplete() {
  
  if (media_packet_list_.size() != recovered_packet_list_.size()) {
    return false;
  }

  ForwardErrorCorrection::Packet* media_packet;
  ForwardErrorCorrection::RecoveredPacket* recovered_packet;

  bool recovery = true;

  PacketList::iterator
    media_packet_list_item = media_packet_list_.begin();
  RecoveredPacketList::iterator
    recovered_packet_list_item = recovered_packet_list_.begin();
  while (media_packet_list_item != media_packet_list_.end()) {
    if (recovered_packet_list_item == recovered_packet_list_.end()) {
      return false;
    }
    media_packet = *media_packet_list_item;
    recovered_packet = *recovered_packet_list_item;
    if (recovered_packet->pkt->length != media_packet->length) {
      return false;
    }
    if (memcmp(recovered_packet->pkt->data, media_packet->data,
               media_packet->length) != 0) {
      return false;
    }
    media_packet_list_item++;
    recovered_packet_list_item++;
  }
  return recovery;
}

void RtpFecTest::NetworkReceivedPackets() {
  const bool kFecPacket = true;
  ReceivedPackets(media_packet_list_, media_loss_mask_, !kFecPacket);
  ReceivedPackets(fec_packet_list_, fec_loss_mask_, kFecPacket);
}

void RtpFecTest:: ReceivedPackets(
    const PacketList& packet_list,
    int* loss_mask,
    bool is_fec) {
  ForwardErrorCorrection::Packet* packet;
  ForwardErrorCorrection::ReceivedPacket* received_packet;
  int seq_num = fec_seq_num_;
  int packet_idx = 0;

  PacketList::const_iterator
  packet_list_item = packet_list.begin();

  while (packet_list_item != packet_list.end()) {
    packet = *packet_list_item;
    if (loss_mask[packet_idx] == 0) {
      received_packet = new ForwardErrorCorrection::ReceivedPacket;
      received_packet->pkt = new ForwardErrorCorrection::Packet;
      received_packet_list_.push_back(received_packet);
      received_packet->pkt->length = packet->length;
      memcpy(received_packet->pkt->data, packet->data,
             packet->length);
      received_packet->isFec = is_fec;
      if (!is_fec) {
        
        
        received_packet->seqNum =
            webrtc::ModuleRTPUtility::BufferToUWord16(&packet->data[2]);
      }
      else {
        
        
        
        
        received_packet->seqNum = seq_num;
        
        
        received_packet->ssrc = ssrc_;
      }
    }
    packet_idx++;
    packet_list_item ++;
    
    
    if (is_fec) seq_num++;
  }
}

int RtpFecTest::ConstructMediaPacketsSeqNum(int num_media_packets,
                                            int start_seq_num) {
  assert(num_media_packets > 0);
  ForwardErrorCorrection::Packet* media_packet = NULL;
  int sequence_number = start_seq_num;
  int time_stamp = rand();

  for (int i = 0; i < num_media_packets; i++) {
    media_packet = new ForwardErrorCorrection::Packet;
    media_packet_list_.push_back(media_packet);
    media_packet->length =
        static_cast<uint16_t>((static_cast<float>(rand()) / RAND_MAX) *
        (IP_PACKET_SIZE - kRtpHeaderSize - kTransportOverhead -
            ForwardErrorCorrection::PacketOverhead()));

    if (media_packet->length < kRtpHeaderSize) {
      media_packet->length = kRtpHeaderSize;
    }
    
    media_packet->data[0] = static_cast<uint8_t>(rand() % 256);
    media_packet->data[1] = static_cast<uint8_t>(rand() % 256);

    
    
    
    
    media_packet->data[0] |= 0x80;
    media_packet->data[0] &= 0xbf;

    
    
    
    
    media_packet->data[1] &= 0x7f;

    webrtc::ModuleRTPUtility::AssignUWord16ToBuffer(&media_packet->data[2],
                                                    sequence_number);
    webrtc::ModuleRTPUtility::AssignUWord32ToBuffer(&media_packet->data[4],
                                                    time_stamp);
    webrtc::ModuleRTPUtility::AssignUWord32ToBuffer(&media_packet->data[8],
                                                    ssrc_);

    
    for (int j = 12; j < media_packet->length; j++) {
      media_packet->data[j] = static_cast<uint8_t> (rand() % 256);
    }
    sequence_number++;
  }
  
  assert(media_packet != NULL);
  media_packet->data[1] |= 0x80;
  return sequence_number;
}

int RtpFecTest::ConstructMediaPackets(int num_media_packets) {
  return ConstructMediaPacketsSeqNum(num_media_packets, rand());
}
