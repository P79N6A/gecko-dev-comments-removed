









#include "webrtc/base/socket_unittest.h"

#include "webrtc/base/asyncudpsocket.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/nethelpers.h"
#include "webrtc/base/socketserver.h"
#include "webrtc/base/testclient.h"
#include "webrtc/base/testutils.h"
#include "webrtc/base/thread.h"

namespace rtc {

#define MAYBE_SKIP_IPV6                             \
  if (!HasIPv6Enabled()) {                          \
    LOG(LS_INFO) << "No IPv6... skipping";          \
    return;                                         \
  }


void SocketTest::TestConnectIPv4() {
  ConnectInternal(kIPv4Loopback);
}

void SocketTest::TestConnectIPv6() {
  MAYBE_SKIP_IPV6;
  ConnectInternal(kIPv6Loopback);
}

void SocketTest::TestConnectWithDnsLookupIPv4() {
  ConnectWithDnsLookupInternal(kIPv4Loopback, "localhost");
}

void SocketTest::TestConnectWithDnsLookupIPv6() {
  
  LOG(LS_INFO) << "Skipping IPv6 DNS test";
  
}

void SocketTest::TestConnectFailIPv4() {
  ConnectFailInternal(kIPv4Loopback);
}

void SocketTest::TestConnectFailIPv6() {
  MAYBE_SKIP_IPV6;
  ConnectFailInternal(kIPv6Loopback);
}

void SocketTest::TestConnectWithDnsLookupFailIPv4() {
  ConnectWithDnsLookupFailInternal(kIPv4Loopback);
}

void SocketTest::TestConnectWithDnsLookupFailIPv6() {
  MAYBE_SKIP_IPV6;
  ConnectWithDnsLookupFailInternal(kIPv6Loopback);
}

void SocketTest::TestConnectWithClosedSocketIPv4() {
  ConnectWithClosedSocketInternal(kIPv4Loopback);
}

void SocketTest::TestConnectWithClosedSocketIPv6() {
  MAYBE_SKIP_IPV6;
  ConnectWithClosedSocketInternal(kIPv6Loopback);
}

void SocketTest::TestConnectWhileNotClosedIPv4() {
  ConnectWhileNotClosedInternal(kIPv4Loopback);
}

void SocketTest::TestConnectWhileNotClosedIPv6() {
  MAYBE_SKIP_IPV6;
  ConnectWhileNotClosedInternal(kIPv6Loopback);
}

void SocketTest::TestServerCloseDuringConnectIPv4() {
  ServerCloseDuringConnectInternal(kIPv4Loopback);
}

void SocketTest::TestServerCloseDuringConnectIPv6() {
  MAYBE_SKIP_IPV6;
  ServerCloseDuringConnectInternal(kIPv6Loopback);
}

void SocketTest::TestClientCloseDuringConnectIPv4() {
  ClientCloseDuringConnectInternal(kIPv4Loopback);
}

void SocketTest::TestClientCloseDuringConnectIPv6() {
  MAYBE_SKIP_IPV6;
  ClientCloseDuringConnectInternal(kIPv6Loopback);
}

void SocketTest::TestServerCloseIPv4() {
  ServerCloseInternal(kIPv4Loopback);
}

void SocketTest::TestServerCloseIPv6() {
  MAYBE_SKIP_IPV6;
  ServerCloseInternal(kIPv6Loopback);
}

void SocketTest::TestCloseInClosedCallbackIPv4() {
  CloseInClosedCallbackInternal(kIPv4Loopback);
}

void SocketTest::TestCloseInClosedCallbackIPv6() {
  MAYBE_SKIP_IPV6;
  CloseInClosedCallbackInternal(kIPv6Loopback);
}

void SocketTest::TestSocketServerWaitIPv4() {
  SocketServerWaitInternal(kIPv4Loopback);
}

void SocketTest::TestSocketServerWaitIPv6() {
  MAYBE_SKIP_IPV6;
  SocketServerWaitInternal(kIPv6Loopback);
}

void SocketTest::TestTcpIPv4() {
  TcpInternal(kIPv4Loopback);
}

void SocketTest::TestTcpIPv6() {
  MAYBE_SKIP_IPV6;
  TcpInternal(kIPv6Loopback);
}

void SocketTest::TestSingleFlowControlCallbackIPv4() {
  SingleFlowControlCallbackInternal(kIPv4Loopback);
}

void SocketTest::TestSingleFlowControlCallbackIPv6() {
  MAYBE_SKIP_IPV6;
  SingleFlowControlCallbackInternal(kIPv6Loopback);
}

void SocketTest::TestUdpIPv4() {
  UdpInternal(kIPv4Loopback);
}

void SocketTest::TestUdpIPv6() {
  MAYBE_SKIP_IPV6;
  UdpInternal(kIPv6Loopback);
}

void SocketTest::TestUdpReadyToSendIPv4() {
#if !defined(WEBRTC_MAC)
  
  UdpReadyToSend(kIPv4Loopback);
#endif
}

void SocketTest::TestUdpReadyToSendIPv6() {
#if defined(WEBRTC_WIN)
  
  MAYBE_SKIP_IPV6;
  UdpReadyToSend(kIPv6Loopback);
#endif
}

void SocketTest::TestGetSetOptionsIPv4() {
  GetSetOptionsInternal(kIPv4Loopback);
}

void SocketTest::TestGetSetOptionsIPv6() {
  MAYBE_SKIP_IPV6;
  GetSetOptionsInternal(kIPv6Loopback);
}



bool IsUnspecOrEmptyIP(const IPAddress& address) {
#if !defined(WEBRTC_WIN)
  return IPIsAny(address);
#else
  return address.family() == AF_UNSPEC;
#endif
}

void SocketTest::ConnectInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(ss_->CreateAsyncSocket(loopback.family(),
                                                        SOCK_STREAM));
  sink.Monitor(client.get());
  EXPECT_EQ(AsyncSocket::CS_CLOSED, client->GetState());
  EXPECT_PRED1(IsUnspecOrEmptyIP, client->GetLocalAddress().ipaddr());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));
  EXPECT_EQ(AsyncSocket::CS_CONNECTING, server->GetState());

  
  EXPECT_FALSE(sink.Check(server.get(), testing::SSE_READ));
  EXPECT_TRUE(NULL == server->Accept(&accept_addr));
  EXPECT_TRUE(accept_addr.IsNil());

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));
  EXPECT_FALSE(client->GetLocalAddress().IsNil());
  EXPECT_NE(server->GetLocalAddress(), client->GetLocalAddress());

  
  EXPECT_EQ(AsyncSocket::CS_CONNECTING, client->GetState());
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  EXPECT_FALSE(accept_addr.IsNil());
  EXPECT_EQ(accepted->GetRemoteAddress(), accept_addr);

  
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());
  EXPECT_EQ(server->GetLocalAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(client->GetLocalAddress(), accepted->GetRemoteAddress());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_EQ(client->GetRemoteAddress(), server->GetLocalAddress());
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
}

