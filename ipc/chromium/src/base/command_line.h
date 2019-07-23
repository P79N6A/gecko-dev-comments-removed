















#ifndef BASE_COMMAND_LINE_H_
#define BASE_COMMAND_LINE_H_

#include "build/build_config.h"

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"

class InProcessBrowserTest;

class CommandLine {
 public:
#if defined(OS_WIN)
  
  
  void ParseFromString(const std::wstring& command_line);
#elif defined(OS_POSIX)
  
  CommandLine(int argc, const char* const* argv);
  explicit CommandLine(const std::vector<std::string>& argv);
#endif

  
  
  
  explicit CommandLine(const std::wstring& program);

  
  
  
  
  static void Init(int argc, const char* const* argv);

  
  
  
  
  
  static void Terminate();

  
  
  static const CommandLine* ForCurrentProcess() {
    DCHECK(current_process_commandline_);
    return current_process_commandline_;
  }

#ifdef CHROMIUM_MOZILLA_BUILD
  static bool IsInitialized() {
    return !!current_process_commandline_;
  }
#endif

  
  
  bool HasSwitch(const std::wstring& switch_string) const;

  
  
  
  std::wstring GetSwitchValue(const std::wstring& switch_string) const;

  
  
  std::vector<std::wstring> GetLooseValues() const;

#if defined(OS_WIN)
  
  const std::wstring& command_line_string() const {
    return command_line_string_;
  }
#elif defined(OS_POSIX)
  
  const std::vector<std::string>& argv() const {
    return argv_;
  }
#endif

  
  std::wstring program() const;

  
  
  static std::wstring PrefixedSwitchString(const std::wstring& switch_string);

  
  
  static std::wstring PrefixedSwitchStringWithValue(
                        const std::wstring& switch_string,
                        const std::wstring& value_string);

  
  
  void AppendSwitch(const std::wstring& switch_string);

  
  
  void AppendSwitchWithValue(const std::wstring& switch_string,
                             const std::wstring& value_string);

  
  void AppendLooseValue(const std::wstring& value);

  
  
  void AppendArguments(const CommandLine& other,
                       bool include_program);

  
  
  void PrependWrapper(const std::wstring& wrapper);

 private:
  friend class InProcessBrowserTest;

  CommandLine() {}

  
  static CommandLine* ForCurrentProcessMutable() {
    DCHECK(current_process_commandline_);
    return current_process_commandline_;
  }

  
  
  static CommandLine* current_process_commandline_;

  
  

#if defined(OS_WIN)
  
  std::wstring command_line_string_;

  
  std::wstring program_;

  
  typedef std::wstring StringType;

#elif defined(OS_POSIX)
  
  std::vector<std::string> argv_;

  
  typedef std::string StringType;

  
  void InitFromArgv();
#endif

  
  
  static bool IsSwitch(const StringType& parameter_string,
                       std::string* switch_string,
                       StringType* switch_value);

  
  std::map<std::string, StringType> switches_;

  
  std::vector<StringType> loose_values_;

  
  
  
  
  
};

#endif  
