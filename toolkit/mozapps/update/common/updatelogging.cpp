



#if defined(XP_WIN)
#include <windows.h>
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "updatelogging.h"

UpdateLog::UpdateLog() : logFP(nullptr)
{
}

void UpdateLog::Init(NS_tchar* sourcePath,
                     const NS_tchar* fileName,
                     const NS_tchar* alternateFileName,
                     bool append)
{
  if (logFP)
    return;

#ifdef XP_WIN
  GetTempFileNameW(sourcePath, L"log", 0, mTmpFilePath);
  if (append) {
    NS_tsnprintf(mDstFilePath, sizeof(mDstFilePath)/sizeof(mDstFilePath[0]),
      NS_T("%s/%s"), sourcePath, alternateFileName);
    MoveFileExW(mDstFilePath, mTmpFilePath, MOVEFILE_REPLACE_EXISTING);
  } else {
    NS_tsnprintf(mDstFilePath, sizeof(mDstFilePath)/sizeof(mDstFilePath[0]),
                 NS_T("%s/%s"), sourcePath, fileName);
  }

  logFP = NS_tfopen(mTmpFilePath, append ? NS_T("a") : NS_T("w"));
  
  
  DeleteFileW(mDstFilePath);
#else
  NS_tsnprintf(mDstFilePath, sizeof(mDstFilePath)/sizeof(mDstFilePath[0]),
               NS_T("%s/%s"), sourcePath, fileName);

  if (alternateFileName && NS_taccess(mDstFilePath, F_OK)) {
    NS_tsnprintf(mDstFilePath, sizeof(mDstFilePath)/sizeof(mDstFilePath[0]),
      NS_T("%s/%s"), sourcePath, alternateFileName);
  }

  logFP = NS_tfopen(mDstFilePath, append ? NS_T("a") : NS_T("w"));
#endif
}

void UpdateLog::Finish()
{
  if (!logFP)
    return;

  fclose(logFP);
  logFP = nullptr;

#ifdef XP_WIN
  
  
  if (!NS_taccess(mDstFilePath, F_OK)) {
    DeleteFileW(mTmpFilePath);
  } else {
    MoveFileW(mTmpFilePath, mDstFilePath);
  }
#endif
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
  fprintf(logFP, "\n");
  va_end(ap);
}

void UpdateLog::WarnPrintf(const char *fmt, ... )
{
  if (!logFP)
    return;

  va_list ap;
  va_start(ap, fmt);
  fprintf(logFP, "*** Warning: ");
  vfprintf(logFP, fmt, ap);
  fprintf(logFP, "***\n");
  va_end(ap);
}
