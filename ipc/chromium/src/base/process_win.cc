



#include "base/process.h"
#include "base/logging.h"
#include "base/process_util.h"

namespace base {

void Process::Close() {
  if (!process_)
    return;
  CloseProcessHandle(process_);
  process_ = NULL;
}

void Process::Terminate(int result_code) {
  if (!process_)
    return;
  ::TerminateProcess(process_, result_code);
}

bool Process::IsProcessBackgrounded() const {
  DCHECK(process_);
  DWORD priority = GetPriorityClass(process_);
  if (priority == 0)
    return false;  
  return priority == BELOW_NORMAL_PRIORITY_CLASS;
}

bool Process::SetProcessBackgrounded(bool value) {
  DCHECK(process_);
  DWORD priority = value ? BELOW_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
  return (SetPriorityClass(process_, priority) != 0);
}

bool Process::EmptyWorkingSet() {
  if (!process_)
    return false;

  BOOL rv = SetProcessWorkingSetSize(process_, -1, -1);
  return rv == TRUE;
}

ProcessId Process::pid() const {
  if (process_ == 0)
    return 0;

  return GetProcId(process_);
}

bool Process::is_current() const {
  return process_ == GetCurrentProcess();
}


Process Process::Current() {
  return Process(GetCurrentProcess());
}

}  
