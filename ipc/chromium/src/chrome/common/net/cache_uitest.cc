



#include <string>

#include "base/platform_thread.h"
#include "base/string_util.h"
#include "chrome/test/ui/ui_test.h"
#include "chrome/test/automation/automation_proxy.h"
#include "chrome/test/automation/browser_proxy.h"
#include "chrome/test/automation/tab_proxy.h"
#include "net/url_request/url_request_unittest.h"

















class CacheTest : public UITest {
  protected:

    
    
    
    
    
    void RunCacheTest(const std::wstring &url,
                      bool expect_new_tab_cached,
                      bool expect_delayed_reload);

  private:
    
    static const int kWaitForCacheUpdateMsec = 2000;
    static const int kCacheWaitMultiplier = 4;  

    
    
    
    void GetNewTab(AutomationProxy* automationProxy, const GURL& tab_url);
};


void CacheTest::RunCacheTest(const std::wstring &url,
                             bool expect_new_tab_cached,
                             bool expect_delayed_reload) {
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"chrome/test/data", NULL);
  ASSERT_TRUE(NULL != server.get());
  GURL test_page(server->TestServerPageW(url));

  NavigateToURL(test_page);
  std::wstring original_time = GetActiveTabTitle();

  PlatformThread::Sleep(kWaitForCacheUpdateMsec);

  GetNewTab(automation(), test_page);
  std::wstring revisit_time = GetActiveTabTitle();

  if (expect_new_tab_cached) {
    EXPECT_EQ(original_time, revisit_time);
  }else {
    EXPECT_NE(original_time, revisit_time);
  }

  PlatformThread::Sleep(kWaitForCacheUpdateMsec);

  
  NavigateToURL(test_page);
  std::wstring reload_time = GetActiveTabTitle();

  EXPECT_NE(revisit_time, reload_time);

  if (expect_delayed_reload) {
    PlatformThread::Sleep(kWaitForCacheUpdateMsec * kCacheWaitMultiplier);

    GetNewTab(automation(), test_page);
    revisit_time = GetActiveTabTitle();

    EXPECT_NE(reload_time, revisit_time);
  }
}


void CacheTest::GetNewTab(AutomationProxy* automationProxy,
                          const GURL& tab_url) {
  scoped_ptr<BrowserProxy> window_proxy(automationProxy->GetBrowserWindow(0));
  ASSERT_TRUE(window_proxy.get());
  ASSERT_TRUE(window_proxy->AppendTab(tab_url));
}



TEST_F(CacheTest, NoCacheMaxAge) {
  RunCacheTest(L"nocachetime/maxage", false, false);
}



TEST_F(CacheTest, NoCache) {
  RunCacheTest(L"nocachetime", false, false);
}



TEST_F(CacheTest, Cache) {
  RunCacheTest(L"cachetime", true, false);
}



TEST_F(CacheTest, Expires) {
  RunCacheTest(L"cache/expires", true, false);
}



TEST_F(CacheTest, ProxyRevalidate) {
  RunCacheTest(L"cache/proxy-revalidate", true, false);
}



TEST_F(CacheTest, Private) {
  RunCacheTest(L"cache/private", true, true);
}



TEST_F(CacheTest, Public) {
  RunCacheTest(L"cache/public", true, true);
}



TEST_F(CacheTest, SMaxAge) {
  RunCacheTest(L"cache/s-maxage", false, false);
}



TEST_F(CacheTest, MustRevalidate) {
  RunCacheTest(L"cache/must-revalidate", false, false);
}



TEST_F(CacheTest, MustRevalidateMaxAge) {
  RunCacheTest(L"cache/must-revalidate/max-age", false, false);
}



TEST_F(CacheTest, NoStore) {
  RunCacheTest(L"cache/no-store", false, false);
}



TEST_F(CacheTest, NoStoreMaxAge) {
  RunCacheTest(L"cache/no-store/max-age", false, false);
}



TEST_F(CacheTest, NoTransform) {
  RunCacheTest(L"cache/no-transform", false, false);
}
