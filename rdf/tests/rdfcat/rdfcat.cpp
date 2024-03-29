














#include <stdio.h>
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIIOService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSource.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsRDFCID.h"
#include "nsThreadUtils.h"
#include "plstr.h"
#include "prio.h"
#include "prthread.h"





static NS_DEFINE_CID(kRDFXMLDataSourceCID,  NS_RDFXMLDATASOURCE_CID);



#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf(">>> %s failed: rv=%x\n", step, static_cast<uint32_t>(rv)); \
        return 1;\
    } \
    PR_END_MACRO



class ConsoleOutputStreamImpl : public nsIOutputStream
{
protected:
    virtual ~ConsoleOutputStreamImpl(void) {}

public:
    ConsoleOutputStreamImpl(void) {}

    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Close(void) override {
        return NS_OK;
    }

    NS_IMETHOD Write(const char* aBuf, uint32_t aCount, uint32_t *aWriteCount) override {
        PR_Write(PR_GetSpecialFD(PR_StandardOutput), aBuf, aCount);
        *aWriteCount = aCount;
        return NS_OK;
    }

    NS_IMETHOD
    WriteFrom(nsIInputStream *inStr, uint32_t count, uint32_t *_retval) override {
        NS_NOTREACHED("WriteFrom");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    WriteSegments(nsReadSegmentFun reader, void * closure, uint32_t count, uint32_t *_retval) override {
        NS_NOTREACHED("WriteSegments");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    IsNonBlocking(bool *aNonBlocking) override {
        NS_NOTREACHED("IsNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD Flush(void) override {
        PR_Sync(PR_GetSpecialFD(PR_StandardOutput));
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(ConsoleOutputStreamImpl, nsIOutputStream)



int
main(int argc, char** argv)
{
    nsresult rv;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <url>\n", argv[0]);
        return 1;
    }

    NS_InitXPCOM2(nullptr, nullptr, nullptr);

    
    
    nsCOMPtr<nsIRDFDataSource> ds = do_CreateInstance(kRDFXMLDataSourceCID,
                                                      &rv);
    RETURN_IF_FAILED(rv, "RDF/XML datasource creation");

    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(ds, &rv);
    RETURN_IF_FAILED(rv, "QI to nsIRDFRemoteDataSource");

    rv = remote->Init(argv[1]);
    RETURN_IF_FAILED(rv, "datasource initialization");

    
    rv = remote->Refresh(false);
    RETURN_IF_FAILED(rv, "datasource refresh");

    
    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
    bool done = false;
    while (!done) {
        NS_ENSURE_TRUE(NS_ProcessNextEvent(thread), 1);
        remote->GetLoaded(&done);
    }

    
    
    nsCOMPtr<nsIOutputStream> out =
        do_QueryInterface(new ConsoleOutputStreamImpl, &rv);
    RETURN_IF_FAILED(rv, "creation of console output stream");

    nsCOMPtr<nsIRDFXMLSource> source = do_QueryInterface(ds);
    RETURN_IF_FAILED(rv, "QI to nsIRDFXMLSource");

    rv = source->Serialize(out);
    RETURN_IF_FAILED(rv, "datasoure serialization");

    return 0;
}
