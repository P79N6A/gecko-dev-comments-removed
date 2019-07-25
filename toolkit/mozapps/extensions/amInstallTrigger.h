




































#include "jscntxt.h"
#include "amIInstallTrigger.h"
#include "nsIDOMWindowInternal.h"
#include "nsIURI.h"
#include "amIWebInstaller.h"
#include "nsCOMPtr.h"

#define AM_InstallTrigger_CID \
 {0xfcfcdf1e, 0xe9ef, 0x4141, {0x90, 0xd8, 0xd5, 0xff, 0x84, 0xc1, 0x7c, 0xce}}
#define AM_INSTALLTRIGGER_CONTRACTID "@mozilla.org/addons/installtrigger;1"

class amInstallTrigger : public amIInstallTrigger
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_AMIINSTALLTRIGGER

  amInstallTrigger();

private:
  ~amInstallTrigger();

  JSContext* GetJSContext();
  already_AddRefed<nsIDOMWindowInternal> GetOriginatingWindow(JSContext* aCx);
  already_AddRefed<nsIURI> GetOriginatingURI(nsIDOMWindowInternal* aWindow);

  nsCOMPtr<amIWebInstaller> mManager;
};
