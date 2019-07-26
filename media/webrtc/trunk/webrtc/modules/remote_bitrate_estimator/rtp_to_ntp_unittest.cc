









#include "gtest/gtest.h"
#include "modules/remote_bitrate_estimator/include/rtp_to_ntp.h"

namespace webrtc {

TEST(WrapAroundTests, NoWrap) {
  EXPECT_EQ(0, synchronization::CheckForWrapArounds(0xFFFFFFFF, 0xFFFFFFFE));
  EXPECT_EQ(0, synchronization::CheckForWrapArounds(1, 0));
  EXPECT_EQ(0, synchronization::CheckForWrapArounds(0x00010000, 0x0000FFFF));
}

TEST(WrapAroundTests, ForwardWrap) {
  EXPECT_EQ(1, synchronization::CheckForWrapArounds(0, 0xFFFFFFFF));
  EXPECT_EQ(1, synchronization::CheckForWrapArounds(0, 0xFFFF0000));
  EXPECT_EQ(1, synchronization::CheckForWrapArounds(0x0000FFFF, 0xFFFFFFFF));
  EXPECT_EQ(1, synchronization::CheckForWrapArounds(0x0000FFFF, 0xFFFF0000));
}

TEST(WrapAroundTests, BackwardWrap) {
  EXPECT_EQ(-1, synchronization::CheckForWrapArounds(0xFFFFFFFF, 0));
  EXPECT_EQ(-1, synchronization::CheckForWrapArounds(0xFFFF0000, 0));
  EXPECT_EQ(-1, synchronization::CheckForWrapArounds(0xFFFFFFFF, 0x0000FFFF));
  EXPECT_EQ(-1, synchronization::CheckForWrapArounds(0xFFFF0000, 0x0000FFFF));
}

TEST(WrapAroundTests, OldRtcpWrapped) {
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0;
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp -= kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp -= kTimestampTicksPerMs;
  int64_t timestamp_in_ms = -1;
  
  
  EXPECT_FALSE(synchronization::RtpToNtpMs(timestamp, rtcp, &timestamp_in_ms));
}

TEST(WrapAroundTests, NewRtcpWrapped) {
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0xFFFFFFFF;
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                         timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                         timestamp));
  int64_t timestamp_in_ms = -1;
  EXPECT_TRUE(synchronization::RtpToNtpMs(rtcp.back().rtp_timestamp, rtcp,
                                          &timestamp_in_ms));
  
  
  EXPECT_EQ(0, timestamp_in_ms);
}

TEST(WrapAroundTests, RtpWrapped) {
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0xFFFFFFFF - 2 * kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += kTimestampTicksPerMs;
  int64_t timestamp_in_ms = -1;
  EXPECT_TRUE(synchronization::RtpToNtpMs(timestamp, rtcp,
                                          &timestamp_in_ms));
  
  
  EXPECT_EQ(2, timestamp_in_ms);
}

TEST(WrapAroundTests, OldRtp_RtcpsWrapped) {
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp -= 2*kTimestampTicksPerMs;
  int64_t timestamp_in_ms = -1;
  EXPECT_FALSE(synchronization::RtpToNtpMs(timestamp, rtcp,
                                           &timestamp_in_ms));
}

TEST(WrapAroundTests, OldRtp_NewRtcpWrapped) {
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0xFFFFFFFF;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp -= kTimestampTicksPerMs;
  int64_t timestamp_in_ms = -1;
  EXPECT_TRUE(synchronization::RtpToNtpMs(timestamp, rtcp,
                                          &timestamp_in_ms));
  
  
  EXPECT_EQ(0, timestamp_in_ms);
}

TEST(WrapAroundTests, OldRtp_OldRtcpWrapped) {
  const uint32_t kOneMsInNtpFrac = 4294967;
  const uint32_t kTimestampTicksPerMs = 90;
  synchronization::RtcpList rtcp;
  uint32_t ntp_sec = 0;
  uint32_t ntp_frac = 0;
  uint32_t timestamp = 0;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp -= kTimestampTicksPerMs;
  rtcp.push_front(synchronization::RtcpMeasurement(ntp_sec, ntp_frac,
                                                   timestamp));
  ntp_frac += kOneMsInNtpFrac;
  timestamp += 2*kTimestampTicksPerMs;
  int64_t timestamp_in_ms = -1;
  EXPECT_FALSE(synchronization::RtpToNtpMs(timestamp, rtcp,
                                           &timestamp_in_ms));
}
};  
