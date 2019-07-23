





































#include <windows.h>
#include <unknwn.h>
#include <stdio.h>
#include "nsISupports.h"
#include "nsIFactory.h"


#include <unknwn.h>


#define NS_ITEST_COM_IID \
{ 0x5846ba30, 0xb856, 0x11d1, \
  { 0xa9, 0x8a, 0x0, 0x80, 0x5f, 0x8a, 0x7a, 0xc4 } }

class nsITestCom: public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEST_COM_IID)
  NS_IMETHOD Test() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITestCom, NS_ITEST_COM_IID)





class nsTestCom: public nsITestCom {
  NS_DECL_ISUPPORTS

public:
  nsTestCom() {
  }

  NS_IMETHOD Test() {
    printf("Accessed nsITestCom::Test() from COM\n");
    return NS_OK;
  }

private:
  ~nsTestCom() {
    printf("nsTestCom instance successfully deleted\n");
  }
};

NS_IMPL_QUERY_INTERFACE1(nsTestCom, nsITestCom)

nsrefcnt nsTestCom::AddRef() 
{
  nsrefcnt res = ++mRefCnt;
  NS_LOG_ADDREF(this, mRefCnt, "nsTestCom", sizeof(*this));
  printf("nsTestCom: Adding ref = %d\n", res);
  return res;
}

nsrefcnt nsTestCom::Release() 
{
  nsrefcnt res = --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsTestCom");
  printf("nsTestCom: Releasing = %d\n", res);
  if (res == 0) {
    delete this;
  }
  return res;
}

class nsTestComFactory: public nsIFactory {
  NS_DECL_ISUPPORTS
public:
  nsTestComFactory() {
  }
  
  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock) {
    printf("nsTestComFactory: ");
    printf("%s", (aLock == PR_TRUE ? "Locking server" : "Unlocking server"));
    printf("\n");
    return S_OK;
  }
};

NS_IMPL_ISUPPORTS1(nsTestComFactory, nsIFactory)

nsresult nsTestComFactory::CreateInstance(nsISupports *aOuter,
					  const nsIID &aIID,
					  void **aResult)
{
  if (aOuter != NULL) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsTestCom *t = new nsTestCom();
  
  if (t == NULL) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(t);
  nsresult res = t->QueryInterface(aIID, aResult);
  NS_RELEASE(t);

  if (NS_SUCCEEDED(res)) {
    printf("nsTestComFactory: successfully created nsTestCom instance\n");
  }

  return res;
}





int main(int argc, char *argv[])
{
  nsTestComFactory *inst = new nsTestComFactory();
  IClassFactory *iFactory;
  inst->QueryInterface(NS_GET_IID(nsIFactory), (void **) &iFactory);

  IUnknown *iUnknown;  
  nsITestCom *iTestCom;

  nsresult res;
  iFactory->LockServer(TRUE);
  res = iFactory->CreateInstance(NULL,
				 IID_IUnknown, 
				 (void **) &iUnknown);
  iFactory->LockServer(FALSE);

  GUID testGUID = NS_ITEST_COM_IID;
  HRESULT hres;
  hres= iUnknown->QueryInterface(testGUID, 
				 (void **) &iTestCom);

  iTestCom->Test();

  iUnknown->Release();
  iTestCom->Release();
  iFactory->Release();

  return 0;
}

