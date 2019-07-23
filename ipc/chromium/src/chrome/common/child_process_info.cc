



#include "chrome/common/child_process_info.h"

#include <limits>

#ifndef CHROMIUM_MOZILLA_BUILD
#include "app/l10n_util.h"
#endif
#include "base/logging.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/string_util.h"
#ifndef CHROMIUM_MOZILLA_BUILD
#include "grit/generated_resources.h"
#endif

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
#ifdef CHROMIUM_MOZILLA_BUILD
  return name_;
#else
  std::wstring title = name_;
  if (type_ == ChildProcessInfo::PLUGIN_PROCESS && title.empty())
    title = l10n_util::GetString(IDS_TASK_MANAGER_UNKNOWN_PLUGIN_NAME);

  int message_id;
  if (type_ == ChildProcessInfo::PLUGIN_PROCESS) {
    message_id = IDS_TASK_MANAGER_PLUGIN_PREFIX;
  } else if (type_ == ChildProcessInfo::WORKER_PROCESS) {
    message_id = IDS_TASK_MANAGER_WORKER_PREFIX;
  } else {
    DCHECK(false) << "Need localized name for child process type.";
    return title;
  }

  
  
  
  
  l10n_util::AdjustStringForLocaleDirection(title, &title);
  return l10n_util::GetStringF(message_id, title);
#endif
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