void SocketTest::ConnectWithDnsLookupInternal(const IPAddress& loopback,
                                              const std::string& host) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  SocketAddress dns_addr(server->GetLocalAddress());
  dns_addr.SetIP(host);
  EXPECT_EQ(0, client->Connect(dns_addr));
  
  

  
  EXPECT_EQ(AsyncSocket::CS_CONNECTING, client->GetState());
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  EXPECT_FALSE(accept_addr.IsNil());
  EXPECT_EQ(accepted->GetRemoteAddress(), accept_addr);

  
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());
  EXPECT_EQ(server->GetLocalAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(client->GetLocalAddress(), accepted->GetRemoteAddress());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_EQ(client->GetRemoteAddress(), server->GetLocalAddress());
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
}

void SocketTest::ConnectFailInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));

  
  
  
  SocketAddress bogus_addr(loopback, 65535);
  EXPECT_EQ(0, client->Connect(bogus_addr));

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_ERROR));
  EXPECT_TRUE(client->GetRemoteAddress().IsNil());

  
  EXPECT_FALSE(sink.Check(server.get(), testing::SSE_READ));
  EXPECT_TRUE(NULL == server->Accept(&accept_addr));
  EXPECT_EQ(IPAddress(), accept_addr.ipaddr());
}

void SocketTest::ConnectWithDnsLookupFailInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));

  
  
  
  SocketAddress bogus_dns_addr("not-a-real-hostname", 65535);
  EXPECT_EQ(0, client->Connect(bogus_dns_addr));

  
  bool dns_lookup_finished = false;
  WAIT_(client->GetState() == AsyncSocket::CS_CLOSED, kTimeout,
        dns_lookup_finished);
  if (!dns_lookup_finished) {
    LOG(LS_WARNING) << "Skipping test; DNS resolution took longer than 5 "
                    << "seconds.";
    return;
  }

  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_ERROR));
  EXPECT_TRUE(client->GetRemoteAddress().IsNil());
  
  EXPECT_FALSE(sink.Check(server.get(), testing::SSE_READ));
  EXPECT_TRUE(NULL == server->Accept(&accept_addr));
  EXPECT_TRUE(accept_addr.IsNil());
}

