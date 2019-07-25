




#include "nsIServiceManager.h"
#include "nsSocketTransport2.h"
#include "nsServerSocket.h"
#include "nsProxyRelease.h"
#include "nsAutoPtr.h"
#include "nsNetError.h"
#include "nsNetCID.h"
#include "prnetdb.h"
#include "prio.h"
#include "mozilla/Attributes.h"

using namespace mozilla;

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);



typedef void (nsServerSocket:: *nsServerSocketFunc)(void);

static nsresult
PostEvent(nsServerSocket *s, nsServerSocketFunc func)
{
  nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(s, func);
  if (!ev)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!gSocketTransportService)
    return NS_ERROR_FAILURE;

  return gSocketTransportService->Dispatch(ev, NS_DISPATCH_NORMAL);
}





nsServerSocket::nsServerSocket()
  : mLock("nsServerSocket.mLock")
  , mFD(nullptr)
  , mAttached(false)
{
  
  
  if (!gSocketTransportService)
  {
    
    nsCOMPtr<nsISocketTransportService> sts =
        do_GetService(kSocketTransportServiceCID);
  }
  
  NS_IF_ADDREF(gSocketTransportService);
}

nsServerSocket::~nsServerSocket()
{
  Close(); 

  
  nsSocketTransportService *serv = gSocketTransportService;
  NS_IF_RELEASE(serv);
}

void
nsServerSocket::OnMsgClose()
{
  SOCKET_LOG(("nsServerSocket::OnMsgClose [this=%p]\n", this));

  if (NS_FAILED(mCondition))
    return;

  
  mCondition = NS_BINDING_ABORTED;

  
  
  if (!mAttached)
    OnSocketDetached(mFD);
}

void
nsServerSocket::OnMsgAttach()
{
  SOCKET_LOG(("nsServerSocket::OnMsgAttach [this=%p]\n", this));

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
nsServerSocket::TryAttach()
{
  nsresult rv;

  if (!gSocketTransportService)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  
  
  
  
  if (!gSocketTransportService->CanAttachSocket())
  {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsServerSocket::OnMsgAttach);
    if (!event)
      return NS_ERROR_OUT_OF_MEMORY;

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
nsServerSocket::OnSocketReady(PRFileDesc *fd, PRInt16 outFlags)
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

  PRFileDesc *clientFD;
  PRNetAddr clientAddr;

  clientFD = PR_Accept(mFD, &clientAddr, PR_INTERVAL_NO_WAIT);
  if (!clientFD)
  {
    NS_WARNING("PR_Accept failed");
    mCondition = NS_ERROR_UNEXPECTED;
  }
  else
  {
    nsRefPtr<nsSocketTransport> trans = new nsSocketTransport;
    if (!trans)
      mCondition = NS_ERROR_OUT_OF_MEMORY;
    else
    {
      nsresult rv = trans->InitWithConnectedSocket(clientFD, &clientAddr);
      if (NS_FAILED(rv))
        mCondition = rv;
      else
        mListener->OnSocketAccepted(this, trans);
    }
  }
}

void
nsServerSocket::OnSocketDetached(PRFileDesc *fd)
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
    mListener->OnStopListening(this, mCondition);

    
    nsIServerSocketListener *listener = nullptr;
    {
      MutexAutoLock lock(mLock);
      mListener.swap(listener);
    }
    
    
    if (listener)
      NS_ProxyRelease(mListenerTarget, listener);
  }
}






NS_IMPL_THREADSAFE_ISUPPORTS1(nsServerSocket, nsIServerSocket)






NS_IMETHODIMP
nsServerSocket::Init(PRInt32 aPort, bool aLoopbackOnly, PRInt32 aBackLog)
{
  PRNetAddrValue val;
  PRNetAddr addr;

  if (aPort < 0)
    aPort = 0;
  if (aLoopbackOnly)
    val = PR_IpAddrLoopback;
  else
    val = PR_IpAddrAny;
  PR_SetNetAddr(val, PR_AF_INET, aPort, &addr);

  return InitWithAddress(&addr, aBackLog);
}

