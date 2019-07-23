



#include "chrome/common/sandbox_init_wrapper.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"

#if defined(OS_WIN)

void SandboxInitWrapper::SetServices(sandbox::SandboxInterfaceInfo* info) {
  if (info) {
    broker_services_ = info->broker_services;
    target_services_ = info->target_services;
  }
}

#endif

void SandboxInitWrapper::InitializeSandbox(const CommandLine& command_line,
                                           const std::wstring& process_type) {
#if defined(OS_WIN)
  if (!target_services_)
    return;
#endif
  if (!command_line.HasSwitch(switches::kNoSandbox)) {
    if ((process_type == switches::kRendererProcess) ||
        (process_type == switches::kWorkerProcess) ||
        (process_type == switches::kPluginProcess &&
         command_line.HasSwitch(switches::kSafePlugins))) {
#if defined(OS_WIN)
      target_services_->Init();
#elif defined(OS_MACOSX)
      
      
      
      process_type_ = process_type;
#endif
    }
  }
}