void SocketTest::ConnectWithClosedSocketInternal(const IPAddress& loopback) {
  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  EXPECT_EQ(0, client->Close());
  EXPECT_EQ(AsyncSocket::CS_CLOSED, client->GetState());

  
  EXPECT_EQ(0, client->Connect(SocketAddress(server->GetLocalAddress())));
  EXPECT_EQ(AsyncSocket::CS_CONNECTING, client->GetState());
}

void SocketTest::ConnectWhileNotClosedInternal(const IPAddress& loopback) {
  
  testing::StreamSink sink;
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));
  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  EXPECT_EQ(0, client->Connect(SocketAddress(server->GetLocalAddress())));
  EXPECT_EQ(AsyncSocket::CS_CONNECTING, client->GetState());
  
  EXPECT_EQ(SOCKET_ERROR,
            client->Connect(SocketAddress(server->GetLocalAddress())));

  
  SocketAddress accept_addr;
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  EXPECT_FALSE(accept_addr.IsNil());

  
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());
  EXPECT_EQ(server->GetLocalAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(client->GetLocalAddress(), accepted->GetRemoteAddress());
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_EQ(client->GetRemoteAddress(), server->GetLocalAddress());
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());

  
  
  EXPECT_EQ(SOCKET_ERROR,
            client->Connect(SocketAddress("localhost",
                                          server->GetLocalAddress().port())));
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, client->GetState());
  EXPECT_EQ(client->GetRemoteAddress(), server->GetLocalAddress());
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
}

void SocketTest::ServerCloseDuringConnectInternal(const IPAddress& loopback) {
  testing::StreamSink sink;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT(sink.Check(server.get(), testing::SSE_READ), kTimeout);
  server->Close();

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_ERROR));
  client->Close();
}

void SocketTest::ClientCloseDuringConnectInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT(sink.Check(server.get(), testing::SSE_READ), kTimeout);
  client->Close();

  
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, accepted->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(accepted.get(), testing::SSE_CLOSE) ||
              sink.Check(accepted.get(), testing::SSE_ERROR));

  
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
}

void SocketTest::ServerCloseInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(accepted->GetRemoteAddress(), client->GetLocalAddress());

  
  EXPECT_EQ(1, accepted->Send("a", 1));
  accepted->Close();
  EXPECT_EQ(AsyncSocket::CS_CLOSED, accepted->GetState());

  
  EXPECT_TRUE_WAIT(sink.Check(client.get(), testing::SSE_READ), kTimeout);
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, client->GetState());

  
  char buffer[10];
  EXPECT_EQ(1, client->Recv(buffer, sizeof(buffer)));
  EXPECT_EQ('a', buffer[0]);

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_FALSE(client->GetRemoteAddress().IsAnyIP());

  
  EXPECT_FALSE(sink.Check(accepted.get(), testing::SSE_CLOSE));
  EXPECT_TRUE(accepted->GetRemoteAddress().IsNil());

  
  Thread::Current()->ProcessMessages(0);
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));

  
  client->Close();
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_TRUE(client->GetRemoteAddress().IsNil());
}

class SocketCloser : public sigslot::has_slots<> {
 public:
  void OnClose(AsyncSocket* socket, int error) {
    socket->Close();  
                      
  }
};

void SocketTest::CloseInClosedCallbackInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketCloser closer;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());
  client->SignalCloseEvent.connect(&closer, &SocketCloser::OnClose);

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(accepted->GetRemoteAddress(), client->GetLocalAddress());

  
  accepted->Close();
  EXPECT_EQ(AsyncSocket::CS_CLOSED, accepted->GetState());

  
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, client->GetState());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_TRUE(Socket::CS_CLOSED == client->GetState());
}

