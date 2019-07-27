





#ifndef mozilla_SandboxLogging_h
#define mozilla_SandboxLogging_h

















#ifndef NDEBUG
#define NDEBUG 1
#include "base/strings/safe_sprintf.h"
#undef NDEBUG
#else
#include "base/strings/safe_sprintf.h"
#endif

namespace mozilla {

void SandboxLogError(const char* aMessage);
}

#define SANDBOX_LOG_LEN 256






#define SANDBOX_LOG_ERROR(fmt, args...) do {                          \
  char _sandboxLogBuf[SANDBOX_LOG_LEN];                               \
  ::base::strings::SafeSPrintf(_sandboxLogBuf, fmt, ## args);         \
  ::mozilla::SandboxLogError(_sandboxLogBuf);                         \
} while(0)

#endif 
