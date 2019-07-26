




#import <Carbon/Carbon.h>

#include "nsIMacDockSupport.h"
#include "nsIStandaloneNativeMenu.h"
#include "nsITaskbarProgress.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsMacDockSupport : public nsIMacDockSupport, public nsITaskbarProgress
{
public:
  nsMacDockSupport();
  virtual ~nsMacDockSupport();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACDOCKSUPPORT
  NS_DECL_NSITASKBARPROGRESS

protected:
  nsCOMPtr<nsIStandaloneNativeMenu> mDockMenu;
  nsString mBadgeText;

  NSImage *mAppIcon, *mProgressBackground;
  HIThemeTrackDrawInfo mProgressDrawInfo;

  nsTaskbarProgressState mProgressState;
  double mProgressFraction;
  nsCOMPtr<nsITimer> mProgressTimer;

  static void RedrawIconCallback(nsITimer* aTimer, void* aClosure);

  bool InitProgress();
  nsresult RedrawIcon();
};
