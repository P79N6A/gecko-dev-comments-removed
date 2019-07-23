







#ifndef CHROME_COMMON_MAIN_FUNCTINON_PARAMS_H_
#define CHROME_COMMON_MAIN_FUNCTINON_PARAMS_H_

#include "base/command_line.h"
#include "chrome/common/sandbox_init_wrapper.h"

namespace base {
class ScopedNSAutoreleasePool;
};
class Task;

struct MainFunctionParams {
  MainFunctionParams(const CommandLine& cl, const SandboxInitWrapper& sb,
                     base::ScopedNSAutoreleasePool* pool)
      : command_line_(cl), sandbox_info_(sb), autorelease_pool_(pool),
        ui_task(NULL) { }
  const CommandLine& command_line_;
  const SandboxInitWrapper& sandbox_info_;
  base::ScopedNSAutoreleasePool* autorelease_pool_;
  
  
  Task* ui_task;
};

#endif  
