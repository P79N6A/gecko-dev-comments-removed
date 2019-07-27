




#include "nsIOService.h"
#include "nsFtpControlConnection.h"
#include "nsFtpProtocolHandler.h"
#include "prlog.h"
#include "nsIInputStream.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsThreadUtils.h"
#include "nsIOutputStream.h"
#include "nsNetCID.h"
#include <algorithm>

extern PRLogModuleInfo* gFTPLog;
#define LOG(args)         PR_LOG(gFTPLog, PR_LOG_DEBUG, args)
#define LOG_ALWAYS(args)  PR_LOG(gFTPLog, PR_LOG_ALWAYS, args)





NS_IMPL_ISUPPORTS(nsFtpControlConnection, nsIInputStreamCallback)

NS_IMETHODIMP
nsFtpControlConnection::OnInputStreamReady(nsIAsyncInputStream *stream)
{
    char data[4096];

    
    uint64_t avail64;
    uint32_t avail = 0;
    nsresult rv = stream->Available(&avail64);
    if (NS_SUCCEEDED(rv)) {
        avail = (uint32_t)std::min(avail64, (uint64_t)sizeof(data));

        uint32_t n;
        rv = stream->Read(data, avail, &n);
        if (NS_SUCCEEDED(rv))
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
                                               uint32_t port)
    : mServerType(0), mSessionId(gFtpHandler->GetSessionId())
    , mUseUTF8(false), mHost(host), mPort(port)
{
    LOG_ALWAYS(("FTP:CC created @%p", this));
}

nsFtpControlConnection::~nsFtpControlConnection() 
{
    LOG_ALWAYS(("FTP:CC destroyed @%p", this));
}

bool
nsFtpControlConnection::IsAlive()
{
    if (!mSocket) 
        return false;

    bool isAlive = false;
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

    rv = sts->CreateTransport(nullptr, 0, mHost, mPort, proxyInfo,
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
        mListener = nullptr;
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
        mSocketInput->AsyncWait(nullptr, 0, 0, nullptr);  
        mSocketInput = nullptr;
        mSocketOutput = nullptr;
    }

    return NS_OK;
}

nsresult 
nsFtpControlConnection::Write(const nsCSubstring& command)
{
    NS_ENSURE_STATE(mSocketOutput);

    uint32_t len = command.Length();
    uint32_t cnt;
    nsresult rv = mSocketOutput->Write(command.Data(), len, &cnt);

    if (NS_FAILED(rv))
        return rv;

    if (len != cnt)
        return NS_ERROR_FAILURE;
    
    return NS_OK;
}
