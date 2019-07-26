



#include "base/command_line.h"

#include <algorithm>
#include <ostream>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>
#endif

using base::FilePath;

CommandLine* CommandLine::current_process_commandline_ = NULL;

namespace {
const CommandLine::CharType kSwitchTerminator[] = FILE_PATH_LITERAL("--");
const CommandLine::CharType kSwitchValueSeparator[] = FILE_PATH_LITERAL("=");


#if defined(OS_WIN)
const CommandLine::CharType* const kSwitchPrefixes[] = {L"--", L"-", L"/"};
#elif defined(OS_POSIX)

const CommandLine::CharType* const kSwitchPrefixes[] = {"--", "-"};
#endif

size_t GetSwitchPrefixLength(const CommandLine::StringType& string) {
  for (size_t i = 0; i < arraysize(kSwitchPrefixes); ++i) {
    CommandLine::StringType prefix(kSwitchPrefixes[i]);
    if (string.compare(0, prefix.length(), prefix) == 0)
      return prefix.length();
  }
  return 0;
}



bool IsSwitch(const CommandLine::StringType& string,
              CommandLine::StringType* switch_string,
              CommandLine::StringType* switch_value) {
  switch_string->clear();
  switch_value->clear();
  size_t prefix_length = GetSwitchPrefixLength(string);
  if (prefix_length == 0 || prefix_length == string.length())
    return false;

  const size_t equals_position = string.find(kSwitchValueSeparator);
  *switch_string = string.substr(0, equals_position);
  if (equals_position != CommandLine::StringType::npos)
    *switch_value = string.substr(equals_position + 1);
  return true;
}


void AppendSwitchesAndArguments(CommandLine& command_line,
                                const CommandLine::StringVector& argv) {
  bool parse_switches = true;
  for (size_t i = 1; i < argv.size(); ++i) {
    CommandLine::StringType arg = argv[i];
    TrimWhitespace(arg, TRIM_ALL, &arg);

    CommandLine::StringType switch_string;
    CommandLine::StringType switch_value;
    parse_switches &= (arg != kSwitchTerminator);
    if (parse_switches && IsSwitch(arg, &switch_string, &switch_value)) {
#if defined(OS_WIN)
      command_line.AppendSwitchNative(WideToASCII(switch_string), switch_value);
#elif defined(OS_POSIX)
      command_line.AppendSwitchNative(switch_string, switch_value);
#endif
    } else {
      command_line.AppendArgNative(arg);
    }
  }
}


std::string LowerASCIIOnWindows(const std::string& string) {
#if defined(OS_WIN)
  return StringToLowerASCII(string);
#elif defined(OS_POSIX)
  return string;
#endif
}


#if defined(OS_WIN)

std::wstring QuoteForCommandLineToArgvW(const std::wstring& arg) {
  
  
  if (arg.find_first_of(L" \\\"") == std::wstring::npos) {
    
    return arg;
  }

  std::wstring out;
  out.push_back(L'"');
  for (size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\\') {
      
      size_t start = i, end = start + 1;
      for (; end < arg.size() && arg[end] == '\\'; ++end)
        ;
      size_t backslash_count = end - start;

      
      
      
      if (end == arg.size() || arg[end] == '"') {
        
        backslash_count *= 2;
      }
      for (size_t j = 0; j < backslash_count; ++j)
        out.push_back('\\');

      
      i = end - 1;
    } else if (arg[i] == '"') {
      out.push_back('\\');
      out.push_back('"');
    } else {
      out.push_back(arg[i]);
    }
  }
  out.push_back('"');

  return out;
}
#endif

}  

CommandLine::CommandLine(NoProgram no_program)
    : argv_(1),
      begin_args_(1) {
}

CommandLine::CommandLine(const FilePath& program)
    : argv_(1),
      begin_args_(1) {
  SetProgram(program);
}

CommandLine::CommandLine(int argc, const CommandLine::CharType* const* argv)
    : argv_(1),
      begin_args_(1) {
  InitFromArgv(argc, argv);
}

