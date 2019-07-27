




























#ifndef CLIENT_WINDOWS_COMMON_AUTO_CRITICAL_SECTION_H__
#define CLIENT_WINDOWS_COMMON_AUTO_CRITICAL_SECTION_H__

#include <windows.h>

namespace google_breakpad {



class AutoCriticalSection {
 public:
  
  
  explicit AutoCriticalSection(CRITICAL_SECTION* cs) : cs_(cs), taken_(false) {
    assert(cs_);
    Acquire();
  }

  
  ~AutoCriticalSection() {
    if (taken_) {
      Release();
    }
  }

  
  void Acquire() {
    assert(!taken_);
    EnterCriticalSection(cs_);
    taken_ = true;
  }

  
  
  void Release() {
    assert(taken_);
    taken_ = false;
    LeaveCriticalSection(cs_);
  }

 private:
  
  AutoCriticalSection(const AutoCriticalSection&);
  AutoCriticalSection& operator=(const AutoCriticalSection&);

  CRITICAL_SECTION* cs_;
  bool taken_;
};

}  

#endif  
