




#include "nsIMacDockSupport.h"
#include "nsIStandaloneNativeMenu.h"
#include "nsITaskbarProgress.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsNativeThemeCocoa.h"

class nsMacDockSupport : public nsIMacDockSupport, public nsITaskbarProgress
{
public:
  nsMacDockSupport();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACDOCKSUPPORT
  NS_DECL_NSITASKBARPROGRESS

protected:
  virtual ~nsMacDockSupport();

  nsCOMPtr<nsIStandaloneNativeMenu> mDockMenu;
  nsString mBadgeText;

  NSImage *mAppIcon, *mProgressBackground;

  HIRect mProgressBounds;
  nsTaskbarProgressState mProgressState;
  double mProgressFraction;
  nsCOMPtr<nsITimer> mProgressTimer;
  nsRefPtr<nsNativeThemeCocoa> mTheme;

  static void RedrawIconCallback(nsITimer* aTimer, void* aClosure);

  bool InitProgress();
  nsresult RedrawIcon();
};
