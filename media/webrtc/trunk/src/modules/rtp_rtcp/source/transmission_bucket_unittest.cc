













#include <gtest/gtest.h>

#include "transmission_bucket.h"

namespace webrtc {

class TransmissionBucketTest : public ::testing::Test {
 protected:  
  TransmissionBucket send_bucket_;
};

TEST_F(TransmissionBucketTest, Fill) {
  EXPECT_TRUE(send_bucket_.Empty());
  send_bucket_.Fill(1, 100);
  EXPECT_FALSE(send_bucket_.Empty());
}

TEST_F(TransmissionBucketTest, Reset) {
  send_bucket_.Fill(1, 100);
  EXPECT_FALSE(send_bucket_.Empty());
  send_bucket_.Reset();
  EXPECT_TRUE(send_bucket_.Empty());
}

TEST_F(TransmissionBucketTest, GetNextPacket) {
  EXPECT_EQ(-1, send_bucket_.GetNextPacket());    
  send_bucket_.Fill(1234, 100);
  EXPECT_EQ(1234, send_bucket_.GetNextPacket());  
  send_bucket_.Fill(1235, 100);
  EXPECT_EQ(-1, send_bucket_.GetNextPacket());    
}

TEST_F(TransmissionBucketTest, UpdateBytesPerInterval) {
  const int delta_time_ms = 1;
  const int target_bitrate_kbps = 800;
  send_bucket_.UpdateBytesPerInterval(delta_time_ms, target_bitrate_kbps);

  send_bucket_.Fill(1234, 50);
  send_bucket_.Fill(1235, 50);
  send_bucket_.Fill(1236, 50);

  EXPECT_EQ(1234, send_bucket_.GetNextPacket());  
  EXPECT_EQ(1235, send_bucket_.GetNextPacket());  
  EXPECT_EQ(1236, send_bucket_.GetNextPacket());  
  EXPECT_TRUE(send_bucket_.Empty());

  send_bucket_.Fill(1237, 50);
  EXPECT_EQ(-1, send_bucket_.GetNextPacket());    
}
}  
