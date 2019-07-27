




#include "nsIServiceManager.h"
#include "nsIStreamConverterService.h"
#include "nsIStreamConverter.h"
#include "nsICategoryManager.h"
#include "mozilla/Module.h"
#include "nsXULAppAPI.h"
#include "nsIStringStream.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "nsMemory.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIRequest.h"
#include "nsNetCID.h"

#define ASYNC_TEST




#ifdef XP_WIN
#include <windows.h>
#endif

static int gKeepRunning = 0;








#include "Converters.h"


static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);






class EndListener final : public nsIStreamListener {
    ~EndListener() {}
public:
    
    NS_DECL_ISUPPORTS

    EndListener() {}

    
    NS_IMETHOD OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *inStr, 
                               uint64_t sourceOffset, uint32_t count) override
    {
        nsresult rv;
        uint32_t read;
        uint64_t len64;
        rv = inStr->Available(&len64);
        if (NS_FAILED(rv)) return rv;
        uint32_t len = (uint32_t)std::min(len64, (uint64_t)(UINT32_MAX - 1));

        char *buffer = (char*)moz_xmalloc(len + 1);
        if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

        rv = inStr->Read(buffer, len, &read);
        buffer[len] = '\0';
        if (NS_SUCCEEDED(rv)) {
            printf("CONTEXT %p: Received %u bytes and the following data: \n %s\n\n",
                   static_cast<void*>(ctxt), read, buffer);
        }
        free(buffer);

        return NS_OK;
    }

    
    NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt) override { return NS_OK; }

    NS_IMETHOD OnStopRequest(nsIRequest* request, nsISupports *ctxt, 
                             nsresult aStatus) override { return NS_OK; }
};

NS_IMPL_ISUPPORTS(EndListener,
                  nsIStreamListener,
                  nsIRequestObserver)





nsresult SendData(const char * aData, nsIStreamListener* aListener, nsIRequest* request) {
    nsresult rv;

    nsCOMPtr<nsIStringInputStream> dataStream
      (do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = dataStream->SetData(aData, strlen(aData));
    NS_ENSURE_SUCCESS(rv, rv);

    uint64_t avail = 0;
    dataStream->Available(&avail);

    uint64_t offset = 0;
    while (avail > 0) {
        uint32_t count = saturated(avail);
        rv = aListener->OnDataAvailable(request, nullptr, dataStream,
                                        offset, count);
        if (NS_FAILED(rv)) return rv;

        offset += count;
        avail -= count;
    }
    return NS_OK;
}
#define SEND_DATA(x) SendData(x, converterListener, request)

static const mozilla::Module::CIDEntry kTestCIDs[] = {
    { &kTestConverterCID, false, nullptr, CreateTestConverter },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kTestContracts[] = {
    { NS_ISTREAMCONVERTER_KEY "?from=a/foo&to=b/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=c/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=b/foo&to=d/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=c/foo&to=d/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=e/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=d/foo&to=f/foo", &kTestConverterCID },
    { NS_ISTREAMCONVERTER_KEY "?from=t/foo&to=k/foo", &kTestConverterCID },
    { nullptr }
};

static const mozilla::Module::CategoryEntry kTestCategories[] = {
    { NS_ISTREAMCONVERTER_KEY, "?from=a/foo&to=b/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=c/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=b/foo&to=d/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=c/foo&to=d/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=e/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=d/foo&to=f/foo", "x" },
    { NS_ISTREAMCONVERTER_KEY, "?from=t/foo&to=k/foo", "x" },
    { nullptr }
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
        if (NS_FAILED(rv)) return -1;
        nsCString previous;

        nsCOMPtr<nsIStreamConverterService> StreamConvService =
                 do_GetService(kStreamConverterServiceCID, &rv);
        if (NS_FAILED(rv)) return -1;

        
        static const char fromStr[] = "a/foo";
        static const char toStr[] = "c/foo";
    
#ifdef ASYNC_TEST
        

        
        
        
        
        
#if 0
        nsCOMPtr<nsIChannel> channel;
        nsCOMPtr<nsIURI> dummyURI;
        rv = NS_NewURI(getter_AddRefs(dummyURI), "http://meaningless");
        if (NS_FAILED(rv)) return -1;

        rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                      dummyURI,
                                      nullptr,   
                                      "text/plain", 
                                      -1);      
        if (NS_FAILED(rv)) return -1;

        nsCOMPtr<nsIRequest> request(do_QueryInterface(channel));
#endif

        nsCOMPtr<nsIRequest> request;

        
        
        
        nsIStreamListener *dataReceiver = new EndListener();
        NS_ADDREF(dataReceiver);

        
        
        
        nsIStreamListener *converterListener = nullptr;
        rv = StreamConvService->AsyncConvertData(fromStr, toStr,
                                                 dataReceiver, nullptr, &converterListener);
        if (NS_FAILED(rv)) return -1;
        NS_RELEASE(dataReceiver);

        
        
        
        
        rv = converterListener->OnStartRequest(request, nullptr);
        if (NS_FAILED(rv)) return -1;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return -1;

        rv = SEND_DATA("aaa");
        if (NS_FAILED(rv)) return -1;

        
        rv = converterListener->OnStopRequest(request, nullptr, rv);
        if (NS_FAILED(rv)) return -1;

        NS_RELEASE(converterListener);
#else
        
        nsCOMPtr<nsIInputStream> convertedData;
        rv = StreamConvService->Convert(inputData, fromStr, toStr,
                                        nullptr, getter_AddRefs(convertedData));
        if (NS_FAILED(rv)) return -1;
#endif

        
        while ( gKeepRunning ) {
            if (!NS_ProcessNextEvent(thread))
                break;
        }
    } 
    
    NS_ShutdownXPCOM(nullptr);
    return 0;
}
