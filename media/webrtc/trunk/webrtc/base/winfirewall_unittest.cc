









#include "webrtc/base/gunit.h"
#include "webrtc/base/winfirewall.h"

#include <objbase.h>

namespace rtc {

TEST(WinFirewallTest, ReadStatus) {
  ::CoInitialize(NULL);
  WinFirewall fw;
  HRESULT hr;
  bool authorized;

  EXPECT_FALSE(fw.QueryAuthorized("bogus.exe", &authorized));
  EXPECT_TRUE(fw.Initialize(&hr));
  EXPECT_EQ(S_OK, hr);

  EXPECT_TRUE(fw.QueryAuthorized("bogus.exe", &authorized));

  
  
  

  fw.Shutdown();
  EXPECT_FALSE(fw.QueryAuthorized("bogus.exe", &authorized));

  ::CoUninitialize();
}

}  
