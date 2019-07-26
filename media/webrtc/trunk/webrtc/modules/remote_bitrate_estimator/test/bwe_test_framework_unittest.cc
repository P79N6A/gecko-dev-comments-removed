









#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_framework.h"

#include <numeric>

#include "gtest/gtest.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/test/testsupport/fileutils.h"

using std::vector;

namespace webrtc {
namespace testing {
namespace bwe {

TEST(BweTestFramework_RandomTest, Gaussian) {
  enum {
    kN = 100000,
    kBuckets = 100,
    kMean = 49,
    kStddev = 10
  };

  Random random(0x12345678);

  int buckets[kBuckets] = {0};
  for (int i = 0; i < kN; ++i) {
    int index = random.Gaussian(kMean, kStddev);
    if (index >= 0 && index < kBuckets) {
      buckets[index]++;
    }
  }

  const double kPi = 3.14159265358979323846;
  const double kScale = kN / (kStddev * std::sqrt(2.0 * kPi));
  const double kDiv = -2.0 * kStddev * kStddev;
  double self_corr = 0.0;
  double bucket_corr = 0.0;
  for (int n = 0; n < kBuckets; ++n) {
    double normal_dist = kScale * std::exp((n - kMean) * (n - kMean) / kDiv);
    self_corr += normal_dist * normal_dist;
    bucket_corr += normal_dist * buckets[n];
  }
  printf("Correlation: %f (random sample), %f (self), %f (quotient)\n",
         bucket_corr, self_corr, bucket_corr / self_corr);
  EXPECT_NEAR(1.0, bucket_corr / self_corr, 0.0004);
}

static bool IsSequenceNumberSorted(const Packets& packets) {
  PacketsConstIt last_it = packets.begin();
  for (PacketsConstIt it = last_it; it != packets.end(); ++it) {
    if (IsNewerSequenceNumber(last_it->header().sequenceNumber,
                              it->header().sequenceNumber)) {
      return false;
    }
    last_it = it;
  }
  return true;
}

TEST(BweTestFramework_PacketTest, IsTimeSorted) {
  Packets packets;
  
  EXPECT_TRUE(IsTimeSorted(packets));

  packets.push_back(Packet(100, 0));
  EXPECT_TRUE(IsTimeSorted(packets));

  packets.push_back(Packet(110, 0));
  EXPECT_TRUE(IsTimeSorted(packets));

  
  packets.push_back(Packet(100, 0));
  EXPECT_FALSE(IsTimeSorted(packets));

  
  packets.pop_back();
  packets.push_back(Packet(120, 0));
  EXPECT_TRUE(IsTimeSorted(packets));
}

TEST(BweTestFramework_PacketTest, IsSequenceNumberSorted) {
  Packets packets;
  
  EXPECT_TRUE(IsSequenceNumberSorted(packets));

  packets.push_back(Packet(0, 100));
  EXPECT_TRUE(IsSequenceNumberSorted(packets));

  packets.push_back(Packet(0, 110));
  EXPECT_TRUE(IsSequenceNumberSorted(packets));

  
  packets.push_back(Packet(0, 100));
  EXPECT_FALSE(IsSequenceNumberSorted(packets));

  
  packets.pop_back();
  packets.push_back(Packet(0, 120));
  EXPECT_TRUE(IsSequenceNumberSorted(packets));
}

TEST(BweTestFramework_StatsTest, Mean) {
  Stats<int32_t> stats;
  EXPECT_EQ(0, stats.GetMean());

  stats.Push(1);
  stats.Push(3);
  EXPECT_EQ(2, stats.GetMean());

  
  stats.Push(-3);
  EXPECT_EQ(0, stats.GetMean());
}

TEST(BweTestFramework_StatsTest, Variance) {
  Stats<int32_t> stats;
  EXPECT_EQ(0, stats.GetVariance());

  
  stats.Push(1);
  stats.Push(3);
  EXPECT_EQ(1, stats.GetVariance());

  
  
  stats.Push(-4);
  EXPECT_EQ(8, stats.GetVariance());
}

TEST(BweTestFramework_StatsTest, StdDev) {
  Stats<int32_t> stats;
  EXPECT_EQ(0, stats.GetStdDev());

  
  stats.Push(1);
  stats.Push(3);
  EXPECT_EQ(1, stats.GetStdDev());

  
  stats.Push(-4);
  EXPECT_EQ(2, stats.GetStdDev());
}

TEST(BweTestFramework_StatsTest, MinMax) {
  Stats<int32_t> stats;
  EXPECT_EQ(0, stats.GetMin());
  EXPECT_EQ(0, stats.GetMax());

  stats.Push(1);
  EXPECT_EQ(1, stats.GetMin());
  EXPECT_EQ(1, stats.GetMax());

  stats.Push(3);
  EXPECT_EQ(1, stats.GetMin());
  EXPECT_EQ(3, stats.GetMax());

  stats.Push(-4);
  EXPECT_EQ(-4, stats.GetMin());
  EXPECT_EQ(3, stats.GetMax());
}

class BweTestFramework_RateCounterFilterTest : public ::testing::Test {
 public:
  BweTestFramework_RateCounterFilterTest()
    : filter_(NULL),
      now_ms_(0) {
  }
  virtual ~BweTestFramework_RateCounterFilterTest() {}

