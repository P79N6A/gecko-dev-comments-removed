





































#ifndef nsWindowsRegKey_h__
#define nsWindowsRegKey_h__



#include "nsIWindowsRegKey.h"





#define NS_WINDOWSREGKEY_CONTRACTID "@mozilla.org/windows-registry-key;1"





extern "C" NS_COM nsresult
NS_NewWindowsRegKey(nsIWindowsRegKey **result);



#ifdef _IMPL_NS_COM

#define NS_WINDOWSREGKEY_CLASSNAME "nsWindowsRegKey"


#define NS_WINDOWSREGKEY_CID \
  { 0xa53bc624, 0xd577, 0x4839, \
    { 0xb8, 0xec, 0xbb, 0x50, 0x40, 0xa5, 0x2f, 0xf4 } }

extern NS_METHOD
nsWindowsRegKeyConstructor(nsISupports *outer, const nsIID &iid, void **result);

#endif  



#endif  
