




































#include "nsIMacDockSupport.h"
#include "nsIStandaloneNativeMenu.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsMacDockSupport : public nsIMacDockSupport
{
public:
  nsMacDockSupport() {}
  virtual ~nsMacDockSupport() {}
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMACDOCKSUPPORT

protected:
  nsCOMPtr<nsIStandaloneNativeMenu> mDockMenu;
  nsString mBadgeText;
};
