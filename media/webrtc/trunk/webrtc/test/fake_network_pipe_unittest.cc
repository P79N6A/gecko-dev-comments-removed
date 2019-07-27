









#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/call.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/test/fake_network_pipe.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::Invoke;

namespace webrtc {

class MockReceiver : public PacketReceiver {
 public:
  MockReceiver() {}
  virtual ~MockReceiver() {}

  void IncomingPacket(const uint8_t* data, size_t length) {
    DeliverPacket(data, length);
    delete [] data;
  }

  MOCK_METHOD2(DeliverPacket, DeliveryStatus(const uint8_t*, size_t));
};

class FakeNetworkPipeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    TickTime::UseFakeClock(12345);
    receiver_.reset(new MockReceiver());
    ON_CALL(*receiver_, DeliverPacket(_, _))
        .WillByDefault(Return(PacketReceiver::DELIVERY_OK));
  }

  virtual void TearDown() {
  }

  void SendPackets(FakeNetworkPipe* pipe, int number_packets, int kPacketSize) {
    scoped_ptr<uint8_t[]> packet(new uint8_t[kPacketSize]);
    for (int i = 0; i < number_packets; ++i) {
      pipe->SendPacket(packet.get(), kPacketSize);
    }
  }

  int PacketTimeMs(int capacity_kbps, int kPacketSize) const {
    return 8 * kPacketSize / capacity_kbps;
  }

  scoped_ptr<MockReceiver> receiver_;
};

void DeleteMemory(uint8_t* data, int length) { delete [] data; }


TEST_F(FakeNetworkPipeTest, CapacityTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  
  
  const int kNumPackets = 10;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets , kPacketSize);

  
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(0);
  pipe->Process();

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(1);
  pipe->Process();

  
  TickTime::AdvanceFakeClock(9 * kPacketTimeMs - 1);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(8);
  pipe->Process();

  
  TickTime::AdvanceFakeClock(1);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(1);
  pipe->Process();
}


TEST_F(FakeNetworkPipeTest, ExtraDelayTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 20;
  config.queue_delay_ms = 100;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  const int kNumPackets = 2;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets , kPacketSize);

  
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(0);
  pipe->Process();

  
  TickTime::AdvanceFakeClock(config.queue_delay_ms);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(1);
  pipe->Process();

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(1);
  pipe->Process();
}



TEST_F(FakeNetworkPipeTest, QueueLengthTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 2;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  const int kPacketSize = 1000;
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  SendPackets(pipe.get(), 3, kPacketSize);

  
  
  TickTime::AdvanceFakeClock(3 * kPacketTimeMs);
  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(2);
  pipe->Process();
}


TEST_F(FakeNetworkPipeTest, StatisticsTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 2;
  config.queue_delay_ms = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  const int kPacketSize = 1000;
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  SendPackets(pipe.get(), 3, kPacketSize);
  TickTime::AdvanceFakeClock(3 * kPacketTimeMs + config.queue_delay_ms);

  EXPECT_CALL(*receiver_, DeliverPacket(_, _))
      .Times(2);
  pipe->Process();

  
  
  EXPECT_EQ(pipe->AverageDelay(), 170);
  EXPECT_EQ(pipe->sent_packets(), 2u);
  EXPECT_EQ(pipe->dropped_packets(), 1u);
  EXPECT_EQ(pipe->PercentageLoss(), 1/3.f);
}



TEST_F(FakeNetworkPipeTest, ChangingCapacityWithEmptyPipeTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  
  
  const int kNumPackets = 10;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets, kPacketSize);

  
  int packet_time_ms = PacketTimeMs(config.link_capacity_kbps, kPacketSize);

  
  EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(0);
  pipe->Process();

  
  for (int i = 0; i < kNumPackets; ++i) {
    TickTime::AdvanceFakeClock(packet_time_ms);
    EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(1);
    pipe->Process();
  }

  
  config.link_capacity_kbps /= 2;  
  pipe->SetConfig(config);

  
  
  SendPackets(pipe.get(), kNumPackets, kPacketSize);

  
  packet_time_ms = PacketTimeMs(config.link_capacity_kbps, kPacketSize);

  
  EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(0);
  pipe->Process();

  
  for (int i = 0; i < kNumPackets; ++i) {
    TickTime::AdvanceFakeClock(packet_time_ms);
    EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(1);
    pipe->Process();
  }

  
  EXPECT_EQ(static_cast<size_t>(2 * kNumPackets), pipe->sent_packets());
  TickTime::AdvanceFakeClock(pipe->TimeUntilNextProcess());
  EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(0);
  pipe->Process();
}



TEST_F(FakeNetworkPipeTest, ChangingCapacityWithPacketsInPipeTest) {
  FakeNetworkPipe::Config config;
  config.queue_length_packets = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));
  pipe->SetReceiver(receiver_.get());

  
  const int kNumPackets = 10;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets, kPacketSize);

  
  int packet_time_1_ms = PacketTimeMs(config.link_capacity_kbps, kPacketSize);

  
  config.link_capacity_kbps *= 2;  
  pipe->SetConfig(config);

  
  
  SendPackets(pipe.get(), kNumPackets, kPacketSize);

  
  int packet_time_2_ms = PacketTimeMs(config.link_capacity_kbps, kPacketSize);

  
  EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(0);
  pipe->Process();

  
  for (int i = 0; i < kNumPackets; ++i) {
    TickTime::AdvanceFakeClock(packet_time_1_ms);
    EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(1);
    pipe->Process();
  }

  
  for (int i = 0; i < kNumPackets; ++i) {
    TickTime::AdvanceFakeClock(packet_time_2_ms);
    EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(1);
    pipe->Process();
  }

  
  EXPECT_EQ(static_cast<size_t>(2 * kNumPackets), pipe->sent_packets());
  TickTime::AdvanceFakeClock(pipe->TimeUntilNextProcess());
  EXPECT_CALL(*receiver_, DeliverPacket(_, _)).Times(0);
  pipe->Process();
}
}  
