




#include "GTestRunner.h"
#include "gtest/gtest.h"
#include "mozilla/Attributes.h"
#include "mozilla/NullPtr.h"
#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif
#include "testing/TestHarness.h"
#include "prenv.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

namespace mozilla {


class MozillaPrinter : public EmptyTestEventListener
{
public:
  virtual void OnTestProgramStart(const UnitTest& ) MOZ_OVERRIDE {
    printf("TEST-INFO | GTest unit test starting\n");
  }
  virtual void OnTestProgramEnd(const UnitTest& aUnitTest) MOZ_OVERRIDE {
    printf("TEST-%s | GTest unit test: %s\n",
           aUnitTest.Passed() ? "PASS" : "UNEXPECTED-FAIL",
           aUnitTest.Passed() ? "passed" : "failed");
  }
  virtual void OnTestStart(const TestInfo& aTestInfo) MOZ_OVERRIDE {
    mTestInfo = &aTestInfo;
    printf("TEST-START | %s.%s\n",
        mTestInfo->test_case_name(), mTestInfo->name());
  }
  virtual void OnTestPartResult(const TestPartResult& aTestPartResult) MOZ_OVERRIDE {
    printf("TEST-%s | %s.%s | %s @ %s:%i\n",
           !aTestPartResult.failed() ? "PASS" : "UNEXPECTED-FAIL",
           mTestInfo ? mTestInfo->test_case_name() : "?", mTestInfo ? mTestInfo->name() : "?",
           aTestPartResult.summary(),
           aTestPartResult.file_name(), aTestPartResult.line_number());
  }
  virtual void OnTestEnd(const TestInfo& aTestInfo) MOZ_OVERRIDE {
    printf("TEST-%s | %s.%s | test completed (time: %llims)\n",
           aTestInfo.result()->Passed() ? "PASS": "UNEXPECTED-FAIL",
           aTestInfo.test_case_name(), aTestInfo.name(),
           aTestInfo.result()->elapsed_time());
    MOZ_ASSERT(&aTestInfo == mTestInfo);
    mTestInfo = nullptr;
  }

  const TestInfo* mTestInfo;
};

static void ReplaceGTestLogger()
{
  
  
  
  UnitTest& unitTest = *UnitTest::GetInstance();
  TestEventListeners& listeners = unitTest.listeners();
  delete listeners.Release(listeners.default_result_printer());

  listeners.Append(new MozillaPrinter);
}

int RunGTestFunc()
{
  int c = 0;
  InitGoogleTest(&c, static_cast<char**>(nullptr));

  if (getenv("MOZ_TBPL_PARSER")) {
    ReplaceGTestLogger();
  }

  PR_SetEnv("XPCOM_DEBUG_BREAK=stack-and-abort");

  ScopedXPCOM xpcom("AsyncPanZoomController");

#ifdef MOZ_CRASHREPORTER
  nsCOMPtr<nsICrashReporter> crashreporter;
  char *crashreporterStr = PR_GetEnv("MOZ_CRASHREPORTER");
  if (crashreporterStr && !strcmp(crashreporterStr, "1")) {
    
    
    crashreporter = do_GetService("@mozilla.org/toolkit/crash-reporter;1");
    if (crashreporter) {
      std::cerr << "Setting up crash reporting" << std::endl;

      nsCOMPtr<nsIProperties> dirsvc =
          do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
      nsCOMPtr<nsIFile> cwd;
      nsresult rv = dirsvc->Get(NS_OS_CURRENT_WORKING_DIR,
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(cwd));
      MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
      crashreporter->SetEnabled(true);
      crashreporter->SetMinidumpPath(cwd);
    }
  }
#endif

  return RUN_ALL_TESTS();
}





class _InitRunGTest {
public:
  _InitRunGTest() {
    RunGTest = RunGTestFunc;
  }
} InitRunGTest;

}
