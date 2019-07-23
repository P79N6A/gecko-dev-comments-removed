






































#ifndef NSS_HELPER_
#define NSS_HELPER_

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "pk11func.h"





class PipUIContext : public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  PipUIContext();
  virtual ~PipUIContext();

};






nsresult 
getNSSDialogs(void **_result, REFNSIID aIID, const char *contract);

extern "C" {

PRBool
pip_ucs2_ascii_conversion_fn(PRBool toUnicode,
                             unsigned char *inBuf,
                             unsigned int inBufLen,
                             unsigned char *outBuf,
                             unsigned int maxOutBufLen,
                             unsigned int *outBufLen,
                             PRBool swapBytes);
}




nsresult
setPassword(PK11SlotInfo *slot, nsIInterfaceRequestor *ctx);

#ifdef XP_MAC
extern OSErr ConvertMacPathToUnixPath(const char *macPath, char **unixPath);
#endif

#endif

