









#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/video_engine/test/libvietest/include/fake_network_pipe.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::Invoke;

namespace webrtc {

class MockReceiver : public PacketReceiver {
 public:
  MockReceiver() {}
  virtual ~MockReceiver() {}

  void IncomingPacket(uint8_t* data, int length) {
    IncomingData(data, length);
    delete [] data;
  }

  MOCK_METHOD2(IncomingData, void(uint8_t*, int));
};

class FakeNetworkPipeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    TickTime::UseFakeClock(12345);
    receiver_.reset(new MockReceiver());
  }

  virtual void TearDown() {
  }

  void SendPackets(FakeNetworkPipe* pipe, int number_packets, int kPacketSize) {
    scoped_array<uint8_t> packet(new uint8_t[kPacketSize]);
    for (int i = 0; i < number_packets; ++i) {
      pipe->SendPacket(packet.get(), kPacketSize);
    }
  }

  int PacketTimeMs(int capacity_kbps, int kPacketSize) {
    return 8 * kPacketSize / capacity_kbps;
  }

  scoped_ptr<MockReceiver> receiver_;
};

void DeleteMemory(uint8_t* data, int length) { delete [] data; }


TEST_F(FakeNetworkPipeTest, CapacityTest) {
  FakeNetworkPipe::Configuration config;
  config.packet_receiver = receiver_.get();
  config.queue_length = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));

  
  
  const int kNumPackets = 10;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets , kPacketSize);

  
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(0);
  pipe->NetworkProcess();

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(1);
  pipe->NetworkProcess();

  
  TickTime::AdvanceFakeClock(9 * kPacketTimeMs - 1);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(8);
  pipe->NetworkProcess();

  
  TickTime::AdvanceFakeClock(1);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(1);
  pipe->NetworkProcess();
}


TEST_F(FakeNetworkPipeTest, ExtraDelayTest) {
  FakeNetworkPipe::Configuration config;
  config.packet_receiver = receiver_.get();
  config.queue_length = 20;
  config.queue_delay_ms = 100;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));

  const int kNumPackets = 2;
  const int kPacketSize = 1000;
  SendPackets(pipe.get(), kNumPackets , kPacketSize);

  
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(0);
  pipe->NetworkProcess();

  
  TickTime::AdvanceFakeClock(config.queue_delay_ms);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(1);
  pipe->NetworkProcess();

  
  TickTime::AdvanceFakeClock(kPacketTimeMs);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(1);
  pipe->NetworkProcess();
}



TEST_F(FakeNetworkPipeTest, QueueLengthTest) {
  FakeNetworkPipe::Configuration config;
  config.packet_receiver = receiver_.get();
  config.queue_length = 2;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));

  const int kPacketSize = 1000;
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  SendPackets(pipe.get(), 3, kPacketSize);

  
  
  TickTime::AdvanceFakeClock(3 * kPacketTimeMs);
  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(2);
  pipe->NetworkProcess();
}


TEST_F(FakeNetworkPipeTest, StatisticsTest) {
  FakeNetworkPipe::Configuration config;
  config.packet_receiver = receiver_.get();
  config.queue_length = 2;
  config.queue_delay_ms = 20;
  config.link_capacity_kbps = 80;
  scoped_ptr<FakeNetworkPipe> pipe(new FakeNetworkPipe(config));

  const int kPacketSize = 1000;
  const int kPacketTimeMs = PacketTimeMs(config.link_capacity_kbps,
                                         kPacketSize);

  
  SendPackets(pipe.get(), 3, kPacketSize);
  TickTime::AdvanceFakeClock(3 * kPacketTimeMs + config.queue_delay_ms);

  EXPECT_CALL(*receiver_, IncomingData(_, _))
      .Times(2);
  pipe->NetworkProcess();

  
  
  EXPECT_EQ(pipe->AverageDelay(), 170);
  EXPECT_EQ(pipe->sent_packets(), 2);
  EXPECT_EQ(pipe->dropped_packets(), 1);
  EXPECT_EQ(pipe->PercentageLoss(), 1/3.f);
}

}  
