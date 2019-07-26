









#include "webrtc/system_wrappers/source/critical_section_win.h"

namespace webrtc {

CriticalSectionWindows::CriticalSectionWindows() {
  InitializeCriticalSection(&crit);
}

CriticalSectionWindows::~CriticalSectionWindows() {
  DeleteCriticalSection(&crit);
}

void
CriticalSectionWindows::Enter() {
  EnterCriticalSection(&crit);
}

void
CriticalSectionWindows::Leave() {
  LeaveCriticalSection(&crit);
}

} 
