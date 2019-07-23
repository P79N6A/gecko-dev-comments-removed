



#ifndef CHROME_COMMON_DEBUG_FLAGS_H__
#define CHROME_COMMON_DEBUG_FLAGS_H__

#include "chrome/common/child_process_info.h"

class CommandLine;

class DebugFlags {
 public:

  
  
  
  
  
  
  
  
  static bool ProcessDebugFlags(CommandLine* command_line,
                                ChildProcessInfo::ProcessType type,
                                bool is_in_sandbox);
};

#endif  
