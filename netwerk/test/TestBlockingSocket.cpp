



#include "TestCommon.h"
#include "nsIComponentRegistrar.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "mozilla/Logging.h"
#include "prenv.h"
#include "prthread.h"
#include <stdlib.h>






static PRLogModuleInfo *gTestLog = nullptr;
#define LOG(args) MOZ_LOG(gTestLog, mozilla::LogLevel::Debug, args)



static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);



static nsresult
RunBlockingTest(const nsACString &host, int32_t port, nsIFile *file)
{
    nsresult rv;

    LOG(("RunBlockingTest\n"));

    nsCOMPtr<nsISocketTransportService> sts =
        do_GetService(kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIInputStream> input;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(input), file);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISocketTransport> trans;
    rv = sts->CreateTransport(nullptr, 0, host, port, nullptr, getter_AddRefs(trans));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIOutputStream> output;
    rv = trans->OpenOutputStream(nsITransport::OPEN_BLOCKING, 100, 10, getter_AddRefs(output));
    if (NS_FAILED(rv)) return rv;

    char buf[120];
    uint32_t nr, nw;
    for (;;) {
        rv = input->Read(buf, sizeof(buf), &nr);
        if (NS_FAILED(rv) || (nr == 0)) return rv;











        
        rv = output->Write(buf, nr, &nw);
        if (NS_FAILED(rv)) return rv;

        NS_ASSERTION(nr == nw, "not all written");
    }

    LOG(("  done copying data.\n"));
    return NS_OK;
}



int
main(int argc, char* argv[])
{
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    nsresult rv;

    if (argc < 4) {
        printf("usage: %s <host> <port> <file-to-read>\n", argv[0]);
        return -1;
    }
    char* hostName = argv[1];
    int32_t port = atoi(argv[2]);
    char* fileName = argv[3];
    {
        nsCOMPtr<nsIServiceManager> servMan;
        NS_InitXPCOM2(getter_AddRefs(servMan), nullptr, nullptr);

        gTestLog = PR_NewLogModule("Test");

        nsCOMPtr<nsIFile> file;
        rv = NS_NewNativeLocalFile(nsDependentCString(fileName), false, getter_AddRefs(file));
        if (NS_FAILED(rv)) return -1;

        rv = RunBlockingTest(nsDependentCString(hostName), port, file);
        if (NS_FAILED(rv))
            LOG(("RunBlockingTest failed [rv=%x]\n", rv));

        
        
        LOG(("sleeping for 5 seconds...\n"));
        PR_Sleep(PR_SecondsToInterval(5));
    } 
    
    rv = NS_ShutdownXPCOM(nullptr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
    return 0;
}
