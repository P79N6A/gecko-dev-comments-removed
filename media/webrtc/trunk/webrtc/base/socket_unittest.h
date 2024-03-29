









#ifndef WEBRTC_BASE_SOCKET_UNITTEST_H_
#define WEBRTC_BASE_SOCKET_UNITTEST_H_

#include "webrtc/base/gunit.h"
#include "webrtc/base/thread.h"

namespace rtc {




class SocketTest : public testing::Test {
 protected:
  SocketTest() : ss_(NULL), kIPv4Loopback(INADDR_LOOPBACK),
                 kIPv6Loopback(in6addr_loopback) {}
  virtual void SetUp() { ss_ = Thread::Current()->socketserver(); }
  void TestConnectIPv4();
  void TestConnectIPv6();
  void TestConnectWithDnsLookupIPv4();
  void TestConnectWithDnsLookupIPv6();
  void TestConnectFailIPv4();
  void TestConnectFailIPv6();
  void TestConnectWithDnsLookupFailIPv4();
  void TestConnectWithDnsLookupFailIPv6();
  void TestConnectWithClosedSocketIPv4();
  void TestConnectWithClosedSocketIPv6();
  void TestConnectWhileNotClosedIPv4();
  void TestConnectWhileNotClosedIPv6();
  void TestServerCloseDuringConnectIPv4();
  void TestServerCloseDuringConnectIPv6();
  void TestClientCloseDuringConnectIPv4();
  void TestClientCloseDuringConnectIPv6();
  void TestServerCloseIPv4();
  void TestServerCloseIPv6();
  void TestCloseInClosedCallbackIPv4();
  void TestCloseInClosedCallbackIPv6();
  void TestSocketServerWaitIPv4();
  void TestSocketServerWaitIPv6();
  void TestTcpIPv4();
  void TestTcpIPv6();
  void TestSingleFlowControlCallbackIPv4();
  void TestSingleFlowControlCallbackIPv6();
  void TestUdpIPv4();
  void TestUdpIPv6();
  void TestUdpReadyToSendIPv4();
  void TestUdpReadyToSendIPv6();
  void TestGetSetOptionsIPv4();
  void TestGetSetOptionsIPv6();

 private:
  void ConnectInternal(const IPAddress& loopback);
  void ConnectWithDnsLookupInternal(const IPAddress& loopback,
                                    const std::string& host);
  void ConnectFailInternal(const IPAddress& loopback);

  void ConnectWithDnsLookupFailInternal(const IPAddress& loopback);
  void ConnectWithClosedSocketInternal(const IPAddress& loopback);
  void ConnectWhileNotClosedInternal(const IPAddress& loopback);
  void ServerCloseDuringConnectInternal(const IPAddress& loopback);
  void ClientCloseDuringConnectInternal(const IPAddress& loopback);
  void ServerCloseInternal(const IPAddress& loopback);
  void CloseInClosedCallbackInternal(const IPAddress& loopback);
  void SocketServerWaitInternal(const IPAddress& loopback);
  void TcpInternal(const IPAddress& loopback);
  void SingleFlowControlCallbackInternal(const IPAddress& loopback);
  void UdpInternal(const IPAddress& loopback);
  void UdpReadyToSend(const IPAddress& loopback);
  void GetSetOptionsInternal(const IPAddress& loopback);

  static const int kTimeout = 5000;  
  SocketServer* ss_;
  const IPAddress kIPv4Loopback;
  const IPAddress kIPv6Loopback;
};

}  

#endif  
