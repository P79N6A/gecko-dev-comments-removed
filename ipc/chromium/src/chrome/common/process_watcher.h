



#ifndef CHROME_COMMON_PROCESS_WATCHER_H_
#define CHROME_COMMON_PROCESS_WATCHER_H_

#include "base/basictypes.h"
#include "base/process_util.h"

class ProcessWatcher {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void EnsureProcessTerminated(base::ProcessHandle process_handle);

 private:
  
  ProcessWatcher();

  DISALLOW_COPY_AND_ASSIGN(ProcessWatcher);
};

#endif  
