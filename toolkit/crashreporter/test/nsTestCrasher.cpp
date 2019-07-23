#include "nsServiceManagerUtils.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsITestCrasher.h"
#include "nsXULAppAPI.h"

class nsTestCrasher : public nsITestCrasher
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITESTCRASHER

  nsTestCrasher() {}

private:
  ~nsTestCrasher() {};
};


NS_IMPL_ISUPPORTS1(nsTestCrasher, nsITestCrasher)


NS_IMETHODIMP nsTestCrasher::Crash()
{
  volatile int* foo = (int*)0x42;
  *foo = 0;
  
  return NS_OK;
}


NS_IMETHODIMP nsTestCrasher::LockDir(nsILocalFile *directory,
                                     nsISupports **_retval NS_OUTPARAM)
{
  return XRE_LockProfileDirectory(directory, _retval);
}


#define NS_TESTCRASHER_CID \
{ 0x54afce51, 0x38d7, 0x4df0, {0x97, 0x50, 0x2f, 0x90, 0xf9, 0xff, 0xbc, 0xa2} }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsTestCrasher)

static const nsModuleComponentInfo components[] = {
    { "Test Crasher",
      NS_TESTCRASHER_CID,
      "@mozilla.org/testcrasher;1",
      nsTestCrasherConstructor
    }
};

NS_IMPL_NSGETMODULE(nsTestCrasherModule, components)