 protected:
  void TestRateCounter(int64_t run_for_ms, uint32_t payload_bits,
                       uint32_t expected_pps, uint32_t expected_bps) {
    Packets packets;
    RTPHeader header = {0};
    
    for (int64_t i = 0; i < run_for_ms; i += 10, now_ms_ += 10) {
      packets.push_back(Packet(now_ms_ * 1000, payload_bits / 8, header));
    }
    filter_.RunFor(run_for_ms, &packets);
    ASSERT_TRUE(IsTimeSorted(packets));
    EXPECT_EQ(expected_pps, filter_.packets_per_second());
    EXPECT_EQ(expected_bps, filter_.bits_per_second());
  }

 private:
  RateCounterFilter filter_;
  int64_t now_ms_;

  DISALLOW_COPY_AND_ASSIGN(BweTestFramework_RateCounterFilterTest);
};

TEST_F(BweTestFramework_RateCounterFilterTest, Short) {
  
  
  TestRateCounter(100, 800, 10, 8000);
}

TEST_F(BweTestFramework_RateCounterFilterTest, Medium) {
  
  TestRateCounter(100, 800, 10, 8000);
  
  
  TestRateCounter(900, 800, 100, 80000);
}

TEST_F(BweTestFramework_RateCounterFilterTest, Long) {
  
  TestRateCounter(100, 800, 10, 8000);
  TestRateCounter(900, 800, 100, 80000);
  
  TestRateCounter(1000, 400, 100, 40000);
  
  
  TestRateCounter(500, 0, 100, 20000);
  
  
  TestRateCounter(500, 0, 100, 0);
  
  TestRateCounter(500, 200, 100, 10000);
}

static void TestLossFilter(float loss_percent, bool zero_tolerance) {
  LossFilter filter(NULL);
  filter.SetLoss(loss_percent);
  Packets::size_type sent_packets = 0;
  Packets::size_type remaining_packets = 0;

  
  {
    Packets packets;
    sent_packets += packets.size();
    filter.RunFor(0, &packets);
    ASSERT_TRUE(IsTimeSorted(packets));
    ASSERT_TRUE(IsSequenceNumberSorted(packets));
    remaining_packets += packets.size();
    EXPECT_EQ(0u, sent_packets);
    EXPECT_EQ(0u, remaining_packets);
  }

  
  for (int i = 0; i < 2225; ++i) {
    Packets packets;
    packets.insert(packets.end(), i % 10, Packet());
    sent_packets += packets.size();
    filter.RunFor(0, &packets);
    ASSERT_TRUE(IsTimeSorted(packets));
    ASSERT_TRUE(IsSequenceNumberSorted(packets));
    remaining_packets += packets.size();
  }

  float loss_fraction = 0.01f * (100.0f - loss_percent);
  Packets::size_type expected_packets = loss_fraction * sent_packets;
  if (zero_tolerance) {
    EXPECT_EQ(expected_packets, remaining_packets);
  } else {
    
    EXPECT_NEAR(expected_packets, remaining_packets, 100);
  }
}

TEST(BweTestFramework_LossFilterTest, Loss0) {
  
  TestLossFilter(0.0f, true);
}

TEST(BweTestFramework_LossFilterTest, Loss10) {
  TestLossFilter(10.0f, false);
}

TEST(BweTestFramework_LossFilterTest, Loss50) {
  TestLossFilter(50.0f, false);
}

TEST(BweTestFramework_LossFilterTest, Loss100) {
  
  TestLossFilter(100.0f, true);
}

class BweTestFramework_DelayFilterTest : public ::testing::Test {
 public:
  BweTestFramework_DelayFilterTest()
    : filter_(NULL),
      now_ms_(0),
      sequence_number_(0) {
  }
  virtual ~BweTestFramework_DelayFilterTest() {}

