





































#include <stdio.h>
#include "nsXPCOM.h"
#include "TestFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"

NS_DEFINE_CID(kTestFactoryCID, NS_TESTFACTORY_CID);
NS_DEFINE_CID(kTestLoadedFactoryCID, NS_TESTLOADEDFACTORY_CID);






class TestClassImpl: public ITestClass {
  NS_DECL_ISUPPORTS
public:
  TestClassImpl() {
  }

  void Test();
};

NS_IMPL_ISUPPORTS1(TestClassImpl, ITestClass)

void TestClassImpl::Test() {
  printf("hello, world!\n");
}





class TestFactory: public nsIFactory {
  NS_DECL_ISUPPORTS
  
public:
  TestFactory() {
  }

  NS_IMETHOD CreateInstance(nsISupports *aDelegate,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock) { return NS_OK; }
};

NS_IMPL_ISUPPORTS1(TestFactory, nsIFactory)

nsresult TestFactory::CreateInstance(nsISupports *aDelegate,
                                     const nsIID &aIID,
                                     void **aResult) {
  if (aDelegate != NULL) {
    return NS_ERROR_NO_AGGREGATION;
  }

  TestClassImpl *t = new TestClassImpl();
  
  if (t == NULL) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult res = t->QueryInterface(aIID, aResult);

  if (NS_FAILED(res)) {
    *aResult = NULL;
    delete t;
  }

  return res;
}


int main(int argc, char **argv) {
  nsresult rv;

  {
    nsCOMPtr<nsIServiceManager> servMan;
    rv = NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    if (NS_FAILED(rv)) return -1;
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    if (registrar)
      registrar->RegisterFactory(kTestFactoryCID,
                                 nsnull,
                                 nsnull,
                                 new TestFactory());

    ITestClass *t = NULL;
    CallCreateInstance(kTestFactoryCID, &t);

    if (t != NULL) {
      t->Test();
      t->Release();
    } else {
      printf("CreateInstance failed\n");
    }

    t = NULL;

    CallCreateInstance(kTestLoadedFactoryCID, &t);

    if (t != NULL) {
      t->Test();
      t->Release();
    } else {
      printf("Dynamic CreateInstance failed\n");
    }
  } 
  
  rv = NS_ShutdownXPCOM(nsnull);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
  return 0;
}

