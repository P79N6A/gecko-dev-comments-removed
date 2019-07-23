



#ifndef BASE_TEST_SUITE_H_
#define BASE_TEST_SUITE_H_





#include "base/at_exit.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/debug_on_start.h"
#include "base/file_path.h"
#include "base/icu_util.h"
#include "base/logging.h"
#include "base/multiprocess_test.h"
#include "base/scoped_nsautorelease_pool.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_LINUX)
#include <gtk/gtk.h>
#endif

class TestSuite {
 public:
  TestSuite(int argc, char** argv) {
    base::EnableTerminationOnHeapCorruption();
    CommandLine::Init(argc, argv);
    testing::InitGoogleTest(&argc, argv);
#if defined(OS_LINUX)
    gtk_init_check(&argc, &argv);
#endif
    
    
  }

  virtual ~TestSuite() {
    CommandLine::Terminate();
  }

  
  
  int Run() {
    base::ScopedNSAutoreleasePool scoped_pool;

    Initialize();
    std::wstring client_func =
        CommandLine::ForCurrentProcess()->GetSwitchValue(kRunClientProcess);
    
    if (!client_func.empty()) {
      
      std::string func_name(client_func.begin(), client_func.end());

      return multi_process_function_list::InvokeChildProcessTest(func_name);
    }
    int result = RUN_ALL_TESTS();

    
    
    
    scoped_pool.Recycle();

    Shutdown();

    return result;
  }

 protected:
  
  static void UnitTestAssertHandler(const std::string& str) {
    FAIL() << str;
  }

#if defined(OS_WIN)
  
  virtual void SuppressErrorDialogs() {
    UINT new_flags = SEM_FAILCRITICALERRORS |
                     SEM_NOGPFAULTERRORBOX |
                     SEM_NOOPENFILEERRORBOX;

    
    
    UINT existing_flags = SetErrorMode(new_flags);
    SetErrorMode(existing_flags | new_flags);
  }
#endif

  
  

  virtual void Initialize() {
    
    FilePath exe;
    PathService::Get(base::FILE_EXE, &exe);
    FilePath log_filename = exe.ReplaceExtension(FILE_PATH_LITERAL("log"));
    logging::InitLogging(log_filename.value().c_str(),
                         logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                         logging::LOCK_LOG_FILE,
                         logging::DELETE_OLD_LOG_FILE);
    
    
    logging::SetLogItems(true, true, true, true);

#if defined(OS_WIN)
    
    if (!IsDebuggerPresent() &&
        !CommandLine::ForCurrentProcess()->HasSwitch(L"show-error-dialogs")) {
      SuppressErrorDialogs();
#if !defined(PURIFY)
      
      
      logging::SetLogAssertHandler(UnitTestAssertHandler);
#endif
    }
#endif

    icu_util::Initialize();
  }

  virtual void Shutdown() {
  }

  
  
  base::AtExitManager at_exit_manager_;
};

#endif  
