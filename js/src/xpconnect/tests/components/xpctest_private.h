









































#ifndef xpctest_private_h___
#define xpctest_private_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsMemory.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsVariant.h"
#include <stdio.h>

#include "xpctest.h"
#include "jsapi.h"

#if defined(WIN32) && !defined(XPCONNECT_STANDALONE)
#define IMPLEMENT_TIMER_STUFF 1
#endif

#ifdef IMPLEMENT_TIMER_STUFF
#include "nsITimer.h"
#endif 


#define NS_ECHO_CID \
{ 0xed132c20, 0xeed1, 0x11d2, \
    { 0xba, 0xa4, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }


#define NS_CHILD_CID \
{ 0xecb3420, 0xd6f, 0x11d3, \
    { 0xba, 0xb8, 0x0, 0x80, 0x5f, 0x8a, 0x5d, 0xd7 } }


#define NS_NOISY_CID \
{ 0xfd774840, 0x237b, 0x11d3, \
    { 0x98, 0x79, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }


#define NS_STRING_TEST_CID \
{ 0x4dd7ec80, 0x30d9, 0x11d3,\
    { 0x98, 0x85, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }


#define NS_OVERLOADED_CID \
{ 0xdc5fde90, 0x439d, 0x11d3, \
    { 0x98, 0x8c, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }

#define NS_XPCTESTOBJECTREADONLY_CID \
  {0x1364941e, 0x4462, 0x11d3, \
    { 0x82, 0xee, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTOBJECTREADWRITE_CID \
  {0x3b9b1d38, 0x491a, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTIN_CID \
  {0xa3cab49d, 0xae83, 0x4e63, \
    { 0xa7, 0x35, 0x00, 0x9b, 0x9a, 0x75, 0x92, 0x04 }}

#define NS_XPCTESTOUT_CID \
  {0x4105ae88, 0x5599, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTINOUT_CID \
  { 0x70c54fa0, 0xc25e, 0x11d3, \
    { 0x98, 0xc9, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }

#define NS_XPCTESTCONST_CID \
  {0x83f57a56, 0x4f55, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTCALLJS_CID \
  {0x38ba7d98, 0x5a54, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTPARENTONE_CID \
  {0x5408fdcc, 0x60a3, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTPARENTTWO_CID \
  {0x63137392, 0x60a3, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTCHILD2_CID \
  {0x66bed216, 0x60a3, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTCHILD3_CID \
  {0x62353978, 0x614e, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTCHILD4_CID \
  {0xa6d22202, 0x622b, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}

#define NS_XPCTESTCHILD5_CID \
  {0xba3eef4e, 0x6250, 0x11d3, \
    { 0x82, 0xef, 0x00, 0x60, 0xb0, 0xeb, 0x59, 0x6f }}


#define NS_ARRAY_CID \
{ 0x5b9af380, 0x6569, 0x11d3, \
    { 0x98, 0x9e, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } }


#define NS_XPCTESTDOMSTRING_CID \
  {0xdb569f7e, 0x16fb, 0x1bcb, \
    { 0xa8, 0x6c, 0xe0, 0x8a, 0xa7, 0xf9, 0x76, 0x66 }}


#define NS_XPCTESTVARIANT_CID \
  {0xdc932d30, 0x95b0, 0x11d5, \
    { 0x90, 0xfc, 0x0, 0x10, 0xa4, 0xe7, 0x3d, 0x9a }}


class xpctest
{
public:
  static NS_METHOD ConstructEcho(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructChild(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructNoisy(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructStringTest(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructOverloaded(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestObjectReadOnly(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestObjectReadWrite(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestIn(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestOut(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestInOut(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestConst(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestCallJS(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestParentOne(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestParentTwo(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestChild2(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestChild3(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestChild4(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestChild5(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructArrayTest(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestDOMString(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  static NS_METHOD ConstructXPCTestVariant(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    xpctest();  
};

#endif 
