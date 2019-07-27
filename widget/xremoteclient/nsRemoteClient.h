






#ifndef nsRemoteClient_h__
#define nsRemoteClient_h__

#include "nscore.h"





class nsRemoteClient
{
public:
  


  virtual nsresult Init() = 0;

  





























  virtual nsresult SendCommandLine(const char *aProgram, const char *aUsername,
                                   const char *aProfile,
                                   int32_t argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse, bool *aSucceeded) = 0;
};

#endif 
