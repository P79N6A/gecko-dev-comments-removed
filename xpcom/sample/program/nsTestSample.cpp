











































#include <stdio.h>

#include "nsXPCOMGlue.h"
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsISample.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"

#define NS_SAMPLE_CONTRACTID "@mozilla.org/sample;1"

int
main(void)
{
    nsresult rv;

    XPCOMGlueStartup(nsnull);

    
    nsCOMPtr<nsIServiceManager> servMan;
    rv = NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    if (NS_FAILED(rv))
    {
        printf("ERROR: XPCOM intialization error [%x].\n", rv);
        return -1;
    }
    
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    registrar->AutoRegister(nsnull);
    
    nsCOMPtr<nsIComponentManager> manager = do_QueryInterface(registrar);
    NS_ASSERTION(registrar, "Null nsIComponentManager");
    
    
    nsCOMPtr<nsISample> mysample;
    rv = manager->CreateInstanceByContractID(NS_SAMPLE_CONTRACTID,
                                             nsnull,
                                             NS_GET_IID(nsISample),
                                             getter_AddRefs(mysample));
    if (NS_FAILED(rv))
    {
        printf("ERROR: Cannot create instance of component " NS_SAMPLE_CONTRACTID " [%x].\n"
               "Debugging hint:\n"
               "\tsetenv NSPR_LOG_MODULES nsComponentManager:5\n"
               "\tsetenv NSPR_LOG_FILE xpcom.log\n"
               "\t./nsTestSample\n"
               "\t<check the contents for xpcom.log for possible cause of error>.\n",
               rv);
        return -2;
    }

    
    rv = mysample->WriteValue("Inital print:");
    if (NS_FAILED(rv))
    {
        printf("ERROR: Calling nsISample::WriteValue() [%x]\n", rv);
        return -3;
    }

    const char *testValue = "XPCOM defies gravity";
    rv = mysample->SetValue(testValue);
    if (NS_FAILED(rv))
    {
        printf("ERROR: Calling nsISample::SetValue() [%x]\n", rv);
        return -3;
    }
    printf("Set value to: %s\n", testValue);
    char *str;
    rv = mysample->GetValue(&str);

    if (NS_FAILED(rv))
    {
        printf("ERROR: Calling nsISample::GetValue() [%x]\n", rv);
        return -3;
    }
    if (strcmp(str, testValue))
    {
        printf("Test FAILED.\n");
        return -4;
    }

    NS_Free(str);

    rv = mysample->WriteValue("Final print :");
    printf("Test passed.\n");
    
    
    
    servMan = 0;
    registrar = 0;
    manager = 0;
    mysample = 0;
    
    
    NS_ShutdownXPCOM(nsnull);

    XPCOMGlueShutdown();
    return 0;
}
