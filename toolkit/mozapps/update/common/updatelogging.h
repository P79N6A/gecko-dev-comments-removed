




































#ifndef UPDATELOGGING_H
#define UPDATELOGGING_H

#include "updatedefines.h"
#include <stdio.h>

class UpdateLog
{
public:
  static UpdateLog & GetPrimaryLog() 
  {
    if (!primaryLog) {
      primaryLog = new UpdateLog();
    }
    return *primaryLog;
  }

  void Init(NS_tchar* sourcePath, NS_tchar* fileName);
  void Finish();
  void Printf(const char *fmt, ... );

  ~UpdateLog()
  {
    delete primaryLog;
  }

protected:
  UpdateLog();
  FILE *logFP;
  NS_tchar* sourcePath;

  static UpdateLog* primaryLog;
};

#define LOG(args) UpdateLog::GetPrimaryLog().Printf args
#define LogInit(PATHNAME_, FILENAME_) \
  UpdateLog::GetPrimaryLog().Init(PATHNAME_, FILENAME_)
#define LogFinish() UpdateLog::GetPrimaryLog().Finish()

#endif
