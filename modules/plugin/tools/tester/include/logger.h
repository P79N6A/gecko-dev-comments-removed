




































#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "plugbase.h"
#include "action.h"
#include "log.h"
#include "logFile.h"

class CLogger
{
private:
  CPluginBase * m_pPlugin;
  NPP m_pPluginInstance;
  CLogItemList * m_pLog;
  CLogFile * m_pLogFile;
  BOOL m_bShowImmediately;
  BOOL m_bLogToFile;
  BOOL m_bLogToFrame;
  BOOL m_bBlockLogToFile;
  BOOL m_bBlockLogToFrame;
  char m_szTarget[256];
  NPStream * m_pStream;
  char m_szStreamType[80];
  DWORD m_dwStartTime;
  char m_szItemSeparator[80];
  int m_iStringDataWrap;
  BOOL m_bStale;

public:
  CLogger(LPSTR szTarget = NULL);
  ~CLogger();

  void associate(CPluginBase * pPlugin);

  void setShowImmediatelyFlag(BOOL bFlagState);
  void setLogToFileFlag(BOOL bFlagState);
  void setLogToFrameFlag(BOOL bFlagState);

  BOOL getShowImmediatelyFlag();
  BOOL getLogToFileFlag();
  BOOL getLogToFrameFlag();
  int getStringDataWrap();

  void restorePreferences(LPSTR szFileName);

  BOOL onNPP_DestroyStream(NPStream * npStream);

  BOOL appendToLog(NPAPI_Action action, DWORD dwTickEnter, DWORD dwTickReturn, DWORD dwRet = 0L, 
                   DWORD dw1 = 0L, DWORD dw2 = 0L, DWORD dw3 = 0L, DWORD dw4 = 0L, 
                   DWORD dw5 = 0L, DWORD dw6 = 0L, DWORD dw7 = 0L);
  void dumpLogToTarget();
  void clearLog();
  void clearTarget();
  void resetStartTime();
  void blockDumpToFile(BOOL bBlock);
  void blockDumpToFrame(BOOL bBlock);

  void closeLogToFile();

  void markStale();
  BOOL isStale();
};

#define LOGGER_DEFAULT_STRING_WRAP    32
#define LOGGER_DEFAULT_TARGET "_npapi_Log"


#define SECTION_LOG           "Log"
#define KEY_RECORD_SEPARATOR  "RecordSeparator"
#define KEY_STRING_WRAP       "StringDataWrap"
#define KEY_FILE_NAME         "FileName"

#endif 
