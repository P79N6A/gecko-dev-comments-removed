




#include "nsSocketTransport2.h"
#include "nsUDPSocket.h"
#include "nsProxyRelease.h"
#include "nsAutoPtr.h"
#include "nsError.h"
#include "nsNetCID.h"
#include "prnetdb.h"
#include "prio.h"
#include "mozilla/Attributes.h"
#include "nsNetAddr.h"
#include "nsNetSegmentUtils.h"
#include "NetworkActivityMonitor.h"
#include "nsStreamUtils.h"
#include "nsIPipe.h"
#include "prerror.h"
#include "nsThreadUtils.h"
#include "nsIDNSRecord.h"
#include "nsIDNSService.h"
#include "nsICancelable.h"

using namespace mozilla::net;
using namespace mozilla;

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);



typedef void (nsUDPSocket:: *nsUDPSocketFunc)(void);

static nsresult
PostEvent(nsUDPSocket *s, nsUDPSocketFunc func)
{
  nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(s, func);

  if (!gSocketTransportService)
    return NS_ERROR_FAILURE;

  return gSocketTransportService->Dispatch(ev, NS_DISPATCH_NORMAL);
}

static nsresult
ResolveHost(const nsACString &host, nsIDNSListener *listener)
{
  nsresult rv;

  nsCOMPtr<nsIDNSService> dns =
      do_GetService("@mozilla.org/network/dns-service;1", &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsICancelable> tmpOutstanding;
  return dns->AsyncResolve(host, 0, listener, nullptr,
                           getter_AddRefs(tmpOutstanding));

}




NS_IMPL_ISUPPORTS1(nsUDPOutputStream, nsIOutputStream)

nsUDPOutputStream::nsUDPOutputStream(nsUDPSocket* aSocket,
                                     PRFileDesc* aFD,
                                     PRNetAddr& aPrClientAddr)
  : mSocket(aSocket)
  , mFD(aFD)
  , mPrClientAddr(aPrClientAddr)
  , mIsClosed(false)
{
}

nsUDPOutputStream::~nsUDPOutputStream()
{
}


NS_IMETHODIMP nsUDPOutputStream::Close()
{
  if (mIsClosed)
    return NS_BASE_STREAM_CLOSED;

  mIsClosed = true;
  return NS_OK;
}


NS_IMETHODIMP nsUDPOutputStream::Flush()
{
  return NS_OK;
}


NS_IMETHODIMP nsUDPOutputStream::Write(const char * aBuf, uint32_t aCount, uint32_t *_retval)
{
  if (mIsClosed)
    return NS_BASE_STREAM_CLOSED;

  *_retval = 0;
  int32_t count = PR_SendTo(mFD, aBuf, aCount, 0, &mPrClientAddr, PR_INTERVAL_NO_WAIT);
  if (count < 0) {
    PRErrorCode code = PR_GetError();
    return ErrorAccordingToNSPR(code);
  }

  *_retval = count;

  mSocket->AddOutputBytes(count);

  return NS_OK;
}


NS_IMETHODIMP nsUDPOutputStream::WriteFrom(nsIInputStream *aFromStream, uint32_t aCount, uint32_t *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsUDPOutputStream::WriteSegments(nsReadSegmentFun aReader, void *aClosure, uint32_t aCount, uint32_t *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsUDPOutputStream::IsNonBlocking(bool *_retval)
{
  *_retval = true;
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsUDPMessage, nsIUDPMessage)

nsUDPMessage::nsUDPMessage(PRNetAddr* aAddr,
             nsIOutputStream* aOutputStream,
             const nsACString& aData)
  : mOutputStream(aOutputStream)
  , mData(aData)
{
  memcpy(&mAddr, aAddr, sizeof(NetAddr));
}

nsUDPMessage::~nsUDPMessage()
{
}


NS_IMETHODIMP nsUDPMessage::GetFromAddr(nsINetAddr * *aFromAddr)
{
  NS_ENSURE_ARG_POINTER(aFromAddr);

  NetAddr clientAddr;
  PRNetAddrToNetAddr(&mAddr, &clientAddr);

  nsCOMPtr<nsINetAddr> result = new nsNetAddr(&clientAddr);
  result.forget(aFromAddr);

  return NS_OK;
}


NS_IMETHODIMP nsUDPMessage::GetData(nsACString & aData)
{
  aData = mData;
  return NS_OK;
}


NS_IMETHODIMP nsUDPMessage::GetOutputStream(nsIOutputStream * *aOutputStream)
{
  NS_ENSURE_ARG_POINTER(aOutputStream);
  NS_IF_ADDREF(*aOutputStream = mOutputStream);
  return NS_OK;
}





nsUDPSocket::nsUDPSocket()
  : mLock("nsUDPSocket.mLock")
  , mFD(nullptr)
  , mAttached(false)
  , mByteReadCount(0)
  , mByteWriteCount(0)
{
  mAddr.raw.family = PR_AF_UNSPEC;
  
  
  if (!gSocketTransportService)
  {
    
    nsCOMPtr<nsISocketTransportService> sts =
        do_GetService(kSocketTransportServiceCID);
  }

  mSts = gSocketTransportService;
  MOZ_COUNT_CTOR(nsUDPSocket);
}

nsUDPSocket::~nsUDPSocket()
{
  Close(); 

  MOZ_COUNT_DTOR(nsUDPSocket);
}

void
nsUDPSocket::AddOutputBytes(uint64_t aBytes)
{
  mByteWriteCount += aBytes;
}

void
nsUDPSocket::OnMsgClose()
{
  SOCKET_LOG(("nsUDPSocket::OnMsgClose [this=%p]\n", this));

  if (NS_FAILED(mCondition))
    return;

  
  mCondition = NS_BINDING_ABORTED;

  
  
  
  if (!mAttached)
    OnSocketDetached(mFD);
}

void
nsUDPSocket::OnMsgAttach()
{
  SOCKET_LOG(("nsUDPSocket::OnMsgAttach [this=%p]\n", this));

  if (NS_FAILED(mCondition))
    return;

  mCondition = TryAttach();

  
  if (NS_FAILED(mCondition))
  {
    NS_ASSERTION(!mAttached, "should not be attached already");
    OnSocketDetached(mFD);
  }
}

nsresult
nsUDPSocket::TryAttach()
{
  nsresult rv;

  if (!gSocketTransportService)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  
  
  
  
  if (!gSocketTransportService->CanAttachSocket())
  {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsUDPSocket::OnMsgAttach);

    nsresult rv = gSocketTransportService->NotifyWhenCanAttachSocket(event);
    if (NS_FAILED(rv))
      return rv;
  }

  
  
  
  rv = gSocketTransportService->AttachSocket(mFD, this);
  if (NS_FAILED(rv))
    return rv;

  mAttached = true;

  
  
  
  mPollFlags = (PR_POLL_READ | PR_POLL_EXCEPT);
  return NS_OK;
}





void
nsUDPSocket::OnSocketReady(PRFileDesc *fd, int16_t outFlags)
{
  NS_ASSERTION(NS_SUCCEEDED(mCondition), "oops");
  NS_ASSERTION(mFD == fd, "wrong file descriptor");
  NS_ASSERTION(outFlags != -1, "unexpected timeout condition reached");

  if (outFlags & (PR_POLL_ERR | PR_POLL_HUP | PR_POLL_NVAL))
  {
    NS_WARNING("error polling on listening socket");
    mCondition = NS_ERROR_UNEXPECTED;
    return;
  }

  PRNetAddr prClientAddr;
  uint32_t count;
  char buff[1500];
  count = PR_RecvFrom(mFD, buff, sizeof(buff), 0, &prClientAddr, PR_INTERVAL_NO_WAIT);

  if (count < 1) {
    NS_WARNING("error of recvfrom on UDP socket");
    mCondition = NS_ERROR_UNEXPECTED;
    return;
  }
  mByteReadCount += count;

  nsCString data;
  if (!data.Assign(buff, count, mozilla::fallible_t())) {
    mCondition = NS_ERROR_OUT_OF_MEMORY;
    return;
  }

  nsCOMPtr<nsIAsyncInputStream> pipeIn;
  nsCOMPtr<nsIAsyncOutputStream> pipeOut;

  uint32_t segsize = 1400;
  uint32_t segcount = 0;
  net_ResolveSegmentParams(segsize, segcount);
  nsresult rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                true, true, segsize, segcount);

  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsUDPOutputStream> os = new nsUDPOutputStream(this, mFD, prClientAddr);
  rv = NS_AsyncCopy(pipeIn, os, mSts,
                    NS_ASYNCCOPY_VIA_READSEGMENTS, 1400);

  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIUDPMessage> message = new nsUDPMessage(&prClientAddr, pipeOut, data);
  mListener->OnPacketReceived(this, message);
}

void
nsUDPSocket::OnSocketDetached(PRFileDesc *fd)
{
  
  if (NS_SUCCEEDED(mCondition))
    mCondition = NS_ERROR_ABORT;

  if (mFD)
  {
    NS_ASSERTION(mFD == fd, "wrong file descriptor");
    PR_Close(mFD);
    mFD = nullptr;
  }

  if (mListener)
  {
    
    nsCOMPtr<nsIUDPSocketListener> listener;
    {
      MutexAutoLock lock(mLock);
      mListener.swap(listener);
    }

    if (listener) {
      listener->OnStopListening(this, mCondition);
      NS_ProxyRelease(mListenerTarget, listener);
    }
  }
}

void
nsUDPSocket::IsLocal(bool *aIsLocal)
{
  
  *aIsLocal = mAddr.raw.family == nsINetAddr::FAMILY_LOCAL;
}





NS_IMPL_ISUPPORTS1(nsUDPSocket, nsIUDPSocket)






NS_IMETHODIMP
nsUDPSocket::Init(int32_t aPort, bool aLoopbackOnly)
{
  NetAddr addr;

  if (aPort < 0)
    aPort = 0;

  addr.raw.family = AF_INET;
  addr.inet.port = htons(aPort);

  if (aLoopbackOnly)
    addr.inet.ip = htonl(INADDR_LOOPBACK);
  else
    addr.inet.ip = htonl(INADDR_ANY);

  return InitWithAddress(&addr);
}

NS_IMETHODIMP
nsUDPSocket::InitWithAddress(const NetAddr *aAddr)
{
  NS_ENSURE_TRUE(mFD == nullptr, NS_ERROR_ALREADY_INITIALIZED);

  
  
  

  mFD = PR_OpenUDPSocket(aAddr->raw.family);
  if (!mFD)
  {
    NS_WARNING("unable to create UDP socket");
    return NS_ERROR_FAILURE;
  }

  PRSocketOptionData opt;

  opt.option = PR_SockOpt_Reuseaddr;
  opt.value.reuse_addr = true;
  PR_SetSocketOption(mFD, &opt);

  opt.option = PR_SockOpt_Nonblocking;
  opt.value.non_blocking = true;
  PR_SetSocketOption(mFD, &opt);

  PRNetAddr addr;
  PR_InitializeNetAddr(PR_IpAddrAny, 0, &addr);
  NetAddrToPRNetAddr(aAddr, &addr);

  if (PR_Bind(mFD, &addr) != PR_SUCCESS)
  {
    NS_WARNING("failed to bind socket");
    goto fail;
  }

  
  
  if (PR_GetSockName(mFD, &addr) != PR_SUCCESS)
  {
    NS_WARNING("cannot get socket name");
    goto fail;
  }

  PRNetAddrToNetAddr(&addr, &mAddr);

  
  NetworkActivityMonitor::AttachIOLayer(mFD);

  
  
  return NS_OK;

fail:
  Close();
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsUDPSocket::Close()
{
  {
    MutexAutoLock lock(mLock);
    
    
    if (!mListener)
    {
      if (mFD)
      {
        PR_Close(mFD);
        mFD = nullptr;
      }
      return NS_OK;
    }
  }
  return PostEvent(this, &nsUDPSocket::OnMsgClose);
}

NS_IMETHODIMP
nsUDPSocket::GetPort(int32_t *aResult)
{
  
  uint16_t port;
  if (mAddr.raw.family == PR_AF_INET)
    port = mAddr.inet.port;
  else if (mAddr.raw.family == PR_AF_INET6)
    port = mAddr.inet6.port;
  else
    return NS_ERROR_NOT_INITIALIZED;

  *aResult = (int32_t) PR_ntohs(port);
  return NS_OK;
}

NS_IMETHODIMP
nsUDPSocket::GetAddress(NetAddr *aResult)
{
  
  memcpy(aResult, &mAddr, sizeof(mAddr));
  return NS_OK;
}

namespace {

class SocketListenerProxy MOZ_FINAL : public nsIUDPSocketListener
{
public:
  SocketListenerProxy(nsIUDPSocketListener* aListener)
    : mListener(new nsMainThreadPtrHolder<nsIUDPSocketListener>(aListener))
    , mTargetThread(do_GetCurrentThread())
  { }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUDPSOCKETLISTENER

  class OnPacketReceivedRunnable : public nsRunnable
  {
  public:
    OnPacketReceivedRunnable(const nsMainThreadPtrHandle<nsIUDPSocketListener>& aListener,
                             nsIUDPSocket* aSocket,
                             nsIUDPMessage* aMessage)
      : mListener(aListener)
      , mSocket(aSocket)
      , mMessage(aMessage)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUDPSocketListener> mListener;
    nsCOMPtr<nsIUDPSocket> mSocket;
    nsCOMPtr<nsIUDPMessage> mMessage;
  };

  class OnStopListeningRunnable : public nsRunnable
  {
  public:
    OnStopListeningRunnable(const nsMainThreadPtrHandle<nsIUDPSocketListener>& aListener,
                            nsIUDPSocket* aSocket,
                            nsresult aStatus)
      : mListener(aListener)
      , mSocket(aSocket)
      , mStatus(aStatus)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIUDPSocketListener> mListener;
    nsCOMPtr<nsIUDPSocket> mSocket;
    nsresult mStatus;
  };

private:
  nsMainThreadPtrHandle<nsIUDPSocketListener> mListener;
  nsCOMPtr<nsIEventTarget> mTargetThread;
};

NS_IMPL_ISUPPORTS1(SocketListenerProxy,
                   nsIUDPSocketListener)

NS_IMETHODIMP
SocketListenerProxy::OnPacketReceived(nsIUDPSocket* aSocket,
                                      nsIUDPMessage* aMessage)
{
  nsRefPtr<OnPacketReceivedRunnable> r =
    new OnPacketReceivedRunnable(mListener, aSocket, aMessage);
  return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
SocketListenerProxy::OnStopListening(nsIUDPSocket* aSocket,
                                     nsresult aStatus)
{
  nsRefPtr<OnStopListeningRunnable> r =
    new OnStopListeningRunnable(mListener, aSocket, aStatus);
  return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
SocketListenerProxy::OnPacketReceivedRunnable::Run()
{
  mListener->OnPacketReceived(mSocket, mMessage);
  return NS_OK;
}

NS_IMETHODIMP
SocketListenerProxy::OnStopListeningRunnable::Run()
{
  mListener->OnStopListening(mSocket, mStatus);
  return NS_OK;
}

class PendingSend : public nsIDNSListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDNSLISTENER

  PendingSend(nsUDPSocket *aSocket, uint16_t aPort,
              FallibleTArray<uint8_t> &aData)
      : mSocket(aSocket)
      , mPort(aPort)
  {
    mData.SwapElements(aData);
  }

  virtual ~PendingSend() {}

private:
  nsRefPtr<nsUDPSocket> mSocket;
  uint16_t mPort;
  FallibleTArray<uint8_t> mData;
};

NS_IMPL_ISUPPORTS1(PendingSend, nsIDNSListener)

NS_IMETHODIMP
PendingSend::OnLookupComplete(nsICancelable *request,
                              nsIDNSRecord  *rec,
                              nsresult       status)
{
  if (NS_FAILED(status)) {
    NS_WARNING("Failed to send UDP packet due to DNS lookup failure");
    return NS_OK;
  }

  NetAddr addr;
  if (NS_SUCCEEDED(rec->GetNextAddr(mPort, &addr))) {
    uint32_t count;
    nsresult rv = mSocket->SendWithAddress(&addr, mData.Elements(),
                                           mData.Length(), &count);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

class SendRequestRunnable: public nsRunnable {
public:
  SendRequestRunnable(nsUDPSocket *aSocket,
                      const NetAddr &aAddr,
                      FallibleTArray<uint8_t> &aData)
    : mSocket(aSocket)
    , mAddr(aAddr)
    , mData(aData)
  { }

  NS_DECL_NSIRUNNABLE

private:
  nsRefPtr<nsUDPSocket> mSocket;
  const NetAddr mAddr;
  FallibleTArray<uint8_t> mData;
};

NS_IMETHODIMP
SendRequestRunnable::Run()
{
  uint32_t count;
  mSocket->SendWithAddress(&mAddr, mData.Elements(),
                           mData.Length(), &count);
  return NS_OK;
}

} 

NS_IMETHODIMP
nsUDPSocket::AsyncListen(nsIUDPSocketListener *aListener)
{
  
  NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mListener == nullptr, NS_ERROR_IN_PROGRESS);
  {
    MutexAutoLock lock(mLock);
    mListener = new SocketListenerProxy(aListener);
    mListenerTarget = NS_GetCurrentThread();
  }
  return PostEvent(this, &nsUDPSocket::OnMsgAttach);
}

NS_IMETHODIMP
nsUDPSocket::Send(const nsACString &aHost, uint16_t aPort,
                  const uint8_t *aData, uint32_t aDataLength,
                  uint32_t *_retval)
{
  NS_ENSURE_ARG(aData);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = 0;

  FallibleTArray<uint8_t> fallibleArray;
  if (!fallibleArray.InsertElementsAt(0, aData, aDataLength)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDNSListener> listener = new PendingSend(this, aPort, fallibleArray);

  nsresult rv = ResolveHost(aHost, listener);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = aDataLength;
  return NS_OK;
}

NS_IMETHODIMP
nsUDPSocket::SendWithAddr(nsINetAddr *aAddr, const uint8_t *aData,
                          uint32_t aDataLength, uint32_t *_retval)
{
  NS_ENSURE_ARG(aAddr);
  NS_ENSURE_ARG(aData);
  NS_ENSURE_ARG_POINTER(_retval);

  NetAddr netAddr;
  aAddr->GetNetAddr(&netAddr);
  return SendWithAddress(&netAddr, aData, aDataLength, _retval);
}

NS_IMETHODIMP
nsUDPSocket::SendWithAddress(const NetAddr *aAddr, const uint8_t *aData,
                             uint32_t aDataLength, uint32_t *_retval)
{
  NS_ENSURE_ARG(aAddr);
  NS_ENSURE_ARG(aData);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = 0;

  PRNetAddr prAddr;
  NetAddrToPRNetAddr(aAddr, &prAddr);

  bool onSTSThread = false;
  mSts->IsOnCurrentThread(&onSTSThread);

  if (onSTSThread) {
    MutexAutoLock lock(mLock);
    if (!mFD) {
      
      return NS_ERROR_FAILURE;
    }
    int32_t count = PR_SendTo(mFD, aData, sizeof(uint8_t) *aDataLength,
                              0, &prAddr, PR_INTERVAL_NO_WAIT);
    if (count < 0) {
      PRErrorCode code = PR_GetError();
      return ErrorAccordingToNSPR(code);
    }
    this->AddOutputBytes(count);
    *_retval = count;
  } else {
    FallibleTArray<uint8_t> fallibleArray;
    if (!fallibleArray.InsertElementsAt(0, aData, aDataLength)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv = mSts->Dispatch(new SendRequestRunnable(this, *aAddr, fallibleArray),
                                 NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
    *_retval = aDataLength;
  }
  return NS_OK;
}
