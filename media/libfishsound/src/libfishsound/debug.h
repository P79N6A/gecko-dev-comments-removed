



















#ifndef __DEBUG_H__
#define __DEBUG_H__


#if (defined (_MSCVER) || defined (_MSC_VER))
#define debug_printf
#else

#ifdef DEBUG
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif
#endif

#if (DEBUG_LEVEL > 0)

#define DEBUG_MAXLINE 4096

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>






static void
debug_print_err (const char * func, int line, const char * fmt, ...)
{
  va_list ap;
  int errno_save;
  char buf[DEBUG_MAXLINE];
  int n=0;

  errno_save = errno;

  va_start (ap, fmt);

  if (func) {
    snprintf (buf+n, DEBUG_MAXLINE-n, "%s():%d: ", func, line);
    n = strlen (buf);
  }

  vsnprintf (buf+n, DEBUG_MAXLINE-n, fmt, ap);
  n = strlen (buf);

  fflush (stdout); 
  fputs (buf, stderr);
  fputc ('\n', stderr);
  fflush (NULL);

  va_end (ap);
}






#define debug_printf(x,y...) {if (x <= DEBUG_LEVEL) debug_print_err (__func__, __LINE__, y);}

#undef MAXLINE

#else
#define debug_printf(x,y...)
#endif

#endif 

#endif 
