





































#include "MyService.h"
#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include <stdio.h>



class MyService : public IMyService {
public:

    NS_IMETHOD
    Doit(void);

    MyService();
    NS_DECL_ISUPPORTS

private:
    ~MyService();
};




NS_IMPL_ISUPPORTS1(MyService, IMyService)

MyService::MyService()
{
    printf("  creating my service\n");
}

MyService::~MyService()
{
    printf("  destroying my service\n");
}

nsresult
MyService::Doit(void)
{
    printf("    invoking my service\n");
    return NS_OK;
}




NS_GENERIC_FACTORY_CONSTRUCTOR(MyService)

static const nsModuleComponentInfo myService_components[] = {
    { "MyService", NS_IMYSERVICE_CID, nsnull, MyServiceConstructor },
};

NS_IMPL_NSGETMODULE(MyService, myService_components)



