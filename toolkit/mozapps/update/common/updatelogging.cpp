




































#if defined(XP_WIN)
#include <windows.h>
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "updatelogging.h"

UpdateLog::UpdateLog() : logFP(NULL)
{
}

void UpdateLog::Init(NS_tchar* sourcePath, NS_tchar* fileName)
{
  if (logFP)
    return;

  this->sourcePath = sourcePath;
  NS_tchar logFile[MAXPATHLEN];
  NS_tsnprintf(logFile, sizeof(logFile)/sizeof(logFile[0]),
    NS_T("%s/%s"), sourcePath, fileName);

  logFP = NS_tfopen(logFile, NS_T("w"));
}

void UpdateLog::Finish()
{
  if (!logFP)
    return;

  fclose(logFP);
  logFP = NULL;
}

void UpdateLog::Flush()
{
  if (!logFP)
    return;

  fflush(logFP);
}


void UpdateLog::Printf(const char *fmt, ... )
{
  if (!logFP)
    return;

  va_list ap;
  va_start(ap, fmt);
  vfprintf(logFP, fmt, ap);
  va_end(ap);
}