class Sleeper : public MessageHandler {
 public:
  Sleeper() {}
  void OnMessage(Message* msg) {
    Thread::Current()->SleepMs(500);
  }
};

void SocketTest::SocketServerWaitInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);

  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, accepted->GetState());
  EXPECT_EQ(server->GetLocalAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(client->GetLocalAddress(), accepted->GetRemoteAddress());

  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_FALSE(sink.Check(client.get(), testing::SSE_CLOSE));
  EXPECT_EQ(client->GetRemoteAddress(), server->GetLocalAddress());
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());

  
  EXPECT_FALSE(sink.Check(accepted.get(), testing::SSE_READ));
  char buf[1024] = {0};

  EXPECT_EQ(1024, client->Send(buf, 1024));
  EXPECT_FALSE(sink.Check(accepted.get(), testing::SSE_READ));

  
  scoped_ptr<Thread> thread(new Thread());
  thread->Start();
  Sleeper sleeper;
  TypedMessageData<AsyncSocket*> data(client.get());
  thread->Send(&sleeper, 0, &data);
  EXPECT_FALSE(sink.Check(accepted.get(), testing::SSE_READ));

  
  EXPECT_TRUE_WAIT((sink.Check(accepted.get(), testing::SSE_READ)), kTimeout);
  EXPECT_LT(0, accepted->Recv(buf, 1024));
}

void SocketTest::TcpInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  const size_t kDataSize = 1024 * 1024;
  scoped_ptr<char[]> send_buffer(new char[kDataSize]);
  scoped_ptr<char[]> recv_buffer(new char[kDataSize]);
  size_t send_pos = 0, recv_pos = 0;
  for (size_t i = 0; i < kDataSize; ++i) {
    send_buffer[i] = static_cast<char>(i % 256);
    recv_buffer[i] = 0;
  }

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(accepted->GetRemoteAddress(), client->GetLocalAddress());

  
  bool send_waiting_for_writability = false;
  bool send_expect_success = true;
  bool recv_waiting_for_readability = true;
  bool recv_expect_success = false;
  int data_in_flight = 0;
  while (recv_pos < kDataSize) {
    
    while (!send_waiting_for_writability && send_pos < kDataSize) {
      int tosend = static_cast<int>(kDataSize - send_pos);
      int sent = accepted->Send(send_buffer.get() + send_pos, tosend);
      if (send_expect_success) {
        
        
        EXPECT_GT(sent, 0);
        send_expect_success = false;
      }
      if (sent >= 0) {
        EXPECT_LE(sent, tosend);
        send_pos += sent;
        data_in_flight += sent;
      } else {
        ASSERT_TRUE(accepted->IsBlocking());
        send_waiting_for_writability = true;
      }
    }

    
    while (data_in_flight > 0) {
      if (recv_waiting_for_readability) {
        
        EXPECT_TRUE_WAIT(sink.Check(client.get(), testing::SSE_READ), kTimeout);
        recv_waiting_for_readability = false;
        recv_expect_success = true;
      }

      
      int rcvd = client->Recv(recv_buffer.get() + recv_pos,
                              kDataSize - recv_pos);

      if (recv_expect_success) {
        
        
        
        
        
        recv_expect_success = false;
      }
      if (rcvd >= 0) {
        EXPECT_LE(rcvd, data_in_flight);
        recv_pos += rcvd;
        data_in_flight -= rcvd;
      } else {
        ASSERT_TRUE(client->IsBlocking());
        recv_waiting_for_readability = true;
      }
    }

    
    if (send_waiting_for_writability) {
      EXPECT_TRUE_WAIT(sink.Check(accepted.get(), testing::SSE_WRITE),
                       kTimeout);
      send_waiting_for_writability = false;
      send_expect_success = true;
    }
  }

  
  EXPECT_EQ(kDataSize, send_pos);
  EXPECT_EQ(kDataSize, recv_pos);
  EXPECT_EQ(0, memcmp(recv_buffer.get(), send_buffer.get(), kDataSize));

  
  accepted->Close();
  EXPECT_EQ_WAIT(AsyncSocket::CS_CLOSED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_CLOSE));
  client->Close();
}