 protected:
  void TestDelayFilter(int64_t run_for_ms, uint32_t in_packets,
                       uint32_t out_packets) {
    Packets packets;
    for (uint32_t i = 0; i < in_packets; ++i) {
      packets.push_back(Packet(now_ms_ * 1000 + (sequence_number_ >> 4),
                                  sequence_number_));
      sequence_number_++;
    }
    filter_.RunFor(run_for_ms, &packets);
    ASSERT_TRUE(IsTimeSorted(packets));
    ASSERT_TRUE(IsSequenceNumberSorted(packets));
    for (PacketsConstIt it = packets.begin(); it != packets.end(); ++it) {
      EXPECT_LE(now_ms_ * 1000, it->send_time_us());
    }
    EXPECT_EQ(out_packets, packets.size());
    accumulated_packets_.splice(accumulated_packets_.end(), packets);
    now_ms_ += run_for_ms;
  }

  void TestDelayFilter(int64_t delay_ms) {
    filter_.SetDelay(delay_ms);
    TestDelayFilter(1, 0, 0);    

    
    TestDelayFilter(0, 1, 1);
    TestDelayFilter(delay_ms, 0, 0);

    for (int i = 0; i < delay_ms; ++i) {
      filter_.SetDelay(i);
      TestDelayFilter(1, 10, 10);
    }
    TestDelayFilter(0, 0, 0);
    TestDelayFilter(delay_ms, 0, 0);

    
    TestDelayFilter(delay_ms, 0, 0);

    for (int i = 1; i < delay_ms + 1; ++i) {
      filter_.SetDelay(i);
      TestDelayFilter(1, 5, 5);
    }
    TestDelayFilter(0, 0, 0);
    filter_.SetDelay(2 * delay_ms);
    TestDelayFilter(1, 0, 0);
    TestDelayFilter(delay_ms, 13, 13);
    TestDelayFilter(delay_ms, 0, 0);

    
    TestDelayFilter(delay_ms, 0, 0);

    for (int i = 0; i < 2 * delay_ms; ++i) {
      filter_.SetDelay(2 * delay_ms - i - 1);
      TestDelayFilter(1, 5, 5);
    }
    TestDelayFilter(0, 0, 0);
    filter_.SetDelay(0);
    TestDelayFilter(0, 7, 7);

    ASSERT_TRUE(IsTimeSorted(accumulated_packets_));
    ASSERT_TRUE(IsSequenceNumberSorted(accumulated_packets_));
  }

  DelayFilter filter_;
  Packets accumulated_packets_;

 private:
  int64_t now_ms_;
  uint32_t sequence_number_;

