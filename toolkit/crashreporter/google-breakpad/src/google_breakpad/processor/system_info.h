

































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SYSTEM_INFO_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SYSTEM_INFO_H__

#include <string>

namespace google_breakpad {

using std::string;

struct SystemInfo {
 public:
  
  void Clear() {
    os.clear();
    os_short.clear();
    os_version.clear();
    cpu.clear();
    cpu_info.clear();
  }

  
  
  
  
  string os;

  
  
  
  
  
  string os_short;

  
  
  
  string os_version;

  
  
  
  
  
  string cpu;

  
  
  
  
  string cpu_info;
};

}  

#endif  
