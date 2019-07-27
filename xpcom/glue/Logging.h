





#ifndef mozilla_logging_h
#define mozilla_logging_h

#include "prlog.h"

#include "mozilla/Assertions.h"






namespace mozilla {

















enum class LogLevel {
  Disabled = 0,
  Error,
  Warning,
  Info,
  Debug,
  Verbose,
};

namespace detail {

inline bool log_test(const PRLogModuleInfo* module, LogLevel level) {
  MOZ_ASSERT(level != LogLevel::Disabled);
  return module && module->level >= static_cast<int>(level);
}

} 

} 

#define MOZ_LOG_TEST(_module,_level) mozilla::detail::log_test(_module, _level)

#define MOZ_LOG(_module,_level,_args)     \
  PR_BEGIN_MACRO             \
    if (MOZ_LOG_TEST(_module,_level)) { \
      PR_LogPrint _args;         \
    }                     \
  PR_END_MACRO

#undef PR_LOG
#undef PR_LOG_TEST

#endif