  DISALLOW_COPY_AND_ASSIGN(BweTestFramework_DelayFilterTest);
};

TEST_F(BweTestFramework_DelayFilterTest, Delay0) {
  TestDelayFilter(1, 0, 0);    
  TestDelayFilter(1, 10, 10);  
  TestDelayFilter(1, 0, 0);    
  filter_.SetDelay(0);
  TestDelayFilter(1, 5, 5);    
  TestDelayFilter(1, 0, 0);    
}

TEST_F(BweTestFramework_DelayFilterTest, Delay1) {
  TestDelayFilter(1);
}

TEST_F(BweTestFramework_DelayFilterTest, Delay2) {
  TestDelayFilter(2);
}

TEST_F(BweTestFramework_DelayFilterTest, Delay20) {
  TestDelayFilter(20);
}

TEST_F(BweTestFramework_DelayFilterTest, Delay100) {
  TestDelayFilter(100);
}

TEST_F(BweTestFramework_DelayFilterTest, JumpToZeroDelay) {
  DelayFilter delay(NULL);
  Packets acc;
  Packets packets;

  
  delay.SetDelay(100.0f);
  for (uint32_t i = 0; i < 10; ++i) {
    packets.push_back(Packet(i * 100, i));
  }
  delay.RunFor(1000, &packets);
  acc.splice(acc.end(), packets);
  ASSERT_TRUE(IsTimeSorted(acc));
  ASSERT_TRUE(IsSequenceNumberSorted(acc));

  
  
  delay.SetDelay(0.0f);
  for (uint32_t i = 10; i < 50; ++i) {
    packets.push_back(Packet(i * 100, i));
  }
  delay.RunFor(1000, &packets);
  acc.splice(acc.end(), packets);
  ASSERT_TRUE(IsTimeSorted(acc));
  ASSERT_TRUE(IsSequenceNumberSorted(acc));
}

TEST_F(BweTestFramework_DelayFilterTest, IncreasingDelay) {
  
  for (int i = 1; i < 50; i += 4) {
    TestDelayFilter(i);
  }
  
  filter_.SetDelay(100);
  TestDelayFilter(1, 20, 20);
  TestDelayFilter(2, 0, 0);
  TestDelayFilter(99, 20, 20);
  
  filter_.SetDelay(0);
  TestDelayFilter(1, 100, 100);
  TestDelayFilter(23010, 0, 0);
  ASSERT_TRUE(IsTimeSorted(accumulated_packets_));
  ASSERT_TRUE(IsSequenceNumberSorted(accumulated_packets_));
}

static void TestJitterFilter(int64_t stddev_jitter_ms) {
  JitterFilter filter(NULL);
  filter.SetJitter(stddev_jitter_ms);

  int64_t now_ms = 0;
  uint32_t sequence_number = 0;

  
  Packets original;
  Packets jittered;
  for (uint32_t i = 0; i < 1000; ++i) {
    Packets packets;
    for (uint32_t j = 0; j < i % 100; ++j) {
      packets.push_back(Packet(now_ms * 1000, sequence_number++));
      now_ms += 5 * stddev_jitter_ms;
    }
    original.insert(original.end(), packets.begin(), packets.end());
    filter.RunFor(stddev_jitter_ms, &packets);
    jittered.splice(jittered.end(), packets);
  }

  
  ASSERT_TRUE(IsTimeSorted(original));
  ASSERT_TRUE(IsTimeSorted(jittered));
  ASSERT_TRUE(IsSequenceNumberSorted(original));
  ASSERT_TRUE(IsSequenceNumberSorted(jittered));
  EXPECT_EQ(original.size(), jittered.size());

  
  
  
  Stats<double> jitter_us;
  for (PacketsIt it1 = original.begin(), it2 = jittered.begin();
       it1 != original.end() && it2 != jittered.end(); ++it1, ++it2) {
    EXPECT_EQ(it1->header().sequenceNumber, it2->header().sequenceNumber);
    jitter_us.Push(it2->send_time_us() - it1->send_time_us());
  }
  EXPECT_NEAR(0.0, jitter_us.GetMean(), stddev_jitter_ms * 1000.0 * 0.008);
  EXPECT_NEAR(stddev_jitter_ms * 1000.0, jitter_us.GetStdDev(),
              stddev_jitter_ms * 1000.0 * 0.02);
}

TEST(BweTestFramework_JitterFilterTest, Jitter0) {
  TestJitterFilter(0);
}

TEST(BweTestFramework_JitterFilterTest, Jitter1) {
  TestJitterFilter(1);
}

TEST(BweTestFramework_JitterFilterTest, Jitter5) {
  TestJitterFilter(5);
}

TEST(BweTestFramework_JitterFilterTest, Jitter10) {
  TestJitterFilter(10);
}

TEST(BweTestFramework_JitterFilterTest, Jitter1031) {
  TestJitterFilter(1031);
}

static void TestReorderFilter(uint32_t reorder_percent, uint32_t near) {
  const uint32_t kPacketCount = 10000;

  
  Packets packets;
  int64_t now_ms = 0;
  uint32_t sequence_number = 1;
  for (uint32_t i = 0; i < kPacketCount; ++i, now_ms += 10) {
    packets.push_back(Packet(now_ms * 1000, sequence_number++));
  }
  ASSERT_TRUE(IsTimeSorted(packets));
  ASSERT_TRUE(IsSequenceNumberSorted(packets));

  
  ReorderFilter filter(NULL);
  filter.SetReorder(reorder_percent);
  filter.RunFor(now_ms, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));

  
  
