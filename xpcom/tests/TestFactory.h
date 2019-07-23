





































#ifndef __TestFactory_h
#define __TestFactory_h

#include "nsIFactory.h"


#define NS_TESTFACTORY_CID    \
{ 0x8b330f20, 0xa24a, 0x11d1, \
  { 0xa9, 0x61, 0x0, 0x80, 0x5f, 0x8a, 0x7a, 0xc4 } }


#define NS_ITESTCLASS_IID     \
{ 0x8b330f21, 0xa24a, 0x11d1, \
  { 0xa9, 0x61, 0x0, 0x80, 0x5f, 0x8a, 0x7a, 0xc4 } }


#define NS_TESTLOADEDFACTORY_CID \
{ 0x8b330f22, 0xa24a, 0x11d1,    \
  { 0xa9, 0x61, 0x0, 0x80, 0x5f, 0x8a, 0x7a, 0xc4 } }

#define NS_TESTLOADEDFACTORY_CONTRACTID "@mozilla.org/xpcom/dynamic-test;1"

class ITestClass: public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITESTCLASS_IID)
  virtual void Test() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(ITestClass, NS_ITESTCLASS_IID)

extern "C" void RegisterTestFactories();

#endif
