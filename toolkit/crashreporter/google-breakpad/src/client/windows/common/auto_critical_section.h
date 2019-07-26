




























#ifndef CLIENT_WINDOWS_COMMON_AUTO_CRITICAL_SECTION_H__
#define CLIENT_WINDOWS_COMMON_AUTO_CRITICAL_SECTION_H__

#include <Windows.h>

namespace google_breakpad {



class AutoCriticalSection {
 public:
  
  
  explicit AutoCriticalSection(CRITICAL_SECTION* cs) : cs_(cs) {
    assert(cs_);
    EnterCriticalSection(cs_);
  }

  
  ~AutoCriticalSection() {
    LeaveCriticalSection(cs_);
  }

 private:
  
  AutoCriticalSection(const AutoCriticalSection&);
  AutoCriticalSection& operator=(const AutoCriticalSection&);

  CRITICAL_SECTION* cs_;
};

}  

#endif  
