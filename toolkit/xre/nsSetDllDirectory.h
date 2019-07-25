





































#ifndef nsSetDllDirectory_h
#define nsSetDllDirectory_h

#ifndef XP_WIN
#error This file only makes sense on Windows.
#endif

#include <nscore.h>

namespace mozilla {



XPCOM_API(void) NS_SetDllDirectory(const WCHAR *aDllDirectory);

}

#endif
