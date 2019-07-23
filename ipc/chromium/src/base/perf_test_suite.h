



#ifndef BASE_PERF_TEST_SUITE_H_
#define BASE_PERF_TEST_SUITE_H_

#include "base/command_line.h"
#include "base/debug_util.h"
#include "base/file_path.h"
#include "base/perftimer.h"
#include "base/process_util.h"
#include "base/string_util.h"
#include "base/test_suite.h"

class PerfTestSuite : public TestSuite {
 public:
  PerfTestSuite(int argc, char** argv) : TestSuite(argc, argv) {
  }

  virtual void Initialize() {
    TestSuite::Initialize();

    
    FilePath log_path;
    std::wstring log_file =
        CommandLine::ForCurrentProcess()->GetSwitchValue(L"log-file");
    if (log_file.empty()) {
      FilePath exe;
      PathService::Get(base::FILE_EXE, &exe);
      log_path = exe.ReplaceExtension(FILE_PATH_LITERAL("log"));
      log_path = log_path.InsertBeforeExtension(FILE_PATH_LITERAL("_perf"));
    } else {
      log_path = FilePath::FromWStringHack(log_file);
    }
    ASSERT_TRUE(InitPerfLog(log_path));

    
    
    if (!DebugUtil::BeingDebugged())
      base::RaiseProcessToHighPriority();
  }

  virtual void Shutdown() {
    TestSuite::Shutdown();

    FinalizePerfLog();
  }
};

#endif  
