




#include "TestCommon.h"
#include <algorithm>
#ifdef WIN32 
#include <windows.h>
#endif

#include "nsIComponentRegistrar.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"

#include "nsIUploadChannel.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

#include "prlog.h"
#if defined(PR_LOGGING)



static PRLogModuleInfo *gTestLog = nullptr;
#endif
#define LOG(args) PR_LOG(gTestLog, PR_LOG_DEBUG, args)





class InputTestConsumer : public nsIStreamListener
{
public:

  InputTestConsumer();
  virtual ~InputTestConsumer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
};

InputTestConsumer::InputTestConsumer()
{
}

InputTestConsumer::~InputTestConsumer()
{
}

NS_IMPL_ISUPPORTS2(InputTestConsumer,
                   nsIStreamListener,
                   nsIRequestObserver)

NS_IMETHODIMP
InputTestConsumer::OnStartRequest(nsIRequest *request, nsISupports* context)
{
  LOG(("InputTestConsumer::OnStartRequest\n"));
  return NS_OK;
}

NS_IMETHODIMP
InputTestConsumer::OnDataAvailable(nsIRequest *request, 
                                   nsISupports* context,
                                   nsIInputStream *aIStream, 
                                   uint64_t aSourceOffset,
                                   uint32_t aLength)
{
  char buf[1025];
  uint32_t amt, size;
  nsresult rv;

  while (aLength) {
    size = std::min<uint32_t>(aLength, sizeof(buf));
    rv = aIStream->Read(buf, size, &amt);
    if (NS_FAILED(rv)) {
      NS_ASSERTION((NS_BASE_STREAM_WOULD_BLOCK != rv), 
                   "The stream should never block.");
      return rv;
    }
    aLength -= amt;
  }
  return NS_OK;
}


NS_IMETHODIMP
InputTestConsumer::OnStopRequest(nsIRequest *request, nsISupports* context,
                                 nsresult aStatus)
{
    LOG(("InputTestConsumer::OnStopRequest [status=%x]\n", aStatus));
    QuitPumpingEvents();
    return NS_OK;
}


int
main(int argc, char* argv[])
{
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    nsresult rv;

    if (argc < 2) {
        printf("usage: %s <url> <file-to-upload>\n", argv[0]);
        return -1;
    }
    char* uriSpec  = argv[1];
    char* fileName = argv[2];

#if defined(PR_LOGGING) 
    gTestLog = PR_NewLogModule("Test");
#endif

    {
        nsCOMPtr<nsIServiceManager> servMan;
        NS_InitXPCOM2(getter_AddRefs(servMan), nullptr, nullptr);

        
        
        nsCOMPtr<nsIInputStream> uploadStream;
        rv = NS_NewPostDataStream(getter_AddRefs(uploadStream),
                                  true,
                                  nsDependentCString(fileName)); 
        if (NS_FAILED(rv)) return -1;

        nsCOMPtr<nsIIOService> ioService(do_GetService(kIOServiceCID, &rv));

        
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), uriSpec);
        if (NS_FAILED(rv)) return -1;

        nsCOMPtr<nsIChannel> channel;
        rv = ioService->NewChannelFromURI(uri, getter_AddRefs(channel));
        if (NS_FAILED(rv)) return -1;
	
        
        nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(channel));
        uploadChannel->SetUploadStream(uploadStream, EmptyCString(), -1);

        
        InputTestConsumer* listener;

        listener = new InputTestConsumer;
        if (!listener) {
            NS_ERROR("Failed to create a new stream listener!");
            return -1;
        }
        NS_ADDREF(listener);

        channel->AsyncOpen(listener, nullptr);

        PumpEvents();
    } 
    
    rv = NS_ShutdownXPCOM(nullptr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

    return 0;
}

