






























#include "gmock/gmock.h"
#include "gmock/internal/gmock-port.h"

namespace testing {




GMOCK_DEFINE_bool_(catch_leaked_mocks, true,
                   "true iff Google Mock should report leaked mock objects "
                   "as failures.");

GMOCK_DEFINE_string_(verbose, internal::kWarningVerbosity,
                     "Controls how verbose Google Mock's output is."
                     "  Valid values:\n"
                     "  info    - prints all messages.\n"
                     "  warning - prints warnings and errors.\n"
                     "  error   - prints errors only.");

namespace internal {






static const char* ParseGoogleMockFlagValue(const char* str,
                                            const char* flag,
                                            bool def_optional) {
  
  if (str == NULL || flag == NULL) return NULL;

  
  const String flag_str = String::Format("--gmock_%s", flag);
  const size_t flag_len = flag_str.length();
  if (strncmp(str, flag_str.c_str(), flag_len) != 0) return NULL;

  
  const char* flag_end = str + flag_len;

  
  if (def_optional && (flag_end[0] == '\0')) {
    return flag_end;
  }

  
  
  
  if (flag_end[0] != '=') return NULL;

  
  return flag_end + 1;
}






static bool ParseGoogleMockBoolFlag(const char* str, const char* flag,
                                    bool* value) {
  
  const char* const value_str = ParseGoogleMockFlagValue(str, flag, true);

  
  if (value_str == NULL) return false;

  
  *value = !(*value_str == '0' || *value_str == 'f' || *value_str == 'F');
  return true;
}






static bool ParseGoogleMockStringFlag(const char* str, const char* flag,
                                      String* value) {
  
  const char* const value_str = ParseGoogleMockFlagValue(str, flag, false);

  
  if (value_str == NULL) return false;

  
  *value = value_str;
  return true;
}





template <typename CharType>
void InitGoogleMockImpl(int* argc, CharType** argv) {
  
  
  InitGoogleTest(argc, argv);
  if (*argc <= 0) return;

  for (int i = 1; i != *argc; i++) {
    const String arg_string = StreamableToString(argv[i]);
    const char* const arg = arg_string.c_str();

    
    if (ParseGoogleMockBoolFlag(arg, "catch_leaked_mocks",
                                &GMOCK_FLAG(catch_leaked_mocks)) ||
        ParseGoogleMockStringFlag(arg, "verbose", &GMOCK_FLAG(verbose))) {
      
      
      
      
      for (int j = i; j != *argc; j++) {
        argv[j] = argv[j + 1];
      }

      
      (*argc)--;

      
      
      i--;
    }
  }
}

}  












void InitGoogleMock(int* argc, char** argv) {
  internal::InitGoogleMockImpl(argc, argv);
}



void InitGoogleMock(int* argc, wchar_t** argv) {
  internal::InitGoogleMockImpl(argc, argv);
}

}  
