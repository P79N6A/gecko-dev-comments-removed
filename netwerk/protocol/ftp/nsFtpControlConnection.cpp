




































#include "nsIOService.h"
#include "nsFTPChannel.h"
#include "nsFtpControlConnection.h"
#include "nsFtpProtocolHandler.h"
#include "prlog.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gFTPLog;
#endif
#define LOG(args)         PR_LOG(gFTPLog, PR_LOG_DEBUG, args)
#define LOG_ALWAYS(args)  PR_LOG(gFTPLog, PR_LOG_ALWAYS, args)





NS_IMPL_ISUPPORTS1(nsFtpControlConnection, nsIInputStreamCallback)

NS_IMETHODIMP
nsFtpControlConnection::OnInputStreamReady(nsIAsyncInputStream *stream)
{
    char data[4096];

    
    PRUint32 avail;
    nsresult rv = stream->Available(&avail);
    if (NS_SUCCEEDED(rv)) {
        if (avail > sizeof(data))
            avail = sizeof(data);

        PRUint32 n;
        rv = stream->Read(data, avail, &n);
        if (NS_SUCCEEDED(rv) && n != avail)
            avail = n;
    }

    
    

    nsRefPtr<nsFtpControlConnectionListener> listener;
    listener.swap(mListener);

    if (!listener)
        return NS_OK;

    if (NS_FAILED(rv)) {
        listener->OnControlError(rv);
    } else {
        listener->OnControlDataAvailable(data, avail);
    }

    return NS_OK;
}

nsFtpControlConnection::nsFtpControlConnection(const nsCSubstring& host,
                                               PRUint32 port)
    : mServerType(0), mSessionId(gFtpHandler->GetSessionId()), mHost(host)
    , mPort(port)
{
    LOG_ALWAYS(("FTP:CC created @%p", this));
}

nsFtpControlConnection::~nsFtpControlConnection() 
{
    LOG_ALWAYS(("FTP:CC destroyed @%p", this));
}

PRBool
nsFtpControlConnection::IsAlive()
{
    if (!mSocket) 
        return PR_FALSE;

    PRBool isAlive = PR_FALSE;
    mSocket->IsAlive(&isAlive);
    return isAlive;
}
nsresult 
nsFtpControlConnection::Connect(nsIProxyInfo* proxyInfo,
                                nsITransportEventSink* eventSink)
{
    if (mSocket)
        return NS_OK;

    
    nsresult rv;
    nsCOMPtr<nsISocketTransportService> sts =
            do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    rv = sts->CreateTransport(nsnull, 0, mHost, mPort, proxyInfo,
                              getter_AddRefs(mSocket)); 
    if (NS_FAILED(rv))
        return rv;

    mSocket->SetQoSBits(gFtpHandler->GetControlQoSBits());

    
    if (eventSink)
        mSocket->SetEventSink(eventSink, NS_GetCurrentThread());

    
    
    
    rv = mSocket->OpenOutputStream(nsITransport::OPEN_BLOCKING, 1024, 1,
                                   getter_AddRefs(mSocketOutput));
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIInputStream> inStream;
    rv = mSocket->OpenInputStream(0,
                                  nsIOService::gDefaultSegmentSize,
                                  nsIOService::gDefaultSegmentCount,
                                  getter_AddRefs(inStream));
    if (NS_SUCCEEDED(rv))
        mSocketInput = do_QueryInterface(inStream);
    
    return rv;
}

nsresult
nsFtpControlConnection::WaitData(nsFtpControlConnectionListener *listener)
{
    LOG(("FTP:(%p) wait data [listener=%p]\n", this, listener));

    
    
    if (!listener) {
        mListener = nsnull;
        return NS_OK;
    }

    NS_ENSURE_STATE(mSocketInput);

    mListener = listener;
    return mSocketInput->AsyncWait(this, 0, 0, NS_GetCurrentThread());
}

nsresult 
nsFtpControlConnection::Disconnect(nsresult status)
{
    if (!mSocket)
        return NS_OK;  
    
    LOG_ALWAYS(("FTP:(%p) CC disconnecting (%x)", this, status));

    if (NS_FAILED(status)) {
        
        mSocket->Close(status);
        mSocket = 0;
        mSocketInput->AsyncWait(nsnull, 0, 0, nsnull);  
        mSocketInput = nsnull;
        mSocketOutput = nsnull;
    }

    return NS_OK;
}

nsresult 
nsFtpControlConnection::Write(const nsCSubstring& command)
{
    NS_ENSURE_STATE(mSocketOutput);

    PRUint32 len = command.Length();
    PRUint32 cnt;
    nsresult rv = mSocketOutput->Write(command.Data(), len, &cnt);

    if (NS_FAILED(rv))
        return rv;

    if (len != cnt)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}
