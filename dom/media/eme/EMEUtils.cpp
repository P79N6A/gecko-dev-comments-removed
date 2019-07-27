





#include "mozilla/EMEUtils.h"

namespace mozilla {

#ifdef PR_LOGGING

PRLogModuleInfo* GetEMELog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("EME");
  }
  return log;
}

PRLogModuleInfo* GetEMEVerboseLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("EMEV");
  }
  return log;
}

#endif

} 
