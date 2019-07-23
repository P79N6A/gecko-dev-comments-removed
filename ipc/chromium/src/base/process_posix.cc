



#include "base/process.h"
#include "base/process_util.h"

namespace base {

void Process::Close() {
  process_ = 0;
  
  
  
}

void Process::Terminate(int result_code) {
  
  if (!process_)
    return;
  
  
  KillProcess(process_, result_code, false);
}

bool Process::IsProcessBackgrounded() const {
  
  return false;
}

bool Process::SetProcessBackgrounded(bool value) {
  
  
  
  
  return true;
}

bool Process::ReduceWorkingSet() {
  
  return false;
}

bool Process::UnReduceWorkingSet() {
  
  return false;
}

bool Process::EmptyWorkingSet() {
  
  return false;
}

ProcessId Process::pid() const {
  if (process_ == 0)
    return 0;

  return GetProcId(process_);
}

bool Process::is_current() const {
  return process_ == GetCurrentProcessHandle();
}


Process Process::Current() {
  return Process(GetCurrentProcessHandle());
}

}  