CommandLine::CommandLine(const StringVector& argv)
    : argv_(1),
      begin_args_(1) {
  InitFromArgv(argv);
}

CommandLine::~CommandLine() {
}


bool CommandLine::Init(int argc, const char* const* argv) {
  if (current_process_commandline_) {
    
    
    
    return false;
  }

  current_process_commandline_ = new CommandLine(NO_PROGRAM);
#if defined(OS_WIN)
  current_process_commandline_->ParseFromString(::GetCommandLineW());
#elif defined(OS_POSIX)
  current_process_commandline_->InitFromArgv(argc, argv);
#endif

  return true;
}


void CommandLine::Reset() {
  DCHECK(current_process_commandline_);
  delete current_process_commandline_;
  current_process_commandline_ = NULL;
}


CommandLine* CommandLine::ForCurrentProcess() {
  DCHECK(current_process_commandline_);
  return current_process_commandline_;
}


bool CommandLine::InitializedForCurrentProcess() {
  return !!current_process_commandline_;
}

#if defined(OS_WIN)

CommandLine CommandLine::FromString(const std::wstring& command_line) {
  CommandLine cmd(NO_PROGRAM);
  cmd.ParseFromString(command_line);
  return cmd;
}
#endif

void CommandLine::InitFromArgv(int argc,
                               const CommandLine::CharType* const* argv) {
  StringVector new_argv;
  for (int i = 0; i < argc; ++i)
    new_argv.push_back(argv[i]);
  InitFromArgv(new_argv);
}

void CommandLine::InitFromArgv(const StringVector& argv) {
  argv_ = StringVector(1);
  switches_.clear();
  begin_args_ = 1;
  SetProgram(argv.empty() ? FilePath() : FilePath(argv[0]));
  AppendSwitchesAndArguments(*this, argv);
}

CommandLine::StringType CommandLine::GetCommandLineString() const {
  StringType string(argv_[0]);
#if defined(OS_WIN)
  string = QuoteForCommandLineToArgvW(string);
#endif
  StringType params(GetArgumentsString());
  if (!params.empty()) {
    string.append(StringType(FILE_PATH_LITERAL(" ")));
    string.append(params);
  }
  return string;
}

CommandLine::StringType CommandLine::GetArgumentsString() const {
  StringType params;
  
  bool parse_switches = true;
  for (size_t i = 1; i < argv_.size(); ++i) {
    StringType arg = argv_[i];
    StringType switch_string;
    StringType switch_value;
    parse_switches &= arg != kSwitchTerminator;
    if (i > 1)
      params.append(StringType(FILE_PATH_LITERAL(" ")));
    if (parse_switches && IsSwitch(arg, &switch_string, &switch_value)) {
      params.append(switch_string);
      if (!switch_value.empty()) {
#if defined(OS_WIN)
        switch_value = QuoteForCommandLineToArgvW(switch_value);
#endif
        params.append(kSwitchValueSeparator + switch_value);
      }
    }
    else {
#if defined(OS_WIN)
      arg = QuoteForCommandLineToArgvW(arg);
#endif
      params.append(arg);
    }
  }
  return params;
}

FilePath CommandLine::GetProgram() const {
  return FilePath(argv_[0]);
}

void CommandLine::SetProgram(const FilePath& program) {
  TrimWhitespace(program.value(), TRIM_ALL, &argv_[0]);
}

bool CommandLine::HasSwitch(const std::string& switch_string) const {
  return switches_.find(LowerASCIIOnWindows(switch_string)) != switches_.end();
}

std::string CommandLine::GetSwitchValueASCII(
    const std::string& switch_string) const {
  StringType value = GetSwitchValueNative(switch_string);
  if (!IsStringASCII(value)) {
    DLOG(WARNING) << "Value of switch (" << switch_string << ") must be ASCII.";
    return std::string();
  }
#if defined(OS_WIN)
  return WideToASCII(value);
#else
  return value;
#endif
}

FilePath CommandLine::GetSwitchValuePath(
    const std::string& switch_string) const {
  return FilePath(GetSwitchValueNative(switch_string));
}

