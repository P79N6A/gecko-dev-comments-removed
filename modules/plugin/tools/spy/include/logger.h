




































#ifndef _LOGGER_H__
#define __LOGGER_H__

#include "npupp.h"
#include "format.h"
#include "logfile.h"

#define TOTAL_NUMBER_OF_API_CALLS 37
#define DEFAULT_LOG_FILE_NAME "spylog.txt"

class Logger
{
public:
  BOOL bMutedAll;
  BOOL bToWindow;
  BOOL bToConsole;
  BOOL bToFile;
  BOOL bOnTop;
  BOOL bSPALID; 
                
  CLogFile filer;

  BOOL bSaveSettings;
  char szFile[_MAX_PATH];
  
  
  
  BOOL bMutedCalls[TOTAL_NUMBER_OF_API_CALLS];

public:
  Logger();
  ~Logger();

  BOOL init();
  void shut();

  
  virtual BOOL platformInit() = 0;
  virtual void platformShut() = 0;
  virtual void dumpStringToMainWindow(char * string) = 0;

  void setOnTop(BOOL ontop);
  void setToFile(BOOL tofile, char * filename);

  BOOL * getMutedCalls();
  void setMutedCalls(BOOL * mutedcalls);

  BOOL isMuted(NPAPI_Action action);

  void logNS_NP_GetEntryPoints();
  void logNS_NP_Initialize();
  void logNS_NP_Shutdown();

  void logSPY_NP_GetEntryPoints(NPPluginFuncs * pNPPFuncs);
  void logSPY_NP_Initialize();
  void logSPY_NP_Shutdown(char * mimetype);

  void logCall(NPAPI_Action action, DWORD dw1 = 0L, DWORD dw2 = 0L, 
               DWORD dw3 = 0L, DWORD dw4 = 0L, DWORD dw5 = 0L, DWORD dw6 = 0L, DWORD dw7 = 0L);
  void logReturn(DWORD dwRet = 0L);
};

Logger * NewLogger();
void DeleteLogger(Logger * logger);

#endif
