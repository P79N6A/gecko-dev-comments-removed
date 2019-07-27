





#ifndef nsWindowsRegKey_h__
#define nsWindowsRegKey_h__



#include "nsIWindowsRegKey.h"





#define NS_WINDOWSREGKEY_CONTRACTID "@mozilla.org/windows-registry-key;1"





extern "C" nsresult NS_NewWindowsRegKey(nsIWindowsRegKey** aResult);



#ifdef IMPL_LIBXUL


#define NS_WINDOWSREGKEY_CID \
  { 0xa53bc624, 0xd577, 0x4839, \
    { 0xb8, 0xec, 0xbb, 0x50, 0x40, 0xa5, 0x2f, 0xf4 } }

extern nsresult nsWindowsRegKeyConstructor(nsISupports* aOuter,
                                           const nsIID& aIID, void** aResult);

#endif  



#endif  