NS_IMETHODIMP
nsServerSocket::InitWithAddress(const PRNetAddr *aAddr, PRInt32 aBackLog)
{
  NS_ENSURE_TRUE(mFD == nullptr, NS_ERROR_ALREADY_INITIALIZED);

  
  
  

  mFD = PR_OpenTCPSocket(aAddr->raw.family);
  if (!mFD)
  {
    NS_WARNING("unable to create server socket");
    return NS_ERROR_FAILURE;
  }

  PRSocketOptionData opt;

  opt.option = PR_SockOpt_Reuseaddr;
  opt.value.reuse_addr = true;
  PR_SetSocketOption(mFD, &opt);

  opt.option = PR_SockOpt_Nonblocking;
  opt.value.non_blocking = true;
  PR_SetSocketOption(mFD, &opt);

  if (PR_Bind(mFD, aAddr) != PR_SUCCESS)
  {
    NS_WARNING("failed to bind socket");
    goto fail;
  }

  if (aBackLog < 0)
    aBackLog = 5; 

  if (PR_Listen(mFD, aBackLog) != PR_SUCCESS)
  {
    NS_WARNING("cannot listen on socket");
    goto fail;
  }

  
  
  if (PR_GetSockName(mFD, &mAddr) != PR_SUCCESS)
  {
    NS_WARNING("cannot get socket name");
    goto fail;
  }

  
  
  return NS_OK;

fail:
  Close();
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsServerSocket::Close()
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
  return PostEvent(this, &nsServerSocket::OnMsgClose);
}

namespace {

class ServerSocketListenerProxy MOZ_FINAL : public nsIServerSocketListener
{
public:
  ServerSocketListenerProxy(nsIServerSocketListener* aListener)
    : mListener(aListener)
    , mTargetThread(do_GetCurrentThread())
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISERVERSOCKETLISTENER

  class OnSocketAcceptedRunnable : public nsRunnable
  {
  public:
    OnSocketAcceptedRunnable(nsIServerSocketListener* aListener,
                             nsIServerSocket* aServ,
                             nsISocketTransport* aTransport)
      : mListener(aListener)
      , mServ(aServ)
      , mTransport(aTransport)
    { }
    
    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIServerSocketListener> mListener;
    nsCOMPtr<nsIServerSocket> mServ;
    nsCOMPtr<nsISocketTransport> mTransport;
  };

  class OnStopListeningRunnable : public nsRunnable
  {
  public:
    OnStopListeningRunnable(nsIServerSocketListener* aListener,
                            nsIServerSocket* aServ,
                            nsresult aStatus)
      : mListener(aListener)
      , mServ(aServ)
      , mStatus(aStatus)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsCOMPtr<nsIServerSocketListener> mListener;
    nsCOMPtr<nsIServerSocket> mServ;
    nsresult mStatus;
  };

private:
  nsCOMPtr<nsIServerSocketListener> mListener;
  nsCOMPtr<nsIEventTarget> mTargetThread;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(ServerSocketListenerProxy,
                              nsIServerSocketListener)

NS_IMETHODIMP
ServerSocketListenerProxy::OnSocketAccepted(nsIServerSocket* aServ,
                                            nsISocketTransport* aTransport)
{
  nsRefPtr<OnSocketAcceptedRunnable> r =
    new OnSocketAcceptedRunnable(mListener, aServ, aTransport);
  return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
ServerSocketListenerProxy::OnStopListening(nsIServerSocket* aServ,
                                           nsresult aStatus)
{
  nsRefPtr<OnStopListeningRunnable> r =
    new OnStopListeningRunnable(mListener, aServ, aStatus);
  return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
ServerSocketListenerProxy::OnSocketAcceptedRunnable::Run()
{
  mListener->OnSocketAccepted(mServ, mTransport);
  return NS_OK;
}

NS_IMETHODIMP
ServerSocketListenerProxy::OnStopListeningRunnable::Run()
{
  mListener->OnStopListening(mServ, mStatus);
  return NS_OK;
}

} 

NS_IMETHODIMP
nsServerSocket::AsyncListen(nsIServerSocketListener *aListener)
{
  
  NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mListener == nullptr, NS_ERROR_IN_PROGRESS);
  {
    MutexAutoLock lock(mLock);
    mListener = new ServerSocketListenerProxy(aListener);
    mListenerTarget = NS_GetCurrentThread();
  }
  return PostEvent(this, &nsServerSocket::OnMsgAttach);
}

NS_IMETHODIMP
nsServerSocket::GetPort(PRInt32 *aResult)
{
  
  PRUint16 port;
  if (mAddr.raw.family == PR_AF_INET)
    port = mAddr.inet.port;
  else
    port = mAddr.ipv6.port;
  *aResult = (PRInt32) PR_ntohs(port);
  return NS_OK;
}

NS_IMETHODIMP
nsServerSocket::GetAddress(PRNetAddr *aResult)
{
  
  memcpy(aResult, &mAddr, sizeof(mAddr));
  return NS_OK;
}
