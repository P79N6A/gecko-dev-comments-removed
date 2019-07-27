









#include <string>

#include "webrtc/base/asyncudpsocket.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/virtualsocketserver.h"

namespace rtc {

class AsyncUdpSocketTest
    : public testing::Test,
      public sigslot::has_slots<> {
 public:
  AsyncUdpSocketTest()
      : pss_(new rtc::PhysicalSocketServer),
        vss_(new rtc::VirtualSocketServer(pss_.get())),
        socket_(vss_->CreateAsyncSocket(SOCK_DGRAM)),
        udp_socket_(new AsyncUDPSocket(socket_)),
        ready_to_send_(false) {
    udp_socket_->SignalReadyToSend.connect(this,
                                           &AsyncUdpSocketTest::OnReadyToSend);
  }

  void OnReadyToSend(rtc::AsyncPacketSocket* socket) {
    ready_to_send_ = true;
  }

 protected:
  scoped_ptr<PhysicalSocketServer> pss_;
  scoped_ptr<VirtualSocketServer> vss_;
  AsyncSocket* socket_;
  scoped_ptr<AsyncUDPSocket> udp_socket_;
  bool ready_to_send_;
};

TEST_F(AsyncUdpSocketTest, OnWriteEvent) {
  EXPECT_FALSE(ready_to_send_);
  socket_->SignalWriteEvent(socket_);
  EXPECT_TRUE(ready_to_send_);
}

}  
