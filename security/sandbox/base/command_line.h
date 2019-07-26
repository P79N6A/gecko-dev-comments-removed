













#ifndef BASE_COMMAND_LINE_H_
#define BASE_COMMAND_LINE_H_

#include <stddef.h>
#include <map>
#include <string>
#include <vector>

#include "base/base_export.h"
#include "build/build_config.h"

namespace base {
class FilePath;
}

class BASE_EXPORT CommandLine {
 public:
#if defined(OS_WIN)
  
  typedef std::wstring StringType;
#elif defined(OS_POSIX)
  typedef std::string StringType;
#endif

  typedef StringType::value_type CharType;
  typedef std::vector<StringType> StringVector;
  typedef std::map<std::string, StringType> SwitchMap;

  
  enum NoProgram { NO_PROGRAM };
  explicit CommandLine(NoProgram no_program);

  
  explicit CommandLine(const base::FilePath& program);

  
  CommandLine(int argc, const CharType* const* argv);
  explicit CommandLine(const StringVector& argv);

  ~CommandLine();

  
  
  
  
  
  
  static bool Init(int argc, const char* const* argv);

  
  
  
  
  static void Reset();

  
  
  
  static CommandLine* ForCurrentProcess();

  
  static bool InitializedForCurrentProcess();

#if defined(OS_WIN)
  static CommandLine FromString(const std::wstring& command_line);
#endif

  
  void InitFromArgv(int argc, const CharType* const* argv);
  void InitFromArgv(const StringVector& argv);

  
  
  
  StringType GetCommandLineString() const;

  
  
  
  StringType GetArgumentsString() const;

  
  const StringVector& argv() const { return argv_; }

  
  base::FilePath GetProgram() const;
  void SetProgram(const base::FilePath& program);

  
  
  bool HasSwitch(const std::string& switch_string) const;

  
  
  std::string GetSwitchValueASCII(const std::string& switch_string) const;
  base::FilePath GetSwitchValuePath(const std::string& switch_string) const;
  StringType GetSwitchValueNative(const std::string& switch_string) const;

  
  const SwitchMap& GetSwitches() const { return switches_; }

  
  
  void AppendSwitch(const std::string& switch_string);
  void AppendSwitchPath(const std::string& switch_string,
                        const base::FilePath& path);
  void AppendSwitchNative(const std::string& switch_string,
                          const StringType& value);
  void AppendSwitchASCII(const std::string& switch_string,
                         const std::string& value);

  
  
  void CopySwitchesFrom(const CommandLine& source,
                        const char* const switches[],
                        size_t count);

  
  StringVector GetArgs() const;

  
  
  
  
  void AppendArg(const std::string& value);
  void AppendArgPath(const base::FilePath& value);
  void AppendArgNative(const StringType& value);

  
  
  void AppendArguments(const CommandLine& other, bool include_program);

  
  
  void PrependWrapper(const StringType& wrapper);

#if defined(OS_WIN)
  
  
  void ParseFromString(const std::wstring& command_line);
#endif

 private:
  
  CommandLine();
  
  
  
  

  
  static CommandLine* current_process_commandline_;

  
  StringVector argv_;

  
  SwitchMap switches_;

  
  size_t begin_args_;
};

#endif  
