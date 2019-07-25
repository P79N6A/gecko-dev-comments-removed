




































#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern char szAppName[];

#ifdef _DEBUG

void __cdecl dbgOut(LPSTR format, ...) { 
  static char buf[1024];
  lstrcpy(buf, szAppName);
  lstrcat(buf, ": ");
  va_list  va;
  va_start(va, format);
  wvsprintf(&buf[lstrlen(buf)], format, va);
  va_end(va);
  lstrcat(buf, "\n");
  OutputDebugString(buf); 
}

#endif