CommandLine::StringType CommandLine::GetSwitchValueNative(
    const std::string& switch_string) const {
  SwitchMap::const_iterator result = switches_.end();
  result = switches_.find(LowerASCIIOnWindows(switch_string));
  return result == switches_.end() ? StringType() : result->second;
}

void CommandLine::AppendSwitch(const std::string& switch_string) {
  AppendSwitchNative(switch_string, StringType());
}

void CommandLine::AppendSwitchPath(const std::string& switch_string,
                                   const FilePath& path) {
  AppendSwitchNative(switch_string, path.value());
}

void CommandLine::AppendSwitchNative(const std::string& switch_string,
                                     const CommandLine::StringType& value) {
  std::string switch_key(LowerASCIIOnWindows(switch_string));
#if defined(OS_WIN)
  StringType combined_switch_string(ASCIIToWide(switch_key));
#elif defined(OS_POSIX)
  StringType combined_switch_string(switch_string);
#endif
  size_t prefix_length = GetSwitchPrefixLength(combined_switch_string);
  switches_[switch_key.substr(prefix_length)] = value;
  
  if (prefix_length == 0)
    combined_switch_string = kSwitchPrefixes[0] + combined_switch_string;
  if (!value.empty())
    combined_switch_string += kSwitchValueSeparator + value;
  
  argv_.insert(argv_.begin() + begin_args_++, combined_switch_string);
}

void CommandLine::AppendSwitchASCII(const std::string& switch_string,
                                    const std::string& value_string) {
#if defined(OS_WIN)
  AppendSwitchNative(switch_string, ASCIIToWide(value_string));
#elif defined(OS_POSIX)
  AppendSwitchNative(switch_string, value_string);
#endif
}

void CommandLine::CopySwitchesFrom(const CommandLine& source,
                                   const char* const switches[],
                                   size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (source.HasSwitch(switches[i]))
      AppendSwitchNative(switches[i], source.GetSwitchValueNative(switches[i]));
  }
}

CommandLine::StringVector CommandLine::GetArgs() const {
  
  StringVector args(argv_.begin() + begin_args_, argv_.end());
  
  StringVector::iterator switch_terminator =
      std::find(args.begin(), args.end(), kSwitchTerminator);
  if (switch_terminator != args.end())
    args.erase(switch_terminator);
  return args;
}

void CommandLine::AppendArg(const std::string& value) {
#if defined(OS_WIN)
  DCHECK(IsStringUTF8(value));
  AppendArgNative(UTF8ToWide(value));
#elif defined(OS_POSIX)
  AppendArgNative(value);
#endif
}

void CommandLine::AppendArgPath(const FilePath& path) {
  AppendArgNative(path.value());
}

void CommandLine::AppendArgNative(const CommandLine::StringType& value) {
  argv_.push_back(value);
}

void CommandLine::AppendArguments(const CommandLine& other,
                                  bool include_program) {
  if (include_program)
    SetProgram(other.GetProgram());
  AppendSwitchesAndArguments(*this, other.argv());
}

void CommandLine::PrependWrapper(const CommandLine::StringType& wrapper) {
  if (wrapper.empty())
    return;
  
  
  StringVector wrapper_argv;
  base::SplitString(wrapper, FILE_PATH_LITERAL(' '), &wrapper_argv);
  
  argv_.insert(argv_.begin(), wrapper_argv.begin(), wrapper_argv.end());
  begin_args_ += wrapper_argv.size();
}

#if defined(OS_WIN)
void CommandLine::ParseFromString(const std::wstring& command_line) {
  std::wstring command_line_string;
  TrimWhitespace(command_line, TRIM_ALL, &command_line_string);
  if (command_line_string.empty())
    return;

  int num_args = 0;
  wchar_t** args = NULL;
  args = ::CommandLineToArgvW(command_line_string.c_str(), &num_args);

  DPLOG_IF(FATAL, !args) << "CommandLineToArgvW failed on command line: "
                         << command_line;
  InitFromArgv(num_args, args);
  LocalFree(args);
}
#endif
