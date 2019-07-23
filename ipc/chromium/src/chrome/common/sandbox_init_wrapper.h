



#ifndef CHROME_COMMON_SANDBOX_INIT_WRAPPER_H_
#define CHROME_COMMON_SANDBOX_INIT_WRAPPER_H_





#include "build/build_config.h"

#include <string>

#include "base/basictypes.h"
#if defined(OS_WIN)
#include "sandbox/src/sandbox.h"
#endif

class CommandLine;

#if defined(OS_WIN)

class SandboxInitWrapper {
 public:
  SandboxInitWrapper() : broker_services_(), target_services_() { }
  
  
  void SetServices(sandbox::SandboxInterfaceInfo* sandbox_info);
  sandbox::BrokerServices* BrokerServices() const { return broker_services_; }
  sandbox::TargetServices* TargetServices() const { return target_services_; }

  
  
  void InitializeSandbox(const CommandLine& parsed_command_line,
                         const std::wstring& process_type);
 private:
  sandbox::BrokerServices* broker_services_;
  sandbox::TargetServices* target_services_;

  DISALLOW_COPY_AND_ASSIGN(SandboxInitWrapper);
};

#elif defined(OS_POSIX)

class SandboxInitWrapper {
 public:
  SandboxInitWrapper() { }

  
  
  void InitializeSandbox(const CommandLine& parsed_command_line,
                         const std::wstring& process_type);

#if defined(OS_MACOSX)
  
 public:
  std::wstring ProcessType() const { return process_type_; }
 private:
  std::wstring process_type_;
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(SandboxInitWrapper);
};

#endif

#endif  
