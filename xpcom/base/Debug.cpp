




#include "mozilla/Debug.h"

#ifdef XP_WIN
#include <windows.h>
#endif

#ifdef XP_WIN

void
mozilla::PrintToDebugger(const char* aStr)
{
  if (::IsDebuggerPresent()) {
    ::OutputDebugStringA(aStr);
  }
}

#endif
