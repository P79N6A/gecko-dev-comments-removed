





































#include "nsIServiceManager.h"



void main(int argc, char* argv[])
{
    nsresult rv;
    nsIServiceManager* servMgr;
    rv = NS_InitXPCOM2(&servMgr, NULL, NULL);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_InitXPCOM failed");

    
    if (argc > 1 && argv[1] != nsnull) {
        char* cidStr = argv[1];
        nsISupports* obj = nsnull;
        if (cidStr[0] == '{') {
            nsCID cid;
            cid.Parse(cidStr);
            rv = CallCreateInstance(cid, &obj);
        }
        else {
            
            rv = CallCreateInstance(cidStr, &obj);
        }
        if (NS_SUCCEEDED(rv)) {
            printf("Successfully created %s\n", cidStr);
            NS_RELEASE(obj);
        }
        else {
            printf("Failed to create %s (%x)\n", cidStr, rv);
        }
    }

    rv = NS_ShutdownXPCOM(servMgr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
}
