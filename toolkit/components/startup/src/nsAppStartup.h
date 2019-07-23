






































#ifndef nsAppStartup_h__
#define nsAppStartup_h__

#include "nsIAppStartup.h"
#include "nsIWindowCreator2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsINativeAppSupport.h"
#include "nsIAppShell.h"

struct PLEvent;


#define NS_TOOLKIT_APPSTARTUP_CID \
{ 0x7dd4d320, 0xc84b, 0x4624, { 0x8d, 0x45, 0x7b, 0xb9, 0xb2, 0x35, 0x69, 0x77 } }


class nsAppStartup : public nsIAppStartup2,
                     public nsIWindowCreator2,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSTARTUP
  NS_DECL_NSIAPPSTARTUP2
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIWINDOWCREATOR2
  NS_DECL_NSIOBSERVER

  nsAppStartup();
  nsresult Init();

private:
  ~nsAppStartup() { }

  void CloseAllWindows();

  friend class nsAppExitEvent;

  nsCOMPtr<nsIAppShell> mAppShell;

  PRInt32      mConsiderQuitStopper; 
  PRPackedBool mRunning;        
  PRPackedBool mShuttingDown;   
  PRPackedBool mAttemptingQuit; 
  PRPackedBool mRestart;        
};

#endif 
