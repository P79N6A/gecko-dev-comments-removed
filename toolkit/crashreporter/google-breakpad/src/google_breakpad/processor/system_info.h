

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SYSTEM_INFO_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SYSTEM_INFO_H__

#include <string>

#include "common/using_std_string.h"

namespace google_breakpad {

struct SystemInfo {
 public:
  SystemInfo() : os(), os_short(), os_version(), cpu(), cpu_info(),
    cpu_count(0) {}

  
  void Clear() {
    os.clear();
    os_short.clear();
    os_version.clear();
    cpu.clear();
    cpu_info.clear();
    cpu_count = 0;
  }

  
  
  
  
  string os;

  
  
  
  
  
  string os_short;

  
  
  
  string os_version;

  
  
  
  
  
  string cpu;

  
  
  
  
  string cpu_info;

  
  
  int cpu_count;
};

}  

#endif  
