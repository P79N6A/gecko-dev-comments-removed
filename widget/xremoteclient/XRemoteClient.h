




#include <X11/X.h>
#include <X11/Xlib.h>

#include "nsRemoteClient.h"

class XRemoteClient : public nsRemoteClient
{
public:
  XRemoteClient();
  ~XRemoteClient();

  virtual nsresult Init();
  virtual nsresult SendCommandLine(const char *aProgram, const char *aUsername,
                                   const char *aProfile,
                                   int32_t argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse, bool *aSucceeded);
  void Shutdown();

private:

  Window         CheckWindow      (Window aWindow);
  Window         CheckChildren    (Window aWindow);
  nsresult       GetLock          (Window aWindow, bool *aDestroyed);
  nsresult       FreeLock         (Window aWindow);
  Window         FindBestWindow   (const char *aProgram,
                                   const char *aUsername,
                                   const char *aProfile);
  nsresult       DoSendCommandLine(Window aWindow,
                                   int32_t argc, char **argv,
                                   const char* aDesktopStartupID,
                                   char **aResponse,
                                   bool *aDestroyed);
  bool           WaitForResponse  (Window aWindow, char **aResponse,
                                   bool *aDestroyed, Atom aCommandAtom);

  Display       *mDisplay;

  Atom           mMozVersionAtom;
  Atom           mMozLockAtom;
  Atom           mMozCommandLineAtom;
  Atom           mMozResponseAtom;
  Atom           mMozWMStateAtom;
  Atom           mMozUserAtom;
  Atom           mMozProfileAtom;
  Atom           mMozProgramAtom;

  char          *mLockData;

  bool           mInitialized;
};
