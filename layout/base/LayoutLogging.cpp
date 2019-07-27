





#include "base/process_util.h"

#include "LayoutLogging.h"

PRLogModuleInfo* GetLayoutLog()
{
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("layout");
  }

  return log;
}

namespace mozilla {
namespace detail {

void LayoutLogWarning(const char* aStr, const char* aExpr,
                      const char* aFile, int32_t aLine)
{
  if (aExpr) {
    MOZ_LOG(GetLayoutLog(),
            mozilla::LogLevel::Warning,
            ("[%d] WARNING: %s: '%s', file %s, line %d",
             base::GetCurrentProcId(),
             aStr, aExpr, aFile, aLine));
  } else {
    MOZ_LOG(GetLayoutLog(),
            mozilla::LogLevel::Warning,
            ("[%d] WARNING: %s: file %s, line %d",
             base::GetCurrentProcId(),
             aStr, aFile, aLine));
  }
}

} 
} 
