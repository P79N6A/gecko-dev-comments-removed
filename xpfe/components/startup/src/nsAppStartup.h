






































#ifndef nsAppStartup_h__
#define nsAppStartup_h__

#include "nsIAppStartup.h"
#include "nsIWindowCreator2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsISplashScreen.h"
#include "nsINativeAppSupport.h"
#include "nsIAppShell.h"

#include "nsString.h"

struct PLEvent;


#define NS_SEAMONKEY_APPSTARTUP_CID \
{ 0xa70a1e2b, 0x1fcb, 0x41ca, { 0xb0, 0x11, 0x1d, 0x20, 0x7b, 0x9e, 0xa7, 0xa9 } }

class nsAppStartup : public nsIAppStartup,
                     public nsIWindowCreator2,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSTARTUP
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIWINDOWCREATOR2
  NS_DECL_NSIOBSERVER

  nsAppStartup();

private:
  friend class nsAppStartupExitEvent;

  ~nsAppStartup() { }

  void AttemptingQuit(PRBool aAttempt);

  nsresult CheckAndRemigrateDefunctProfile();
  nsresult LaunchTask(const char* aParam,
                      PRInt32 height, PRInt32 width,
                      PRBool *windowOpened);
  nsresult OpenWindow(const nsAFlatCString& aChromeURL,
                      const nsAFlatString& aAppArgs,
                      PRInt32 aWidth, PRInt32 aHeight);
  nsresult OpenBrowserWindow(PRInt32 height, PRInt32 width);

  nsCOMPtr<nsIAppShell> mAppShell;
  nsCOMPtr<nsISplashScreen> mSplashScreen;
  nsCOMPtr<nsINativeAppSupport> mNativeAppSupport;

  PRInt32      mConsiderQuitStopper; 
  PRPackedBool mShuttingDown;   
  PRPackedBool mAttemptingQuit; 
};

#endif 
