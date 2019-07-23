



#include <windows.h>

#include "base/debug_on_start.h"

#include "base/base_switches.h"
#include "base/basictypes.h"
#include "base/debug_util.h"







bool DebugOnStart::FindArgument(wchar_t* command_line, const wchar_t* argument)
{
  int argument_len = lstrlen(argument);
  int command_line_len = lstrlen(command_line);
  while (command_line_len > argument_len) {
    wchar_t first_char = command_line[0];
    wchar_t last_char = command_line[argument_len+1];
    
    if ((first_char == L'-' || first_char == L'/') &&
        (last_char == L' ' || last_char == 0 || last_char == L'=')) {
      command_line[argument_len+1] = 0;
      
      if (lstrcmpi(command_line+1, argument) == 0) {
        
        command_line[argument_len+1] = last_char;
        return true;
      }
      
      command_line[argument_len+1] = last_char;
    }
    
    ++command_line;
    --command_line_len;
  }
  return false;
}


int __cdecl DebugOnStart::Init() {
  
  if (FindArgument(GetCommandLine(), switches::kDebugOnStart)) {
    
    
    
    

    
    DebugUtil::SpawnDebuggerOnProcess(GetCurrentProcessId());

    
    DebugUtil::WaitForDebugger(60, false);
  } else if (FindArgument(GetCommandLine(), switches::kWaitForDebugger)) {
    
    DebugUtil::WaitForDebugger(60, true);
  }
  return 0;
}
