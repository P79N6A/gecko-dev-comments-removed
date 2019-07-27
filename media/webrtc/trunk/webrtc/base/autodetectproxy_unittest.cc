









#include "webrtc/base/autodetectproxy.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/httpcommon.h"
#include "webrtc/base/httpcommon-inl.h"
#include "webrtc/test/testsupport/gtest_disable.h"

namespace rtc {

static const char kUserAgent[] = "";
static const char kPath[] = "/";
static const char kHost[] = "relay.google.com";
static const uint16 kPort = 443;
static const bool kSecure = true;









static const int kTimeoutMs = 10000;

class AutoDetectProxyTest : public testing::Test, public sigslot::has_slots<> {
 public:
  AutoDetectProxyTest() : auto_detect_proxy_(NULL), done_(false) {}

 protected:
  bool Create(const std::string &user_agent,
              const std::string &path,
              const std::string &host,
              uint16 port,
              bool secure,
              bool startnow) {
    auto_detect_proxy_ = new AutoDetectProxy(user_agent);
    EXPECT_TRUE(auto_detect_proxy_ != NULL);
    if (!auto_detect_proxy_) {
      return false;
    }
    Url<char> host_url(path, host, port);
    host_url.set_secure(secure);
    auto_detect_proxy_->set_server_url(host_url.url());
    auto_detect_proxy_->SignalWorkDone.connect(
        this,
        &AutoDetectProxyTest::OnWorkDone);
    if (startnow) {
      auto_detect_proxy_->Start();
    }
    return true;
  }

  bool Run(int timeout_ms) {
    EXPECT_TRUE_WAIT(done_, timeout_ms);
    return done_;
  }

  void SetProxy(const SocketAddress& proxy) {
    auto_detect_proxy_->set_proxy(proxy);
  }

  void Start() {
    auto_detect_proxy_->Start();
  }

  void TestCopesWithProxy(const SocketAddress& proxy) {
    
    ASSERT_TRUE(Create(kUserAgent,
                       kPath,
                       kHost,
                       kPort,
                       kSecure,
                       false));
    SetProxy(proxy);
    Start();
    ASSERT_TRUE(Run(kTimeoutMs));
  }

 private:
  void OnWorkDone(rtc::SignalThread *thread) {
    AutoDetectProxy *auto_detect_proxy =
        static_cast<rtc::AutoDetectProxy *>(thread);
    EXPECT_TRUE(auto_detect_proxy == auto_detect_proxy_);
    auto_detect_proxy_ = NULL;
    auto_detect_proxy->Release();
    done_ = true;
  }

  AutoDetectProxy *auto_detect_proxy_;
  bool done_;
};

TEST_F(AutoDetectProxyTest, TestDetectUnresolvedProxy) {
  TestCopesWithProxy(rtc::SocketAddress("localhost", 9999));
}

TEST_F(AutoDetectProxyTest, TestDetectUnresolvableProxy) {
  TestCopesWithProxy(rtc::SocketAddress("invalid", 9999));
}

TEST_F(AutoDetectProxyTest, TestDetectIPv6Proxy) {
  TestCopesWithProxy(rtc::SocketAddress("::1", 9999));
}

TEST_F(AutoDetectProxyTest, TestDetectIPv4Proxy) {
  TestCopesWithProxy(rtc::SocketAddress("127.0.0.1", 9999));
}




TEST_F(AutoDetectProxyTest, TestProxyDetection) {
  ASSERT_TRUE(Create(kUserAgent,
                     kPath,
                     kHost,
                     kPort,
                     kSecure,
                     true));
  ASSERT_TRUE(Run(kTimeoutMs));
}

}  
