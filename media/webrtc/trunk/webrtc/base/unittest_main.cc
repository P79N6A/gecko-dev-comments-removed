











#if defined(WEBRTC_WIN)
#include <crtdbg.h>
#endif

#include "webrtc/base/flags.h"
#include "webrtc/base/fileutils.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"

DEFINE_bool(help, false, "prints this message");
DEFINE_string(log, "", "logging options to use");
#if defined(WEBRTC_WIN)
DEFINE_int(crt_break_alloc, -1, "memory allocation to break on");
DEFINE_bool(default_error_handlers, false,
            "leave the default exception/dbg handler functions in place");

void TestInvalidParameterHandler(const wchar_t* expression,
                                 const wchar_t* function,
                                 const wchar_t* file,
                                 unsigned int line,
                                 uintptr_t pReserved) {
  LOG(LS_ERROR) << "InvalidParameter Handler called.  Exiting.";
  LOG(LS_ERROR) << expression << std::endl << function << std::endl << file
                << std::endl << line;
  exit(1);
}
void TestPureCallHandler() {
  LOG(LS_ERROR) << "Purecall Handler called.  Exiting.";
  exit(1);
}
int TestCrtReportHandler(int report_type, char* msg, int* retval) {
    LOG(LS_ERROR) << "CrtReport Handler called...";
    LOG(LS_ERROR) << msg;
  if (report_type == _CRT_ASSERT) {
    exit(1);
  } else {
    *retval = 0;
    return TRUE;
  }
}
#endif  

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, false);
  if (FLAG_help) {
    rtc::FlagList::Print(NULL, false);
    return 0;
  }

#if defined(WEBRTC_WIN)
  if (!FLAG_default_error_handlers) {
    
    _set_invalid_parameter_handler(TestInvalidParameterHandler);
    _set_purecall_handler(TestPureCallHandler);
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, TestCrtReportHandler);
  }

#ifdef _DEBUG  
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |_CRTDBG_LEAK_CHECK_DF);
  if (FLAG_crt_break_alloc >= 0) {
    _crtBreakAlloc = FLAG_crt_break_alloc;
  }
#endif  
#endif  

  rtc::Filesystem::SetOrganizationName("google");
  rtc::Filesystem::SetApplicationName("unittest");

  
  rtc::LogMessage::LogTimestamps();
  if (*FLAG_log != '\0') {
    rtc::LogMessage::ConfigureLogging(FLAG_log, "unittest.log");
  }

  
  rtc::InitializeSSL();

  int res = RUN_ALL_TESTS();

  rtc::CleanupSSL();

  
  rtc::LogMessage::ConfigureLogging("", "");

#if defined(WEBRTC_WIN)
  
  
  if (!FLAG_default_error_handlers)
    _CrtSetReportHook2(_CRT_RPTHOOK_REMOVE, TestCrtReportHandler);
#endif

  return res;
}
