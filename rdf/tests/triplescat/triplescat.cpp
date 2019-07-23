














































#include <stdio.h>
#include "nsXPCOM.h"
#include "nsCOMPtr.h"

#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
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
#include "plstr.h"
#include "prio.h"
#include "prthread.h"

#include "rdf.h"
#include "rdfIDataSource.h"
#include "rdfITripleVisitor.h"
#include "rdfISerializer.h"



#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf(">>> %s failed: rv=%x\n", step, rv); \
        return rv;\
    } \
    PR_END_MACRO



class ConsoleOutputStreamImpl : public nsIOutputStream
{
public:
    ConsoleOutputStreamImpl(void) {}
    virtual ~ConsoleOutputStreamImpl(void) {}

    
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD Close(void) {
        return NS_OK;
    }

    
    NS_IMETHOD Write(const char* aBuf, PRUint32 aCount, PRUint32 *aWriteCount) {
        PR_Write(PR_GetSpecialFD(PR_StandardOutput), aBuf, aCount);
        *aWriteCount = aCount;
        return NS_OK;
    }

    NS_IMETHOD
    WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteFrom");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval) {
        NS_NOTREACHED("WriteSegments");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD
    IsNonBlocking(PRBool *aNonBlocking) {
        NS_NOTREACHED("IsNonBlocking");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD Flush(void) {
        PR_Sync(PR_GetSpecialFD(PR_StandardOutput));
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(ConsoleOutputStreamImpl, nsIOutputStream)



int
main(int argc, char** argv)
{
    nsresult rv;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <url>\n", argv[0]);
        return 1;
    }

    NS_InitXPCOM2(nsnull, nsnull, nsnull);

    
    
    nsCOMPtr<nsIRDFDataSource> ds =
        do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX "xml-datasource",
                          &rv);
    RETURN_IF_FAILED(rv, "RDF/XML datasource creation");

    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(ds, &rv);
    RETURN_IF_FAILED(rv, "QI to nsIRDFRemoteDataSource");

    rv = remote->Init(argv[1]);
    RETURN_IF_FAILED(rv, "datasource initialization");

    
    rv = remote->Refresh(PR_FALSE);
    RETURN_IF_FAILED(rv, "datasource refresh");

    
    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
    PRBool done = PR_FALSE;
    while (!done) {
        NS_ENSURE_STATE(NS_ProcessNextEvent(thread));
        remote->GetLoaded(&done);
    }

    nsCOMPtr<rdfIDataSource> rdfds = do_QueryInterface(ds, &rv);
    RETURN_IF_FAILED(rv, "QI to rdIDataSource");
    {
        nsCOMPtr<nsIOutputStream> out = new ConsoleOutputStreamImpl();
        nsCOMPtr<rdfISerializer> ser =
            do_CreateInstance(NS_RDF_SERIALIZER "ntriples", &rv);
        RETURN_IF_FAILED(rv, "Creation of NTriples Serializer");
        rv = ser->Serialize(rdfds, out);
        RETURN_IF_FAILED(rv, "Serialization to NTriples");
        out->Close();
    }

    return NS_OK;
}
