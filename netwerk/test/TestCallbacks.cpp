




































#include "TestCommon.h"
#include <stdio.h>
#ifdef WIN32 
#include <windows.h>
#endif
#include "nspr.h"
#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsIURL.h"
#include "nsIInterfaceRequestor.h" 
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDNSService.h" 

#include "nsISimpleEnumerator.h"
#include "nsNetUtil.h"
#include "nsStringAPI.h"

static NS_DEFINE_CID(kIOServiceCID,              NS_IOSERVICE_CID);

static PRBool gError = PR_FALSE;
static PRInt32 gKeepRunning = 0;

#define NS_IEQUALS_IID \
    { 0x11c5c8ee, 0x1dd2, 0x11b2, \
      { 0xa8, 0x93, 0xbb, 0x23, 0xa1, 0xb6, 0x27, 0x76 }}

class nsIEquals : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEQUALS_IID)
    NS_IMETHOD Equals(void *aPtr, PRBool *_retval) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEquals, NS_IEQUALS_IID)

class ConsumerContext : public nsIEquals {
public:
    NS_DECL_ISUPPORTS

    ConsumerContext() { }

    NS_IMETHOD Equals(void *aPtr, PRBool *_retval) {
        *_retval = PR_TRUE;
        if (aPtr != this) *_retval = PR_FALSE;
        return NS_OK;
    };
};

NS_IMPL_THREADSAFE_ISUPPORTS1(ConsumerContext, nsIEquals)

class Consumer : public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    Consumer();
    virtual ~Consumer();
    nsresult Init(nsIURI *aURI, nsIChannel *aChannel, nsISupports *aContext);
    nsresult Validate(nsIRequest *request, nsISupports *aContext);

    
    PRBool  mOnStart; 
    PRBool  mOnStop;  
    PRInt32 mOnDataCount; 
    nsCOMPtr<nsIURI>     mURI;
    nsCOMPtr<nsIChannel> mChannel;
    nsCOMPtr<nsIEquals>  mContext;
};


NS_IMPL_THREADSAFE_ISUPPORTS2(Consumer, nsIStreamListener, nsIRequestObserver)



NS_IMETHODIMP
Consumer::OnStartRequest(nsIRequest *request, nsISupports* aContext) {
    fprintf(stderr, "Consumer::OnStart() -> in\n\n");

    if (mOnStart) {
        fprintf(stderr, "INFO: multiple OnStarts received\n");
    }
    mOnStart = PR_TRUE;

    nsresult rv = Validate(request, aContext);
    if (NS_FAILED(rv)) return rv;

    fprintf(stderr, "Consumer::OnStart() -> out\n\n");
    return rv;
}

NS_IMETHODIMP
Consumer::OnStopRequest(nsIRequest *request, nsISupports *aContext,
                        nsresult aStatus) {
    fprintf(stderr, "Consumer::OnStop() -> in\n\n");

    if (!mOnStart) {
        gError = PR_TRUE;
        fprintf(stderr, "ERROR: No OnStart received\n");
    }

    if (mOnStop) {
        fprintf(stderr, "INFO: multiple OnStops received\n");
    }

    fprintf(stderr, "INFO: received %d OnData()s\n", mOnDataCount);

    mOnStop = PR_TRUE;

    nsresult rv = Validate(request, aContext);
    if (NS_FAILED(rv)) return rv;

    fprintf(stderr, "Consumer::OnStop() -> out\n\n");
    return rv;
}



NS_IMETHODIMP
Consumer::OnDataAvailable(nsIRequest *request, nsISupports *aContext,
                          nsIInputStream *aIStream,
                          PRUint32 aOffset, PRUint32 aLength) {
    fprintf(stderr, "Consumer::OnData() -> in\n\n");

    if (!mOnStart) {
        gError = PR_TRUE;
        fprintf(stderr, "ERROR: No OnStart received\n");
    }

    mOnDataCount += 1;

    nsresult rv = Validate(request, aContext);
    if (NS_FAILED(rv)) return rv;

    fprintf(stderr, "Consumer::OnData() -> out\n\n");
    return rv;
}


Consumer::Consumer() {
    mOnStart = mOnStop = PR_FALSE;
    mOnDataCount = 0;
    gKeepRunning++;
}

Consumer::~Consumer() {
    fprintf(stderr, "Consumer::~Consumer -> in\n\n");

    if (!mOnStart) {
        gError = PR_TRUE;
        fprintf(stderr, "ERROR: Never got an OnStart\n");
    }

    if (!mOnStop) {
        gError = PR_TRUE;
        fprintf(stderr, "ERROR: Never got an OnStop \n");
    }

    fprintf(stderr, "Consumer::~Consumer -> out\n\n");
    if (--gKeepRunning == 0)
      QuitPumpingEvents();
}

nsresult
Consumer::Init(nsIURI *aURI, nsIChannel* aChannel, nsISupports *aContext) {
    mURI     = aURI;
    mChannel = aChannel;
    mContext = do_QueryInterface(aContext);
    return NS_OK;
}

nsresult
Consumer::Validate(nsIRequest* request, nsISupports *aContext) {
    nsresult rv = NS_OK;
    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(request);

    rv = aChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    PRBool same = PR_FALSE;

    rv = mURI->Equals(uri, &same);
    if (NS_FAILED(rv)) return rv;

    if (!same)
        fprintf(stderr, "INFO: URIs do not match\n");

    rv = mContext->Equals((void*)aContext, &same);
    if (NS_FAILED(rv)) return rv;

    if (!same) {
        gError = PR_TRUE;
        fprintf(stderr, "ERROR: Contexts do not match\n");
    }
    return rv;
}

nsresult StartLoad(const char *);

int main(int argc, char *argv[]) {
    if (test_common_init(&argc, &argv) != 0)
        return -1;

    nsresult rv = NS_OK;
    PRBool cmdLineURL = PR_FALSE;

    if (argc > 1) {
        
        cmdLineURL = PR_TRUE;
    }

    rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if (NS_FAILED(rv)) return rv;

    if (cmdLineURL) {
        rv = StartLoad(argv[1]);
    } else {
        rv = StartLoad("http://badhostnamexyz/test.txt");
    }
    if (NS_FAILED(rv)) return rv;

    
    PumpEvents();

    NS_ShutdownXPCOM(nsnull);
    if (gError) {
        fprintf(stderr, "\n\n-------ERROR-------\n\n");
    }
    return rv;
}

nsresult StartLoad(const char *aURISpec) {
    nsresult rv = NS_OK;

    
    ConsumerContext *context = new ConsumerContext;
    nsCOMPtr<nsISupports> contextSup = do_QueryInterface(context, &rv);
    if (NS_FAILED(rv)) return rv;


    nsCOMPtr<nsIIOService> serv = do_GetService(kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aURISpec);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIChannel> channel;
    rv = serv->NewChannelFromURI(uri, getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    Consumer *consumer = new Consumer;
    rv = consumer->Init(uri, channel, contextSup);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIRequest> request;
    return channel->AsyncOpen(static_cast<nsIStreamListener*>(consumer), contextSup);
}
