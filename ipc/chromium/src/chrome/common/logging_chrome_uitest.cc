



#include <string>
#include <windows.h>

#include "base/command_line.h"
#include "base/basictypes.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/env_vars.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/test/ui/ui_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
  class ChromeLoggingTest : public testing::Test {
   public:
    
    
    void SaveEnvironmentVariable(std::wstring new_value) {
      unsigned status = GetEnvironmentVariable(env_vars::kLogFileName,
                                               environment_filename_,
                                               MAX_PATH);
      if (!status) {
        wcscpy_s(environment_filename_, L"");
      }

      SetEnvironmentVariable(env_vars::kLogFileName, new_value.c_str());
    }

    
    
    void RestoreEnvironmentVariable() {
      SetEnvironmentVariable(env_vars::kLogFileName, environment_filename_);
    }

   private:
    wchar_t environment_filename_[MAX_PATH];  
  };
};


TEST_F(ChromeLoggingTest, LogFileName) {
  SaveEnvironmentVariable(std::wstring());

  std::wstring filename = logging::GetLogFileName();
  ASSERT_NE(std::wstring::npos, filename.find(L"chrome_debug.log"));

  RestoreEnvironmentVariable();
}


TEST_F(ChromeLoggingTest, EnvironmentLogFileName) {
  SaveEnvironmentVariable(std::wstring(L"test value"));

  std::wstring filename = logging::GetLogFileName();
  ASSERT_EQ(std::wstring(L"test value"), filename);

  RestoreEnvironmentVariable();
}

#ifndef NDEBUG  

class AssertionTest : public UITest {
 protected:
 AssertionTest() : UITest()
  {
    
    wait_for_initial_loads_ = false;

    
    
    
    launch_arguments_.AppendSwitch(switches::kRendererAssertTest);
  }
};


TEST_F(AssertionTest, Assertion) {
  if (UITest::in_process_renderer()) {
    
    expected_errors_ = 0;
    expected_crashes_ = 0;
  } else {
    expected_errors_ = 1;
    expected_crashes_ = 1;
  }
}
#endif  


class RendererCrashTest : public UITest {
 protected:
 RendererCrashTest() : UITest()
  {
    
    wait_for_initial_loads_ = false;

    launch_arguments_.AppendSwitch(switches::kRendererCrashTest);
  }
};


TEST_F(RendererCrashTest, Crash) {
  if (UITest::in_process_renderer()) {
    
    expected_crashes_ = 0;
  } else {
    
    PlatformThread::Sleep(5000);
    expected_crashes_ = 1;
  }
}


class BrowserCrashTest : public UITest {
 protected:
 BrowserCrashTest() : UITest()
  {
    
    wait_for_initial_loads_ = false;

    launch_arguments_.AppendSwitch(switches::kBrowserCrashTest);
  }
};



TEST_F(BrowserCrashTest, DISABLED_Crash) {
  
  PlatformThread::Sleep(5000);
  expected_crashes_ = 1;
}
