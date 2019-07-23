






































#ifndef nsRemoteClient_h__
#define nsRemoteClient_h__

#include "nscore.h"





class nsRemoteClient
{
public:
  


  virtual nsresult Init() = 0;

  
































  virtual nsresult SendCommand(const char *aProgram, const char *aUsername,
                               const char *aProfile, const char *aCommand,
                               const char* aDesktopStartupID,
                               char **aResponse, PRBool *aSucceeded) = 0;

  










  virtual nsresult SendCommandLine(const char *aProgram, const char *aUsername,
                                   const char *aProfile,
                                   PRInt32 argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse, PRBool *aSucceeded) = 0;
};

#endif 
