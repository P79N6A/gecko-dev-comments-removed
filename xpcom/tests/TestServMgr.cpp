





































#include "MyService.h"
#include "nsXPCOM.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsServiceManagerUtils.h"
#include <stdio.h>

static NS_DEFINE_CID(kIMyServiceCID, NS_IMYSERVICE_CID);



IMyService* myServ = NULL;

nsresult
BeginTest(int testNumber)
{
    nsresult err;
    NS_ASSERTION(myServ == NULL, "myServ not reset");
    err = CallGetService(kIMyServiceCID, &myServ);
    return err;
}

nsresult
EndTest(int testNumber)
{
    nsresult err = NS_OK;

    if (myServ) {
        err = myServ->Doit();
        if (err != NS_OK) return err;

        NS_RELEASE(myServ);
    }
    
    printf("test %d succeeded\n", testNumber);
    return NS_OK;
}

nsresult
SimpleTest(int testNumber)
{
    

    nsresult err;
    err = BeginTest(testNumber);
    if (err != NS_OK) return err;
    err = EndTest(testNumber);
    return err;
}



nsresult
AsyncShutdown(int testNumber)
{
    nsresult err = NS_OK;

    
    
    

    

    







    return err;
}

nsresult
AsyncNoShutdownTest(int testNumber)
{
    
    
    
    
    
    

    nsresult err;

    err = BeginTest(testNumber);
    if (err != NS_OK) return err;

    
    
    IMyService* otherClient;
    err = CallGetService(kIMyServiceCID, &otherClient);
    if (err != NS_OK) return err;

    err = AsyncShutdown(testNumber);
    if (err != NS_OK) return err;
    err = EndTest(testNumber);

    
    NS_RELEASE(otherClient);

    return err;
}


int
main(void)
{
    nsresult err;
    int testNumber = 0;

    err = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if (NS_FAILED(err)) {
        printf("NS_InitXPCOM2 failed\n");
        return -1;
    }

    err = SimpleTest(++testNumber);
    if (err != NS_OK)
        goto error;

    err = AsyncNoShutdownTest(++testNumber);
    if (err != NS_OK)
        goto error;

    AsyncShutdown(++testNumber);

    printf("there was great success\n");
    return 0;

  error:
    printf("test %d failed\n", testNumber);
    return -1;
}


