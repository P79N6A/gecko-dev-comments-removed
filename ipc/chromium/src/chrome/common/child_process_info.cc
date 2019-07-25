



#include "chrome/common/child_process_info.h"

#include <limits>

#include "base/logging.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/string_util.h"

std::wstring ChildProcessInfo::GetTypeNameInEnglish(
    ChildProcessInfo::ProcessType type) {
  switch (type) {
    case BROWSER_PROCESS:
      return L"Browser";
    case RENDER_PROCESS:
      return L"Tab";
    case PLUGIN_PROCESS:
      return L"Plug-in";
    case WORKER_PROCESS:
      return L"Web Worker";
    case UNKNOWN_PROCESS:
      default:
      DCHECK(false) << "Unknown child process type!";
      return L"Unknown";
    }
}

std::wstring ChildProcessInfo::GetLocalizedTitle() const {
  return name_;
}

ChildProcessInfo::ChildProcessInfo(ProcessType type) {
  
  
  
  
  type_ = type;
  pid_ = -1;
}


ChildProcessInfo::~ChildProcessInfo() {
}

std::wstring ChildProcessInfo::GenerateRandomChannelID(void* instance) {
  
  
  
  
  
  
  return StringPrintf(L"%d.%x.%d",
                      base::GetCurrentProcId(), instance,
                      base::RandInt(0, std::numeric_limits<int>::max()));
}
