




#include "nsCOMPtr.h"
#include "nsIPrefService.h"
#include "nsIX509CertDB.h"
#include "nsServiceManagerUtils.h"

int
main(int argc, char* argv[])
{
  {
    NS_InitXPCOM2(nullptr, nullptr, nullptr);
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefs) {
      return -1;
    }
    
    
    
    
    
    nsresult rv = prefs->SetBoolPref("intl.locale.matchOS", true);
    if (NS_FAILED(rv)) {
      return -1;
    }
    nsCOMPtr<nsIX509CertDB> certdb(do_GetService(NS_X509CERTDB_CONTRACTID));
    if (!certdb) {
      return -1;
    }
  } 
  
  NS_ShutdownXPCOM(nullptr);
  return 0;
}
