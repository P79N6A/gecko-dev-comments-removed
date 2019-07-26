






































#include "cpr.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpr_types.h"
#include "cpr_win_stdio.h"
#include "CSFLog.h"

typedef enum
{
    CPR_LOGLEVEL_ERROR=0,
    CPR_LOGLEVEL_WARNING,
    CPR_LOGLEVEL_INFO,
    CPR_LOGLEVEL_DEBUG
} cpr_log_level_e;







#define LOG_MAX 4096


int32_t
buginf_msg (const char *str)
{
    OutputDebugStringA(str);

    return (0);
}

void
err_msg (const char *_format, ...)
{
  va_list ap;
  
  va_start(ap, _format);  
  CSFLogErrorV("cpr", _format, ap);
  va_end(ap);
}

void
notice_msg (const char *_format, ...)
{
  va_list ap;

  va_start(ap, _format);  
  CSFLogInfoV("cpr", _format, ap);
  va_end(ap);
}

int32_t
buginf (const char *_format, ...)
{
  va_list ap;
  
  va_start(ap, _format);  
  CSFLogDebugV("cpr", _format, ap);
  va_end(ap);

  return (0);
}