  uint32_t distance = 0;
  uint32_t last_sequence_number = 0;
  for (PacketsIt it = packets.begin(); it != packets.end(); ++it) {
    uint32_t sequence_number = it->header().sequenceNumber;
    if (sequence_number < last_sequence_number) {
      distance += last_sequence_number - sequence_number;
    }
    last_sequence_number = sequence_number;
  }

  
  
  EXPECT_NEAR(((kPacketCount - 1) * reorder_percent) / 100, distance, near);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder0) {
  
  TestReorderFilter(0, 0);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder10) {
  TestReorderFilter(10, 30);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder20) {
  TestReorderFilter(20, 20);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder50) {
  TestReorderFilter(50, 20);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder70) {
  TestReorderFilter(70, 20);
}

TEST(BweTestFramework_ReorderFilterTest, Reorder100) {
  
  
  
  
  TestReorderFilter(100.0, 0);
}

class BweTestFramework_ChokeFilterTest : public ::testing::Test {
 public:
  BweTestFramework_ChokeFilterTest()
    : now_ms_(0),
      sequence_number_(0),
      output_packets_(),
      send_times_us_() {
  }
  virtual ~BweTestFramework_ChokeFilterTest() {}

 protected:
  void TestChoke(PacketProcessor* filter,
                 int64_t run_for_ms,
                 uint32_t packets_to_generate,
                 uint32_t expected_kbit_transmitted) {
    
    Packets packets;
    RTPHeader header = {0};
    for (uint32_t i = 0; i < packets_to_generate; ++i) {
      int64_t send_time_ms = now_ms_ + (i * run_for_ms) / packets_to_generate;
      header.sequenceNumber = sequence_number_++;
      
      packets.push_back(Packet(send_time_ms * 1000, 125, header));
      send_times_us_.push_back(send_time_ms * 1000);
    }
    ASSERT_TRUE(IsTimeSorted(packets));
    filter->RunFor(run_for_ms, &packets);
    now_ms_ += run_for_ms;
    output_packets_.splice(output_packets_.end(), packets);
    ASSERT_TRUE(IsTimeSorted(output_packets_));
    ASSERT_TRUE(IsSequenceNumberSorted(output_packets_));

    
    uint32_t bytes_transmitted = 0;
    while (!output_packets_.empty()) {
      const Packet& packet = output_packets_.front();
      if (packet.send_time_us() > now_ms_ * 1000) {
        break;
      }
      bytes_transmitted += packet.payload_size();
      output_packets_.pop_front();
    }
    EXPECT_EQ(expected_kbit_transmitted, (bytes_transmitted * 8) / 1000);
  }

  void CheckMaxDelay(int64_t max_delay_ms) {
    for (PacketsIt it = output_packets_.begin(); it != output_packets_.end();
        ++it) {
      const Packet& packet = *it;
      int64_t delay_us = packet.send_time_us() -
          send_times_us_[packet.header().sequenceNumber];
      EXPECT_GE(max_delay_ms * 1000, delay_us);
    }
  }

 private:
  int64_t now_ms_;
  uint32_t sequence_number_;
  Packets output_packets_;
  std::vector<int64_t> send_times_us_;

