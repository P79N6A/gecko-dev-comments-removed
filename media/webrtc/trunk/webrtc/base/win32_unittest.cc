









#include <string>

#include "webrtc/base/gunit.h"
#include "webrtc/base/nethelpers.h"
#include "webrtc/base/win32.h"
#include "webrtc/base/winping.h"

#if !defined(WEBRTC_WIN)
#error Only for Windows
#endif

namespace rtc {

class Win32Test : public testing::Test {
 public:
  Win32Test() {
  }
};

TEST_F(Win32Test, FileTimeToUInt64Test) {
  FILETIME ft;
  ft.dwHighDateTime = 0xBAADF00D;
  ft.dwLowDateTime = 0xFEED3456;

  uint64 expected = 0xBAADF00DFEED3456;
  EXPECT_EQ(expected, ToUInt64(ft));
}

TEST_F(Win32Test, WinPingTest) {
  WinPing ping;
  ASSERT_TRUE(ping.IsValid());

  
  WinPing::PingResult result = ping.Ping(IPAddress(INADDR_LOOPBACK), 20, 50, 1,
                                         false);
  ASSERT_EQ(WinPing::PING_SUCCESS, result);
  if (HasIPv6Enabled()) {
    WinPing::PingResult v6result = ping.Ping(IPAddress(in6addr_loopback), 20,
                                             50, 1, false);
    ASSERT_EQ(WinPing::PING_SUCCESS, v6result);
  }

  
  ASSERT_EQ(WinPing::PING_INVALID_PARAMS, ping.Ping(
            IPAddress(INADDR_LOOPBACK), 0, 50, 1, false));
  ASSERT_EQ(WinPing::PING_INVALID_PARAMS, ping.Ping(
            IPAddress(INADDR_LOOPBACK), 20, 0, 1, false));
  ASSERT_EQ(WinPing::PING_INVALID_PARAMS, ping.Ping(
            IPAddress(INADDR_LOOPBACK), 20, 50, 0, false));
}

}  
