





































#ifndef MOZCE_WINDOWS_H
#define MOZCE_WINDOWS_H
#include "mozce_windows_actual_incl.h"
#ifdef GetProcAddress
#undef GetProcAddress
#endif
#define GetProcAddress GetProcAddressA
#endif