void SocketTest::SingleFlowControlCallbackInternal(const IPAddress& loopback) {
  testing::StreamSink sink;
  SocketAddress accept_addr;

  
  scoped_ptr<AsyncSocket> client(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(client.get());

  
  scoped_ptr<AsyncSocket> server(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_STREAM));
  sink.Monitor(server.get());
  EXPECT_EQ(0, server->Bind(SocketAddress(loopback, 0)));
  EXPECT_EQ(0, server->Listen(5));

  
  EXPECT_EQ(0, client->Connect(server->GetLocalAddress()));

  
  EXPECT_TRUE_WAIT((sink.Check(server.get(), testing::SSE_READ)), kTimeout);
  scoped_ptr<AsyncSocket> accepted(server->Accept(&accept_addr));
  ASSERT_TRUE(accepted);
  sink.Monitor(accepted.get());

  
  EXPECT_EQ_WAIT(AsyncSocket::CS_CONNECTED, client->GetState(), kTimeout);
  EXPECT_TRUE(sink.Check(client.get(), testing::SSE_OPEN));
  EXPECT_EQ(client->GetRemoteAddress(), accepted->GetLocalAddress());
  EXPECT_EQ(accepted->GetRemoteAddress(), client->GetLocalAddress());

  
  EXPECT_TRUE_WAIT(sink.Check(accepted.get(), testing::SSE_WRITE), kTimeout);

  
  char buf[1024 * 16] = {0};
  int sends = 0;
  while (++sends && accepted->Send(&buf, ARRAY_SIZE(buf)) != -1) {}
  EXPECT_TRUE(accepted->IsBlocking());

  
  EXPECT_TRUE_WAIT(sink.Check(client.get(), testing::SSE_READ), kTimeout);

  
  for (int i = 0; i < sends; ++i) {
    client->Recv(buf, ARRAY_SIZE(buf));
  }

  
  EXPECT_TRUE_WAIT(sink.Check(accepted.get(), testing::SSE_WRITE), kTimeout);

  
  
  int extras = 0;
  for (int i = 0; i < 100; ++i) {
    accepted->Send(&buf, ARRAY_SIZE(buf));
    rtc::Thread::Current()->ProcessMessages(1);
    if (sink.Check(accepted.get(), testing::SSE_WRITE)) {
      extras++;
    }
  }
  EXPECT_LT(extras, 2);

  
  accepted->Close();
  client->Close();
}

void SocketTest::UdpInternal(const IPAddress& loopback) {
  SocketAddress empty = EmptySocketAddressWithFamily(loopback.family());
  
  AsyncSocket* socket =
      ss_->CreateAsyncSocket(loopback.family(), SOCK_DGRAM);
  EXPECT_EQ(AsyncSocket::CS_CLOSED, socket->GetState());
  EXPECT_EQ(0, socket->Bind(SocketAddress(loopback, 0)));
  SocketAddress addr1 = socket->GetLocalAddress();
  EXPECT_EQ(0, socket->Connect(addr1));
  EXPECT_EQ(AsyncSocket::CS_CONNECTED, socket->GetState());
  socket->Close();
  EXPECT_EQ(AsyncSocket::CS_CLOSED, socket->GetState());
  delete socket;

  
  scoped_ptr<TestClient> client1(
      new TestClient(AsyncUDPSocket::Create(ss_, addr1)));
  scoped_ptr<TestClient> client2(
      new TestClient(AsyncUDPSocket::Create(ss_, empty)));

  SocketAddress addr2;
  EXPECT_EQ(3, client2->SendTo("foo", 3, addr1));
  EXPECT_TRUE(client1->CheckNextPacket("foo", 3, &addr2));

  SocketAddress addr3;
  EXPECT_EQ(6, client1->SendTo("bizbaz", 6, addr2));
  EXPECT_TRUE(client2->CheckNextPacket("bizbaz", 6, &addr3));
  EXPECT_EQ(addr3, addr1);
  
  for (int i = 0; i < 10; ++i) {
    client2.reset(new TestClient(AsyncUDPSocket::Create(ss_, empty)));

    SocketAddress addr4;
    EXPECT_EQ(3, client2->SendTo("foo", 3, addr1));
    EXPECT_TRUE(client1->CheckNextPacket("foo", 3, &addr4));
    EXPECT_EQ(addr4.ipaddr(), addr2.ipaddr());

    SocketAddress addr5;
    EXPECT_EQ(6, client1->SendTo("bizbaz", 6, addr4));
    EXPECT_TRUE(client2->CheckNextPacket("bizbaz", 6, &addr5));
    EXPECT_EQ(addr5, addr1);

    addr2 = addr4;
  }
}

