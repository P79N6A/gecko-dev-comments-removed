




#include "nsCOMPtr.h"
#include "nsIX509CertDB.h"
#include "nsServiceManagerUtils.h"

int
main(int argc, char* argv[])
{
  {
    NS_InitXPCOM2(nullptr, nullptr, nullptr);
    nsCOMPtr<nsIX509CertDB> certdb(do_GetService(NS_X509CERTDB_CONTRACTID));
    if (!certdb) {
      return -1;
    }
  } 
  
  NS_ShutdownXPCOM(nullptr);
  return 0;
}
