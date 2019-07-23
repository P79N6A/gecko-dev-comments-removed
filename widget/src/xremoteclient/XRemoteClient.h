




































#include <X11/X.h>
#include <X11/Xlib.h>

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

  Window         CheckWindow      (Window aWindow);
  Window         CheckChildren    (Window aWindow);
  nsresult       GetLock          (Window aWindow, PRBool *aDestroyed);
  nsresult       FreeLock         (Window aWindow);
  Window         FindBestWindow   (const char *aProgram,
                                   const char *aUsername,
                                   const char *aProfile,
                                   PRBool aSupportsCommandLine);
  nsresult     SendCommandInternal(const char *aProgram, const char *aUsername,
                                   const char *aProfile, const char *aCommand,
                                   PRInt32 argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse, PRBool *aWindowFound);
  nsresult       DoSendCommand    (Window aWindow,
                                   const char *aCommand,
                                   const char* aDesktopStartupID,
                                   char **aResponse,
                                   PRBool *aDestroyed);
  nsresult       DoSendCommandLine(Window aWindow,
                                   PRInt32 argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse,
                                   PRBool *aDestroyed);
  PRBool         WaitForResponse  (Window aWindow, char **aResponse,
                                   PRBool *aDestroyed, Atom aCommandAtom);

  Display       *mDisplay;

  Atom           mMozVersionAtom;
  Atom           mMozLockAtom;
  Atom           mMozCommandAtom;
  Atom           mMozCommandLineAtom;
  Atom           mMozResponseAtom;
  Atom           mMozWMStateAtom;
  Atom           mMozUserAtom;
  Atom           mMozProfileAtom;
  Atom           mMozProgramAtom;
  Atom           mMozSupportsCLAtom;

  char          *mLockData;

  PRBool         mInitialized;
};
