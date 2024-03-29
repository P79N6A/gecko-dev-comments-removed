









#include "webrtc/base/gunit.h"
#include "webrtc/base/nethelpers.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/testclient.h"
#include "webrtc/base/testechoserver.h"
#include "webrtc/base/thread.h"
#include "webrtc/test/testsupport/gtest_disable.h"

using namespace rtc;

void TestUdpInternal(const SocketAddress& loopback) {
  Thread *main = Thread::Current();
  AsyncSocket* socket = main->socketserver()
      ->CreateAsyncSocket(loopback.family(), SOCK_DGRAM);
  socket->Bind(loopback);

  TestClient client(new AsyncUDPSocket(socket));
  SocketAddress addr = client.address(), from;
  EXPECT_EQ(3, client.SendTo("foo", 3, addr));
  EXPECT_TRUE(client.CheckNextPacket("foo", 3, &from));
  EXPECT_EQ(from, addr);
  EXPECT_TRUE(client.CheckNoPacket());
}

void TestTcpInternal(const SocketAddress& loopback) {
  Thread *main = Thread::Current();
  TestEchoServer server(main, loopback);

  AsyncSocket* socket = main->socketserver()
      ->CreateAsyncSocket(loopback.family(), SOCK_STREAM);
  AsyncTCPSocket* tcp_socket = AsyncTCPSocket::Create(
      socket, loopback, server.address());
  ASSERT_TRUE(tcp_socket != NULL);

  TestClient client(tcp_socket);
  SocketAddress addr = client.address(), from;
  EXPECT_TRUE(client.CheckConnected());
  EXPECT_EQ(3, client.Send("foo", 3));
  EXPECT_TRUE(client.CheckNextPacket("foo", 3, &from));
  EXPECT_EQ(from, server.address());
  EXPECT_TRUE(client.CheckNoPacket());
}


TEST(TestClientTest, TestUdpIPv4) {
  TestUdpInternal(SocketAddress("127.0.0.1", 0));
}

TEST(TestClientTest, TestUdpIPv6) {
  if (HasIPv6Enabled()) {
    TestUdpInternal(SocketAddress("::1", 0));
  } else {
    LOG(LS_INFO) << "Skipping IPv6 test.";
  }
}


TEST(TestClientTest, TestTcpIPv4) {
  TestTcpInternal(SocketAddress("127.0.0.1", 0));
}

TEST(TestClientTest, TestTcpIPv6) {
  if (HasIPv6Enabled()) {
    TestTcpInternal(SocketAddress("::1", 0));
  } else {
    LOG(LS_INFO) << "Skipping IPv6 test.";
  }
}
