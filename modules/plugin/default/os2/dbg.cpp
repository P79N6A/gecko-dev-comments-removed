




































#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern char szAppName[];

#ifdef _DEBUG

void dbgOut(PSZ format, ...) { 
  static char buf[1024];
  strcpy(buf, szAppName);
  strcat(buf, ": ");
  va_list  va;
  va_start(va, format);
  vsprintf(&buf[strlen(buf)], format, va);
  va_end(va);
  strcat(buf, "\n");
  printf("%s\n", buf); 
}

#endif
