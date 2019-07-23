





































#include "nsRemoteClient.h"

class XRemoteClient : public nsRemoteClient
{
 public:
  XRemoteClient();
  ~XRemoteClient();

  virtual nsresult Init();
  virtual nsresult SendCommand(const char *aProgram, const char *aUsername,
                               const char *aProfile, const char *aCommand,
							   const char* aDesktopStartupID,
                               char **aResponse, PRBool *aSucceeded);
  virtual nsresult SendCommandLine(const char *aProgram, const char *aUsername,
                                   const char *aProfile,
                                   PRInt32 argc, char **argv,
								   const char* aDesktopStartupID,
                                   char **aResponse, PRBool *aSucceeded);
  void Shutdown();

 private:
	PRBool mInitialized;
};
