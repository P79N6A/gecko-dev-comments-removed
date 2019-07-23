




































#ifndef __nsAppShellService_h
#define __nsAppShellService_h

#include "nsIAppShellService.h"
#include "nsIObserver.h"


#include "nsWebShellWindow.h"
#include "nsStringFwd.h"
#include "nsAutoPtr.h"


#define NS_APPSHELLSERVICE_CID \
{ 0x99907d, 0x123c, 0x4853, { 0xa4, 0x6a, 0x43, 0x9, 0x8b, 0x5f, 0xb6, 0x8c } }

class nsAppShellService : public nsIAppShellService,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSHELLSERVICE
  NS_DECL_NSIOBSERVER

  nsAppShellService();

protected:
  ~nsAppShellService();

  nsresult JustCreateTopWindow(nsIXULWindow *aParent,
                               nsIURI *aUrl, 
                               PRUint32 aChromeMask,
                               PRInt32 aInitialWidth, PRInt32 aInitialHeight,
                               PRBool aIsHiddenWindow, nsIAppShell* aAppShell,
                               nsWebShellWindow **aResult);
  PRUint32 CalculateWindowZLevel(nsIXULWindow *aParent, PRUint32 aChromeMask);
  nsresult SetXPConnectSafeContext();
  nsresult ClearXPConnectSafeContext();

  nsRefPtr<nsWebShellWindow>  mHiddenWindow;
  PRPackedBool                mXPCOMWillShutDown;
  PRPackedBool                mXPCOMShuttingDown;
  PRUint16                    mModalWindowCount;
  PRPackedBool                mApplicationProvidedHiddenWindow;
};

#endif
