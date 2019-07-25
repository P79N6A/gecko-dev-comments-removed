





#include "nsHttp.h"
#include "NullHttpTransaction.h"
#include "nsProxyRelease.h"
#include "nsHttpHandler.h"

namespace mozilla {
namespace net {

NS_IMPL_THREADSAFE_ISUPPORTS0(NullHttpTransaction)

NullHttpTransaction::NullHttpTransaction(nsHttpConnectionInfo *ci,
                                         nsIInterfaceRequestor *callbacks,
                                         nsIEventTarget *target,
                                         PRUint8 caps)
  : mStatus(NS_OK)
  , mCaps(caps | NS_HTTP_ALLOW_KEEPALIVE)
  , mCallbacks(callbacks)
  , mEventTarget(target)
  , mConnectionInfo(ci)
  , mRequestHead(nullptr)
  , mIsDone(false)
{
}

NullHttpTransaction::~NullHttpTransaction()
{
  if (mCallbacks) {
    nsIInterfaceRequestor *cbs = nullptr;
    mCallbacks.swap(cbs);
    NS_ProxyRelease(mEventTarget, cbs);
  }
  delete mRequestHead;
}

void
NullHttpTransaction::SetConnection(nsAHttpConnection *conn)
{
  mConnection = conn;
}

nsAHttpConnection *
NullHttpTransaction::Connection()
{
  return mConnection.get();
}

void
NullHttpTransaction::GetSecurityCallbacks(nsIInterfaceRequestor **outCB,
                                           nsIEventTarget **outTarget)
{
  nsCOMPtr<nsIInterfaceRequestor> copyCB(mCallbacks);
  *outCB = copyCB;
  copyCB.forget();

  if (outTarget) {
    nsCOMPtr<nsIEventTarget> copyET(mEventTarget);
    *outTarget = copyET;
    copyET.forget();
  }
}

void
NullHttpTransaction::OnTransportStatus(nsITransport* transport,
                                       nsresult status, PRUint64 progress)
{
}

bool
NullHttpTransaction::IsDone()
{
  return mIsDone;
}

nsresult
NullHttpTransaction::Status()
{
  return mStatus;
}

PRUint8
NullHttpTransaction::Caps()
{
  return mCaps;
}

PRUint64
NullHttpTransaction::Available()
{
  return 0;
}

nsresult
NullHttpTransaction::ReadSegments(nsAHttpSegmentReader *reader,
                                  PRUint32 count, PRUint32 *countRead)
{
  *countRead = 0;
  mIsDone = true;
  return NS_BASE_STREAM_CLOSED;
}

nsresult
NullHttpTransaction::WriteSegments(nsAHttpSegmentWriter *writer,
                                   PRUint32 count, PRUint32 *countWritten)
{
  *countWritten = 0;
  return NS_BASE_STREAM_CLOSED;
}

PRUint32
NullHttpTransaction::Http1xTransactionCount()
{
  return 0;
}

nsHttpRequestHead *
NullHttpTransaction::RequestHead()
{
  
  

  if (!mRequestHead) {
    mRequestHead = new nsHttpRequestHead();

    nsCAutoString hostHeader;
    nsCString host(mConnectionInfo->GetHost());
    nsresult rv = nsHttpHandler::GenerateHostPort(host,
                                                  mConnectionInfo->Port(),
                                                  hostHeader);
    if (NS_SUCCEEDED(rv))
       mRequestHead->SetHeader(nsHttp::Host, hostHeader);

    
    
    
    
  }
  
  return mRequestHead;
}

nsresult
NullHttpTransaction::TakeSubTransactions(
  nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
NullHttpTransaction::SetSSLConnectFailed()
{
}

void
NullHttpTransaction::Close(nsresult reason)
{
  mStatus = reason;
  mConnection = nullptr;
  mIsDone = true;
}

nsresult
NullHttpTransaction::AddTransaction(nsAHttpTransaction *trans)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRUint32
NullHttpTransaction::PipelineDepth()
{
  return 0;
}

nsresult
NullHttpTransaction::SetPipelinePosition(PRInt32 position)
{
    return NS_OK;
}
 
PRInt32
NullHttpTransaction::PipelinePosition()
{
  return 1;
}

} 
} 