void SocketTest::UdpReadyToSend(const IPAddress& loopback) {
  SocketAddress empty = EmptySocketAddressWithFamily(loopback.family());
  
  
  
  std::string dest = (loopback.family() == AF_INET6) ?
      "2001:db8::1" : "192.0.2.0";
  SocketAddress test_addr(dest, 2345);

  
  scoped_ptr<TestClient> client(
      new TestClient(AsyncUDPSocket::Create(ss_, empty)));
  int test_packet_size = 1200;
  rtc::scoped_ptr<char[]> test_packet(new char[test_packet_size]);
  
  memset(test_packet.get(), 0, test_packet_size);
  
  
  int send_buffer_size = test_packet_size;
#if defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
  send_buffer_size /= 2;
#endif
  client->SetOption(rtc::Socket::OPT_SNDBUF, send_buffer_size);

  int error = 0;
  uint32 start_ms = Time();
  int sent_packet_num = 0;
  int expected_error = EWOULDBLOCK;
  while (start_ms + kTimeout > Time()) {
    int ret = client->SendTo(test_packet.get(), test_packet_size, test_addr);
    ++sent_packet_num;
    if (ret != test_packet_size) {
      error = client->GetError();
      if (error == expected_error) {
        LOG(LS_INFO) << "Got expected error code after sending "
                     << sent_packet_num << " packets.";
        break;
      }
    }
  }
  EXPECT_EQ(expected_error, error);
  EXPECT_FALSE(client->ready_to_send());
  EXPECT_TRUE_WAIT(client->ready_to_send(), kTimeout);
  LOG(LS_INFO) << "Got SignalReadyToSend";
}

void SocketTest::GetSetOptionsInternal(const IPAddress& loopback) {
  rtc::scoped_ptr<AsyncSocket> socket(
      ss_->CreateAsyncSocket(loopback.family(), SOCK_DGRAM));
  socket->Bind(SocketAddress(loopback, 0));

  
  const int desired_size = 12345;
#if defined(WEBRTC_LINUX)
  
  const int expected_size = desired_size * 2;
#else   
  const int expected_size = desired_size;
#endif  
  int recv_size = 0;
  int send_size = 0;
  
  ASSERT_NE(-1, socket->GetOption(Socket::OPT_RCVBUF, &recv_size));
  ASSERT_NE(-1, socket->GetOption(Socket::OPT_SNDBUF, &send_size));
  
  ASSERT_NE(-1, socket->SetOption(Socket::OPT_RCVBUF, desired_size));
  ASSERT_NE(-1, socket->SetOption(Socket::OPT_SNDBUF, desired_size));
  
  ASSERT_NE(-1, socket->GetOption(Socket::OPT_RCVBUF, &recv_size));
  ASSERT_NE(-1, socket->GetOption(Socket::OPT_SNDBUF, &send_size));
  
  ASSERT_EQ(expected_size, recv_size);
  ASSERT_EQ(expected_size, send_size);

  
  int current_nd, desired_nd = 1;
  ASSERT_EQ(-1, socket->GetOption(Socket::OPT_NODELAY, &current_nd));
  ASSERT_EQ(-1, socket->SetOption(Socket::OPT_NODELAY, desired_nd));

  
  if (loopback.family() != AF_INET6) {
    
    rtc::scoped_ptr<AsyncSocket>
        mtu_socket(
            ss_->CreateAsyncSocket(loopback.family(), SOCK_DGRAM));
    mtu_socket->Bind(SocketAddress(loopback, 0));
    uint16 mtu;
    
    ASSERT_EQ(-1, mtu_socket->EstimateMTU(&mtu));
    mtu_socket->Connect(SocketAddress(loopback, 0));
#if defined(WEBRTC_WIN)
    
    ASSERT_NE(-1, mtu_socket->EstimateMTU(&mtu));
    ASSERT_GE(mtu, 1492);  
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
    
    ASSERT_EQ(-1, mtu_socket->EstimateMTU(&mtu));
#else
    
    
    
#endif
  }
}

}  