  DISALLOW_COPY_AND_ASSIGN(BweTestFramework_ChokeFilterTest);
};

TEST_F(BweTestFramework_ChokeFilterTest, Short) {
  
  
  
  ChokeFilter filter(NULL);
  filter.SetCapacity(10);
  TestChoke(&filter, 100, 100, 1);
}

TEST_F(BweTestFramework_ChokeFilterTest, Medium) {
  
  ChokeFilter filter(NULL);
  filter.SetCapacity(10);
  TestChoke(&filter, 100, 10, 1);
  
  TestChoke(&filter, 100, 0, 1);
  
  TestChoke(&filter, 800, 0, 8);
  
  TestChoke(&filter, 1000, 0, 0);
}

TEST_F(BweTestFramework_ChokeFilterTest, Long) {
  
  ChokeFilter filter(NULL);
  filter.SetCapacity(10);
  TestChoke(&filter, 100, 100, 1);
  
  TestChoke(&filter, 100, 0, 1);
  
  TestChoke(&filter, 800, 0, 8);
  
  
  filter.SetCapacity(100);
  TestChoke(&filter, 9000, 0, 90);
  
  TestChoke(&filter, 100, 20, 10);
  
  TestChoke(&filter, 200, 10, 20);
  
  filter.SetCapacity(10);
  TestChoke(&filter, 1000, 0, 0);
}

TEST_F(BweTestFramework_ChokeFilterTest, MaxDelay) {
  
  ChokeFilter filter(NULL);
  filter.SetCapacity(10);
  filter.SetMaxDelay(500);
  
  TestChoke(&filter, 100, 100, 1);
  CheckMaxDelay(500);
  
  TestChoke(&filter, 400, 0, 4);
  
  TestChoke(&filter, 9500, 0, 0);

  
  filter.SetMaxDelay(100);
  
  TestChoke(&filter, 100, 50, 2);
  CheckMaxDelay(100);
  
  TestChoke(&filter, 9900, 0, 0);

  
  filter.SetCapacity(10);
  filter.SetMaxDelay(0);
  TestChoke(&filter, 100, 100, 2);
  TestChoke(&filter, 9900, 0, 98);
}

TEST_F(BweTestFramework_ChokeFilterTest, ShortTrace) {
  
  
  TraceBasedDeliveryFilter filter(NULL);
  ASSERT_TRUE(filter.Init(test::ResourcePath("synthetic-trace", "rx")));
  TestChoke(&filter, 100, 100, 6);
}

TEST_F(BweTestFramework_ChokeFilterTest, ShortTraceWrap) {
  
  
  TraceBasedDeliveryFilter filter(NULL);
  ASSERT_TRUE(filter.Init(test::ResourcePath("synthetic-trace", "rx")));
  TestChoke(&filter, 140, 100, 10);
}

void TestVideoSender(VideoSender* sender, int64_t run_for_ms,
                     uint32_t expected_packets,
                     uint32_t expected_payload_size,
                     uint32_t expected_total_payload_size) {
  assert(sender);
  Packets packets;
  sender->RunFor(run_for_ms, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));
  ASSERT_TRUE(IsSequenceNumberSorted(packets));
  EXPECT_EQ(expected_packets, packets.size());
  int64_t send_time_us = -1;
  uint32_t total_payload_size = 0;
  uint32_t absolute_send_time = 0;
  uint32_t absolute_send_time_wraps = 0;
  uint32_t rtp_timestamp = 0;
  uint32_t rtp_timestamp_wraps = 0;
  for (PacketsIt it = packets.begin(); it != packets.end(); ++it) {
    EXPECT_LE(send_time_us, it->send_time_us());
    send_time_us = it->send_time_us();
    if (sender->max_payload_size_bytes() != it->payload_size()) {
      EXPECT_EQ(expected_payload_size, it->payload_size());
    }
    total_payload_size += it->payload_size();
    if (absolute_send_time > it->header().extension.absoluteSendTime) {
      absolute_send_time_wraps++;
    }
    absolute_send_time = it->header().extension.absoluteSendTime;
    if (rtp_timestamp > it->header().timestamp) {
      rtp_timestamp_wraps++;
    }
    rtp_timestamp = it->header().timestamp;
  }
  EXPECT_EQ(expected_total_payload_size, total_payload_size);
  EXPECT_GE(1u, absolute_send_time_wraps);
  EXPECT_GE(1u, rtp_timestamp_wraps);
}

TEST(BweTestFramework_VideoSenderTest, Fps1Kpbs80_1s) {
  
  VideoSender sender(NULL, 1.0f, 80, 0x1234, 0);
  EXPECT_EQ(10000u, sender.bytes_per_second());
  
  
  TestVideoSender(&sender, 1, 10, 1000, 10000);
  
  TestVideoSender(&sender, 998, 0, 0, 0);
  
  TestVideoSender(&sender, 1000, 10, 1000, 10000);
  
  TestVideoSender(&sender, 1, 10, 1000, 10000);
  
  TestVideoSender(&sender, 999, 0, 0, 0);
}

TEST(BweTestFramework_VideoSenderTest, Fps1Kpbs80_1s_Offset) {
  
  VideoSender sender(NULL, 1.0f, 80, 0x1234, 0.5f);
  EXPECT_EQ(10000u, sender.bytes_per_second());
  
  TestVideoSender(&sender, 499, 0, 0, 0);
  
  TestVideoSender(&sender, 1, 10, 1000, 10000);
  
  TestVideoSender(&sender, 999, 0, 0, 0);
  
  TestVideoSender(&sender, 500, 10, 1000, 10000);
  
  TestVideoSender(&sender, 500, 0, 0, 0);
  
  TestVideoSender(&sender, 1, 10, 1000, 10000);
  
  TestVideoSender(&sender, 999, 0, 0, 0);
}

TEST(BweTestFramework_VideoSenderTest, Fps50Kpbs80_11s) {
  
  VideoSender sender(NULL, 50.0f, 80, 0x1234, 0);
  EXPECT_EQ(10000u, sender.bytes_per_second());
  
  TestVideoSender(&sender, 9998, 500, 200, 100000);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
  
  TestVideoSender(&sender, 1, 1, 200, 200);
  
  TestVideoSender(&sender, 998, 49, 200, 9800);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
}

TEST(BweTestFramework_VideoSenderTest, Fps10Kpbs120_1s) {
  
  VideoSender sender(NULL, 20.0f, 120, 0x1234, 0);
  EXPECT_EQ(15000u, sender.bytes_per_second());
  
  TestVideoSender(&sender, 498, 10, 750, 7500);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
  
  TestVideoSender(&sender, 1, 1, 750, 750);
  
  TestVideoSender(&sender, 498, 9, 750, 6750);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
}

TEST(BweTestFramework_VideoSenderTest, Fps30Kpbs800_20s) {
  
  VideoSender sender(NULL, 25.0f, 820, 0x1234, 0);
  EXPECT_EQ(102500u, sender.bytes_per_second());
  
  
  
  
  TestVideoSender(&sender, 9998, 1250, 100, 1025000);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
  
  TestVideoSender(&sender, 9999, 1250, 100, 1025000);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
  
  TestVideoSender(&sender, 39, 5, 100, 4100);
  
  TestVideoSender(&sender, 1, 0, 0, 0);
}

TEST(BweTestFramework_VideoSenderTest, TestAppendInOrder) {
  
  VideoSender sender1(NULL, 1.0f, 80, 0x1234, 0.25f);
  EXPECT_EQ(10000u, sender1.bytes_per_second());
  Packets packets;
  
  sender1.RunFor(999, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));
  ASSERT_TRUE(IsSequenceNumberSorted(packets));
  EXPECT_EQ(10u, packets.size());
  
