




































#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "nsIFactory.h"
#include "nsIStringStream.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"

#include "nspr.h"

#define ASYNC_TEST




#ifdef XP_WIN
#include <windows.h>
#endif
#ifdef XP_OS2
#include <os2.h>
#endif

static int gKeepRunning = 0;








#include "Converters.h"


static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);






class EndListener : public nsIStreamListener {
public:
    
    NS_DECL_ISUPPORTS

    EndListener() {};

    
    NS_IMETHOD OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *inStr, 
                               PRUint32 sourceOffset, PRUint32 count)
    {
        nsresult rv;
        PRUint32 read, len;
        rv = inStr->Available(&len);
        if (NS_FAILED(rv)) return rv;

        char *buffer = (char*)nsMemory::Alloc(len + 1);
        if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

        rv = inStr->Read(buffer, len, &read);
        buffer[len] = '\0';
        if (NS_SUCCEEDED(rv)) {
            printf("CONTEXT %p: Received %u bytes and the following data: \n %s\n\n", ctxt, read, buffer);
        }
        nsMemory::Free(buffer);

        return NS_OK;
    }

    
    NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt) { return NS_OK; }

    NS_IMETHOD OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus) { return NS_OK; }
};

NS_IMPL_ISUPPORTS1(EndListener, nsIStreamListener)





nsresult SendData(const char * aData, nsIStreamListener* aListener, nsIRequest* request) {
    nsresult rv;

    nsCOMPtr<nsIStringInputStream> dataStream
      (do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = dataStream->SetData(aData, strlen(aData));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 avail;
    dataStream->Available(&avail);

    return aListener->OnDataAvailable(request, nsnull, dataStream, 0, avail);
}
#define SEND_DATA(x) SendData(x, converterListener, request)

int
main(int argc, char* argv[])
{
    nsresult rv;
    {
        nsCOMPtr<nsIServiceManager> servMan;
        NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
        nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
        NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
        if (registrar)
            registrar->AutoRegister(nsnull);
    
        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

        nsCOMPtr<nsICategoryManager> catman =
            do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
        nsCString previous;

        
        
        
        

        
        
        

        PRUint32 converterListSize = 7;
        const char *const converterList[] = {
            "?from=a/foo&to=b/foo",
            "?from=b/foo&to=c/foo",
            "?from=b/foo&to=d/foo",
            "?from=c/foo&to=d/foo",
            "?from=d/foo&to=e/foo",
            "?from=d/foo&to=f/foo",
            "?from=t/foo&to=k/foo",
        };

        TestConverterFactory *convFactory = new TestConverterFactory(kTestConverterCID, "TestConverter", NS_ISTREAMCONVERTER_KEY);
        nsCOMPtr<nsIFactory> convFactSup(do_QueryInterface(convFactory, &rv));
        if (NS_FAILED(rv)) return rv;

        for (PRUint32 count = 0; count < converterListSize; ++count) {
            
            
            nsCString contractID(NS_ISTREAMCONVERTER_KEY);
            contractID.Append(converterList[count]);
            rv = registrar->RegisterFactory(kTestConverterCID,
                                            "TestConverter",
                                            contractID.get(),
                                            convFactSup);
            if (NS_FAILED(rv)) return rv;
            rv = catman->AddCategoryEntry(NS_ISTREAMCONVERTER_KEY, converterList[count], "x",
                                            PR_TRUE, PR_TRUE, getter_Copies(previous));
            if (NS_FAILED(rv)) return rv;
        }

        nsCOMPtr<nsIStreamConverterService> StreamConvService =
                 do_GetService(kStreamConverterServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        
        static const char fromStr[] = "a/foo";
        static const char toStr[] = "c/foo";
    
#ifdef ASYNC_TEST
        

        
        
        
        
        
#if 0
        nsCOMPtr<nsIChannel> channel;
        nsCOMPtr<nsIURI> dummyURI;
        rv = NS_NewURI(getter_AddRefs(dummyURI), "http://meaningless");
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                      dummyURI,
                                      nsnull,   
                                      "text/plain", 
                                      -1);      
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRequest> request(do_QueryInterface(channel));
#endif

        nsCOMPtr<nsIRequest> request;

        
        
        
        nsIStreamListener *dataReceiver = new EndListener();
        NS_ADDREF(dataReceiver);

        
        
        
        nsIStreamListener *converterListener = nsnull;
        rv = StreamConvService->AsyncConvertData(fromStr, toStr,
                                                 dataReceiver, nsnull, &converterListener);
        if (NS_FAILED(rv)) return rv;
        NS_RELEASE(dataReceiver);

        
        
        
        
        rv = converterListener->OnStartRequest(request, nsnull);
        if (NS_FAILED(rv)) return rv;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return rv;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return rv;

        
        rv = converterListener->OnStopRequest(request, nsnull, rv);
        if (NS_FAILED(rv)) return rv;

        NS_RELEASE(converterListener);
#else
        
        nsCOMPtr<nsIInputStream> convertedData;
        rv = StreamConvService->Convert(inputData, fromStr, toStr,
                                        nsnull, getter_AddRefs(convertedData));
        if (NS_FAILED(rv)) return rv;
#endif

        
        while ( gKeepRunning ) {
            if (!NS_ProcessNextEvent(thread))
                break;
        }
    } 
    
    NS_ShutdownXPCOM(nsnull);
    return rv;
}
