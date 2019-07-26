




























#include <windows.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/unittests/exception_handler_test.h"

namespace {

const char kFoo[] = "foo";
const char kBar[] = "bar";

const char kStartOfLine[] = "^";
const char kEndOfLine[] = "$";

const char kFilterReturnsTrue[] = "filter_returns_true";
const char kFilterReturnsFalse[] = "filter_returns_false";

const char kCallbackReturnsTrue[] = "callback_returns_true";
const char kCallbackReturnsFalse[] = "callback_returns_false";

bool DoesPathExist(const wchar_t *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return true;
}












template <bool filter_return_value>
bool CrashHandlerFilter(void* context,
                        EXCEPTION_POINTERS* exinfo,
                        MDRawAssertionInfo* assertion) {
  if (filter_return_value) {
    fprintf(stderr, kFilterReturnsTrue);
  } else {
    fprintf(stderr, kFilterReturnsFalse);
  }
  fflush(stderr);

  return filter_return_value;
}

























template <bool callback_return_value>
bool MinidumpWrittenCallback(const wchar_t* dump_path,
                             const wchar_t* minidump_id,
                             void* context,
                             EXCEPTION_POINTERS* exinfo,
                             MDRawAssertionInfo* assertion,
                             bool succeeded) {
  bool rv = false;
  if (callback_return_value &&
      succeeded &&
      DoesPathExist(dump_path)) {
    rv = true;
    fprintf(stderr, kCallbackReturnsTrue);
  } else {
    fprintf(stderr, kCallbackReturnsFalse);
  }
  fflush(stderr);

  return rv;
}


void DoCrash(const char *message) {
  if (message) {
    fprintf(stderr, "%s", message);
    fflush(stderr);
  }
  int *i = NULL;
  (*i)++;

  ASSERT_TRUE(false);
}

void InstallExceptionHandlerAndCrash(bool install_filter,
                                     bool filter_return_value,
                                     bool install_callback,
                                     bool callback_return_value) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      install_filter ? 
        (filter_return_value ?
          &CrashHandlerFilter<true> :
          &CrashHandlerFilter<false>) :
        NULL,
      install_callback ?
        (callback_return_value ?
          &MinidumpWrittenCallback<true> :
          &MinidumpWrittenCallback<false>) :
        NULL,
      NULL,  
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  DoCrash(NULL);
}

TEST(AssertDeathSanity, Simple) {
  ASSERT_DEATH(DoCrash(NULL), "");
}

TEST(AssertDeathSanity, Regex) {
  ASSERT_DEATH(DoCrash(kFoo),
    std::string(kStartOfLine) +
      std::string(kFoo) +
      std::string(kEndOfLine));

  ASSERT_DEATH(DoCrash(kBar), 
    std::string(kStartOfLine) +
      std::string(kBar) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterTrue_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    
                                    true,    
                                    false,   
                                    false),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterTrue_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    
                                    true,    
                                    true,    
                                    false),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +
      std::string(kCallbackReturnsFalse) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterFalse_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    
                                    false,   
                                    false,   
                                    false),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +
      std::string(kEndOfLine));
}


TEST(ExceptionHandlerCallbacks, FilterFalse_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    
                                    false,   
                                    true,    
                                    false),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, No_Filter_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(false,   
                                    true,    
                                    false,   
                                    false),  
    std::string(kStartOfLine) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, No_Filter_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(false,   
                                    true,    
                                    true,    
                                    false),  
    std::string(kStartOfLine) +
      std::string(kCallbackReturnsFalse) +
      std::string(kEndOfLine));
}


TEST(ExceptionHandlerNesting, Skip_From_Inner_Filter) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<false>,
      NULL,  
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,   
                                    false,  
                                    true,   
                                    true),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +    
      std::string(kFilterReturnsTrue) +     
      std::string(kCallbackReturnsFalse) +  
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerNesting, Skip_From_Inner_Callback) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<false>,
      NULL,  
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    
                                    true,    
                                    true,    
                                    false),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +      
      std::string(kCallbackReturnsFalse) +   
      std::string(kFilterReturnsTrue) +      
      std::string(kCallbackReturnsFalse) +   
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerNesting, Handled_By_Inner_Handler) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<true>,
      NULL,  
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,   
                                    true,   
                                    true,   
                                    true),  
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +    
      std::string(kCallbackReturnsTrue) +  
      std::string(kEndOfLine));
}

}  