  sender1.RunFor(1000, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));
  ASSERT_TRUE(IsSequenceNumberSorted(packets));
  EXPECT_EQ(20u, packets.size());

  
  VideoSender sender2(NULL, 2.0f, 160, 0x2234, 0.30f);
  EXPECT_EQ(20000u, sender2.bytes_per_second());
  
  
  sender2.RunFor(999, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));
  EXPECT_EQ(40u, packets.size());
  
  sender2.RunFor(1000, &packets);
  ASSERT_TRUE(IsTimeSorted(packets));
  EXPECT_EQ(60u, packets.size());
}

TEST(BweTestFramework_VideoSenderTest, FeedbackIneffective) {
  VideoSender sender(NULL, 25.0f, 820, 0x1234, 0);
  EXPECT_EQ(102500u, sender.bytes_per_second());
  TestVideoSender(&sender, 9998, 1250, 100, 1025000);

  
  PacketSender::Feedback feedback = { 512000 };
  sender.GiveFeedback(feedback);
  EXPECT_EQ(102500u, sender.bytes_per_second());
  TestVideoSender(&sender, 9998, 1250, 100, 1025000);
}

TEST(BweTestFramework_AdaptiveVideoSenderTest, FeedbackChangesBitrate) {
  AdaptiveVideoSender sender(NULL, 25.0f, 820, 0x1234, 0);
  EXPECT_EQ(102500u, sender.bytes_per_second());
  TestVideoSender(&sender, 9998, 1250, 100, 1025000);

  
  PacketSender::Feedback feedback = { 512000 };
  sender.GiveFeedback(feedback);
  EXPECT_EQ(64000u, sender.bytes_per_second());
  TestVideoSender(&sender, 9998, 750, 560, 640000);

  
  
  feedback.estimated_bps = 820000;
  sender.GiveFeedback(feedback);
  EXPECT_EQ(102500u, sender.bytes_per_second());
  TestVideoSender(&sender, 9998, 1250, 100, 1025000);
}
}  
}  
}  
