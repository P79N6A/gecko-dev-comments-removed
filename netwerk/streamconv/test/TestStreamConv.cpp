




#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "mozilla/Module.h"
#include "nsXULAppAPI.h"
#include "nsIStringStream.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"

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






class EndListener MOZ_FINAL : public nsIStreamListener {
public:
    
    NS_DECL_ISUPPORTS

    EndListener() {};

    
    NS_IMETHOD OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *inStr, 
                               PRUint32 sourceOffset, PRUint32 count)
    {
        nsresult rv;
        PRUint32 read;
        PRUint64 len64;
        rv = inStr->Available(&len64);
        if (NS_FAILED(rv)) return rv;
        PRUint32 len = (PRUint32)NS_MIN(len64, (PRUint64)(PR_UINT32_MAX - 1));

        char *buffer = (char*)nsMemory::Alloc(len + 1);
        if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

        rv = inStr->Read(buffer, len, &read);
        buffer[len] = '\0';
        if (NS_SUCCEEDED(rv)) {
            printf("CONTEXT %p: Received %u bytes and the following data: \n %s\n\n",
                   static_cast<void*>(ctxt), read, buffer);
        }
        nsMemory::Free(buffer);

        return NS_OK;
    }

    
    NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt) { return NS_OK; }

    NS_IMETHOD OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus) { return NS_OK; }
};

NS_IMPL_ISUPPORTS2(EndListener,
                   nsIStreamListener,
                   nsIRequestObserver)





static PRUint32 
saturated(PRUint64 aValue)
{
    return (PRUint32)NS_MIN(aValue, (PRUint64)PR_UINT32_MAX);
}
 
nsresult SendData(const char * aData, nsIStreamListener* aListener, nsIRequest* request) {
    nsresult rv;

    nsCOMPtr<nsIStringInputStream> dataStream
      (do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = dataStream->SetData(aData, strlen(aData));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint64 avail = 0;
    dataStream->Available(&avail);

    PRUint64 offset = 0;
    while (avail > 0) {
        PRUint32 count = saturated(avail);
        rv = aListener->OnDataAvailable(request, nullptr, dataStream,
                                        saturated(offset), count);
        if (NS_FAILED(rv)) return rv;

        offset += count;
        avail -= count;
    }
    return NS_OK;
}
#define SEND_DATA(x) SendData(x, converterListener, request)

static const mozilla::Module::CIDEntry kTestCIDs[] = {
    { &kTestConverterCID, false, NULL, CreateTestConverter },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kTestContracts[] = {
    { NS_ISTREAMCONVERTER_KEY "?from=a/foo&to=b/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=c/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=d/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=c/foo&to=d/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=e/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=f/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=t/foo&to=k/foo", &kTestConverterCID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kTestCategories[] = {
    { NS_ISTREAMCONVERTER_KEY, "?from=a/foo&to=b/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=c/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=d/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=c/foo&to=d/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=e/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=f/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=t/foo&to=k/foo", "x" },
    { NULL }
};

static const mozilla::Module kTestModule = {
    mozilla::Module::kVersion,
    kTestCIDs,
    kTestContracts,
    kTestCategories
};

int
main(int argc, char* argv[])
{
    nsresult rv;
    {
        XRE_AddStaticComponent(&kTestModule);

        nsCOMPtr<nsIServiceManager> servMan;
        NS_InitXPCOM2(getter_AddRefs(servMan), nullptr, nullptr);
    
        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

        nsCOMPtr<nsICategoryManager> catman =
            do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
        nsCString previous;

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
                                      nullptr,   
                                      "text/plain", 
                                      -1);      
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRequest> request(do_QueryInterface(channel));
#endif

        nsCOMPtr<nsIRequest> request;

        
        
        
        nsIStreamListener *dataReceiver = new EndListener();
        NS_ADDREF(dataReceiver);

        
        
        
        nsIStreamListener *converterListener = nullptr;
        rv = StreamConvService->AsyncConvertData(fromStr, toStr,
                                                 dataReceiver, nullptr, &converterListener);
        if (NS_FAILED(rv)) return rv;
        NS_RELEASE(dataReceiver);

        
        
        
        
        rv = converterListener->OnStartRequest(request, nullptr);
        if (NS_FAILED(rv)) return rv;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return rv;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return rv;

        
        rv = converterListener->OnStopRequest(request, nullptr, rv);
        if (NS_FAILED(rv)) return rv;

        NS_RELEASE(converterListener);
#else
        
        nsCOMPtr<nsIInputStream> convertedData;
        rv = StreamConvService->Convert(inputData, fromStr, toStr,
                                        nullptr, getter_AddRefs(convertedData));
        if (NS_FAILED(rv)) return rv;
#endif

        
        while ( gKeepRunning ) {
            if (!NS_ProcessNextEvent(thread))
                break;
        }
    } 
    
    NS_ShutdownXPCOM(nullptr);
    return rv;
}
