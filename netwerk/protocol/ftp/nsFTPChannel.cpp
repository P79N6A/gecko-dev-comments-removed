





































#include "nsFTPChannel.h"
#include "nsFtpConnectionThread.h"  

#include "nsIStreamListener.h"
#include "nsIServiceManager.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsReadableUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIStreamConverterService.h"
#include "nsISocketTransport.h"
#include "nsURLHelper.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gFTPLog;
#endif 


static inline PRUint32
PRTimeToSeconds(PRTime t_usec)
{
    return PRUint32(t_usec / PR_USEC_PER_SEC);
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())













NS_IMPL_ISUPPORTS_INHERITED4(nsFtpChannel,
                             nsBaseChannel,
                             nsIUploadChannel,
                             nsIResumableChannel,
                             nsIFTPChannel,
                             nsIProxiedChannel)



NS_IMETHODIMP
nsFtpChannel::SetUploadStream(nsIInputStream *stream,
                              const nsACString &contentType,
                              PRInt32 contentLength)
{
    NS_ENSURE_TRUE(!IsPending(), NS_ERROR_IN_PROGRESS);

    mUploadStream = stream;

    
 
    return NS_OK;
}

NS_IMETHODIMP
nsFtpChannel::GetUploadStream(nsIInputStream **stream)
{
    NS_ENSURE_ARG_POINTER(stream);
    *stream = mUploadStream;
    NS_IF_ADDREF(*stream);
    return NS_OK;
}



NS_IMETHODIMP
nsFtpChannel::ResumeAt(PRUint64 aStartPos, const nsACString& aEntityID)
{
    NS_ENSURE_TRUE(!IsPending(), NS_ERROR_IN_PROGRESS);
    mEntityID = aEntityID;
    mStartPos = aStartPos;
    mResumeRequested = (mStartPos || !mEntityID.IsEmpty());
    return NS_OK;
}

NS_IMETHODIMP
nsFtpChannel::GetEntityID(nsACString& entityID)
{
    if (mEntityID.IsEmpty())
      return NS_ERROR_NOT_RESUMABLE;

    entityID = mEntityID;
    return NS_OK;
}


NS_IMETHODIMP
nsFtpChannel::GetProxyInfo(nsIProxyInfo** aProxyInfo)
{
    *aProxyInfo = ProxyInfo();
    NS_IF_ADDREF(*aProxyInfo);
    return NS_OK;
}



nsresult
nsFtpChannel::OpenContentStream(bool async, nsIInputStream **result,
                                nsIChannel** channel)
{
    if (!async)
        return NS_ERROR_NOT_IMPLEMENTED;

    nsFtpState *state = new nsFtpState();
    if (!state)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(state);

    nsresult rv = state->Init(this);
    if (NS_FAILED(rv)) {
        NS_RELEASE(state);
        return rv;
    }

    *result = state;
    return NS_OK;
}

bool
nsFtpChannel::GetStatusArg(nsresult status, nsString &statusArg)
{
    nsCAutoString host;
    URI()->GetHost(host);
    CopyUTF8toUTF16(host, statusArg);
    return PR_TRUE;
}

void
nsFtpChannel::OnCallbacksChanged()
{
    mFTPEventSink = nsnull;
}



namespace {

class FTPEventSinkProxy : public nsIFTPEventSink
{
public:
    FTPEventSinkProxy(nsIFTPEventSink* aTarget)
        : mTarget(aTarget)
        , mTargetThread(do_GetCurrentThread())
    { }
        
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFTPEVENTSINK

    class OnFTPControlLogRunnable : public nsRunnable
    {
    public:
        OnFTPControlLogRunnable(nsIFTPEventSink* aTarget,
                                bool aServer,
                                const char* aMessage)
            : mTarget(aTarget)
            , mServer(aServer)
            , mMessage(aMessage)
        { }

        NS_DECL_NSIRUNNABLE

    private:
        nsCOMPtr<nsIFTPEventSink> mTarget;
        bool mServer;
        nsCString mMessage;
    };

private:
    nsCOMPtr<nsIFTPEventSink> mTarget;
    nsCOMPtr<nsIThread> mTargetThread;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(FTPEventSinkProxy, nsIFTPEventSink)

NS_IMETHODIMP
FTPEventSinkProxy::OnFTPControlLog(bool aServer, const char* aMsg)
{
    nsRefPtr<OnFTPControlLogRunnable> r =
        new OnFTPControlLogRunnable(mTarget, aServer, aMsg);
    return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
FTPEventSinkProxy::OnFTPControlLogRunnable::Run()
{
    mTarget->OnFTPControlLog(mServer, mMessage.get());
    return NS_OK;
}

} 

void
nsFtpChannel::GetFTPEventSink(nsCOMPtr<nsIFTPEventSink> &aResult)
{
    if (!mFTPEventSink) {
        nsCOMPtr<nsIFTPEventSink> ftpSink;
        GetCallback(ftpSink);
        if (ftpSink) {
            mFTPEventSink = new FTPEventSinkProxy(ftpSink);
        }
    }
    aResult = mFTPEventSink;
}
