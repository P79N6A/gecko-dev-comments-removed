




































#include <windows.h>

#ifndef NTRACE
void __cdecl dbgOut(LPSTR format, ...) { 
  static char buf[1024];
  va_list  va;
  va_start(va, format);
  wvsprintf(buf, format, va);
  va_end(va);
  lstrcat(buf, "\n");
  OutputDebugString(buf); 
}
#endif  

