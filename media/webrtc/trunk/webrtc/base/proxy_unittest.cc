









#include <string>
#include "webrtc/base/autodetectproxy.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/httpserver.h"
#include "webrtc/base/proxyserver.h"
#include "webrtc/base/socketadapters.h"
#include "webrtc/base/testclient.h"
#include "webrtc/base/testechoserver.h"
#include "webrtc/base/virtualsocketserver.h"
#include "webrtc/test/testsupport/gtest_disable.h"

using rtc::Socket;
using rtc::Thread;
using rtc::SocketAddress;

static const SocketAddress kSocksProxyIntAddr("1.2.3.4", 1080);
static const SocketAddress kSocksProxyExtAddr("1.2.3.5", 0);
static const SocketAddress kHttpsProxyIntAddr("1.2.3.4", 443);
static const SocketAddress kHttpsProxyExtAddr("1.2.3.5", 0);
static const SocketAddress kBogusProxyIntAddr("1.2.3.4", 999);



class AutoDetectProxyRunner : public rtc::AutoDetectProxy {
 public:
  explicit AutoDetectProxyRunner(const std::string& agent)
      : AutoDetectProxy(agent) {}
  void Run() {
    DoWork();
    Thread::Current()->Restart();  
  }
};


class ProxyTest : public testing::Test {
 public:
  ProxyTest() : ss_(new rtc::VirtualSocketServer(NULL)) {
    Thread::Current()->set_socketserver(ss_.get());
    socks_.reset(new rtc::SocksProxyServer(
        ss_.get(), kSocksProxyIntAddr, ss_.get(), kSocksProxyExtAddr));
    https_.reset(new rtc::HttpListenServer());
    https_->Listen(kHttpsProxyIntAddr);
  }
  ~ProxyTest() {
    Thread::Current()->set_socketserver(NULL);
  }

  rtc::SocketServer* ss() { return ss_.get(); }

  rtc::ProxyType DetectProxyType(const SocketAddress& address) {
    rtc::ProxyType type;
    AutoDetectProxyRunner* detect = new AutoDetectProxyRunner("unittest/1.0");
    detect->set_proxy(address);
    detect->Run();  
    type = detect->proxy().type;
    detect->Destroy(false);
    return type;
  }

 private:
  rtc::scoped_ptr<rtc::SocketServer> ss_;
  rtc::scoped_ptr<rtc::SocksProxyServer> socks_;
  
  rtc::scoped_ptr<rtc::HttpListenServer> https_;
};


TEST_F(ProxyTest, TestSocks5Connect) {
  rtc::AsyncSocket* socket =
      ss()->CreateAsyncSocket(kSocksProxyIntAddr.family(), SOCK_STREAM);
  rtc::AsyncSocksProxySocket* proxy_socket =
      new rtc::AsyncSocksProxySocket(socket, kSocksProxyIntAddr,
                                           "", rtc::CryptString());
  

  rtc::TestEchoServer server(Thread::Current(),
                                   SocketAddress(INADDR_ANY, 0));

  rtc::AsyncTCPSocket* packet_socket = rtc::AsyncTCPSocket::Create(
      proxy_socket, SocketAddress(INADDR_ANY, 0), server.address());
  EXPECT_TRUE(packet_socket != NULL);
  rtc::TestClient client(packet_socket);

  EXPECT_EQ(Socket::CS_CONNECTING, proxy_socket->GetState());
  EXPECT_TRUE(client.CheckConnected());
  EXPECT_EQ(Socket::CS_CONNECTED, proxy_socket->GetState());
  EXPECT_EQ(server.address(), client.remote_address());
  client.Send("foo", 3);
  EXPECT_TRUE(client.CheckNextPacket("foo", 3, NULL));
  EXPECT_TRUE(client.CheckNoPacket());
}




















TEST_F(ProxyTest, TestAutoDetectSocks5) {
  EXPECT_EQ(rtc::PROXY_SOCKS5, DetectProxyType(kSocksProxyIntAddr));
}









TEST_F(ProxyTest, TestAutoDetectBogus) {
  EXPECT_EQ(rtc::PROXY_UNKNOWN, DetectProxyType(kBogusProxyIntAddr));
}
