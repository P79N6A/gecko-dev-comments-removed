



#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsISocketTransportService.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsITransport.h"
#include "nsIRequest.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);
static NS_DEFINE_CID(kEventQueueServiceCID,      NS_EVENTQUEUESERVICE_CID);

static int gKeepRunning = 1;

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


NS_IMPL_ISUPPORTS(InputTestConsumer, nsIRequestObserver, nsIStreamListener)


NS_IMETHODIMP
InputTestConsumer::OnStartRequest(nsIRequest *request, nsISupports* context)
{
  printf("+++ OnStartRequest +++\n");
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
  while (aLength > 0) {
    uint32_t amt;
    aIStream->Read(buf, 1024, &amt);
    if (amt == 0) break;
    buf[amt] = '\0';
    printf(buf);
    aLength -= amt;
  }

  return NS_OK;
}


NS_IMETHODIMP
InputTestConsumer::OnStopRequest(nsIRequest *request, nsISupports* context,
                                 nsresult aStatus)
{
  gKeepRunning = 0;
  printf("+++ OnStopRequest status %x +++\n", aStatus);
  return NS_OK;
}


int
main(int argc, char* argv[])
{
  nsresult rv;

  if (argc < 2) {
      printf("usage: %s <host>\n", argv[0]);
      return -1;
  }

  int port;
  char* hostName = argv[1];



  port = 13;
  {
    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nullptr, nullptr);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    if (registrar)
      registrar->AutoRegister(nullptr);

    
    nsCOMPtr<nsIEventQueueService> eventQService =
             do_GetService(kEventQueueServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIEventQueue> eventQ;
    rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD, getter_AddRefs(eventQ));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISocketTransportService> sts =
             do_GetService(kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsITransport* transport;

    rv = sts->CreateTransport(hostName, port, nullptr, 0, 0, &transport);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIRequest> request;
      transport->AsyncRead(new InputTestConsumer, nullptr, 0, -1, 0, getter_AddRefs(request));

      NS_RELEASE(transport);
    }

    
    while ( gKeepRunning ) {
      PLEvent *gEvent;
      eventQ->WaitForEvent(&gEvent);
      eventQ->HandleEvent(gEvent);
    }

  } 
  
  rv = NS_ShutdownXPCOM(nullptr);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
  return 0;
}

