



#include "base/process.h"
#include "base/logging.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"

namespace base {

void Process::Close() {
  if (!process_)
    return;
  ::CloseHandle(process_);
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




static const int kWinDefaultMinSet = 50 * 4096;
static const int kWinDefaultMaxSet = 345 * 4096;
static const int kDampingFactor = 2;

bool Process::ReduceWorkingSet() {
  if (!process_)
    return false;
  
  
  
  

  
  
  
  
  
  

  scoped_ptr<ProcessMetrics> metrics(
      ProcessMetrics::CreateProcessMetrics(process_));
  WorkingSetKBytes working_set;
  if (!metrics->GetWorkingSetKBytes(&working_set))
    return false;


  
  
  
  
  
  
  
  size_t current_working_set_size  = working_set.priv +
                                     working_set.shareable;

  size_t max_size = current_working_set_size;
  if (last_working_set_size_)
    max_size = (max_size + last_working_set_size_) / 2;  
  max_size *= 1024;  
  last_working_set_size_ = current_working_set_size / kDampingFactor;

  BOOL rv = SetProcessWorkingSetSize(process_, kWinDefaultMinSet, max_size);
  return rv == TRUE;
}

bool Process::UnReduceWorkingSet() {
  if (!process_)
    return false;

  if (!last_working_set_size_)
    return true;  

  
  
  size_t limit = last_working_set_size_ * kDampingFactor * 2 * 1024;
  BOOL rv = SetProcessWorkingSetSize(process_, kWinDefaultMinSet, limit);
  return rv == TRUE;
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
