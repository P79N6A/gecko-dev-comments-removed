









#include <string>

#include "webrtc/base/asynctcpsocket.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/virtualsocketserver.h"

namespace rtc {

class AsyncTCPSocketTest
    : public testing::Test,
      public sigslot::has_slots<> {
 public:
  AsyncTCPSocketTest()
      : pss_(new rtc::PhysicalSocketServer),
        vss_(new rtc::VirtualSocketServer(pss_.get())),
        socket_(vss_->CreateAsyncSocket(SOCK_STREAM)),
        tcp_socket_(new AsyncTCPSocket(socket_, true)),
        ready_to_send_(false) {
    tcp_socket_->SignalReadyToSend.connect(this,
                                           &AsyncTCPSocketTest::OnReadyToSend);
  }

  void OnReadyToSend(rtc::AsyncPacketSocket* socket) {
    ready_to_send_ = true;
  }

 protected:
  scoped_ptr<PhysicalSocketServer> pss_;
  scoped_ptr<VirtualSocketServer> vss_;
  AsyncSocket* socket_;
  scoped_ptr<AsyncTCPSocket> tcp_socket_;
  bool ready_to_send_;
};

TEST_F(AsyncTCPSocketTest, OnWriteEvent) {
  EXPECT_FALSE(ready_to_send_);
  socket_->SignalWriteEvent(socket_);
  EXPECT_TRUE(ready_to_send_);
}

}  
