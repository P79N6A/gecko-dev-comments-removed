









































#ifndef xpctest_private_h___
#define xpctest_private_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsMemory.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "mozilla/ModuleUtils.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include <stdio.h>

#include "jsapi.h"

#define NS_XPCTESTOBJECTREADONLY_CID \
  {0x1364941e, 0x4462, 0x11d3, \
    { 0x82, 0xee, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTOBJECTREADWRITE_CID \
  {0x3b9b1d38, 0x491a, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}


class xpctest
{
public:
  static nsresult ConstructXPCTestObjectReadOnly(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static nsresult ConstructXPCTestObjectReadWrite(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    xpctest();  
};

#endif 
