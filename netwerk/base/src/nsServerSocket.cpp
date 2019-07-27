




#include "nsSocketTransport2.h"
#include "nsServerSocket.h"
#include "nsProxyRelease.h"
#include "nsAutoPtr.h"
#include "nsError.h"
#include "nsNetCID.h"
#include "prnetdb.h"
#include "prio.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/Endian.h"
#include "mozilla/net/DNS.h"
#include "nsServiceManagerUtils.h"
#include "nsIFile.h"

using namespace mozilla;
using namespace mozilla::net;

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
  , mKeepWhenOffline(false)
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
nsServerSocket::OnSocketReady(PRFileDesc *fd, int16_t outFlags)
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
  PRNetAddr prClientAddr;
  NetAddr clientAddr;

  
  
  
  
  
  memset(&prClientAddr, 0, sizeof(prClientAddr));

  clientFD = PR_Accept(mFD, &prClientAddr, PR_INTERVAL_NO_WAIT);
  PRNetAddrToNetAddr(&prClientAddr, &clientAddr);
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

void
nsServerSocket::IsLocal(bool *aIsLocal)
{
#if defined(XP_UNIX)
  
  if (mAddr.raw.family == PR_AF_LOCAL)
  {
    *aIsLocal = true;
    return;
  }
#endif

  
  *aIsLocal = PR_IsNetAddrType(&mAddr, PR_IpAddrLoopback);
}

void
nsServerSocket::KeepWhenOffline(bool *aKeepWhenOffline)
{
  *aKeepWhenOffline = mKeepWhenOffline;
}





NS_IMPL_ISUPPORTS(nsServerSocket, nsIServerSocket)






NS_IMETHODIMP
nsServerSocket::Init(int32_t aPort, bool aLoopbackOnly, int32_t aBackLog)
{
  return InitSpecialConnection(aPort, aLoopbackOnly ? LoopbackOnly : 0, aBackLog);
}

NS_IMETHODIMP
nsServerSocket::InitWithFilename(nsIFile *aPath, uint32_t aPermissions, int32_t aBacklog)
{
#if defined(XP_UNIX)
  nsresult rv;

  nsAutoCString path;
  rv = aPath->GetNativePath(path);
  if (NS_FAILED(rv))
    return rv;

  
  PRNetAddr addr;
  if (path.Length() > sizeof(addr.local.path) - 1)
    return NS_ERROR_FILE_NAME_TOO_LONG;
  addr.local.family = PR_AF_LOCAL;
  memcpy(addr.local.path, path.get(), path.Length());
  addr.local.path[path.Length()] = '\0';

  rv = InitWithAddress(&addr, aBacklog);
  if (NS_FAILED(rv))
    return rv;

  return aPath->SetPermissions(aPermissions);
#else
  return NS_ERROR_SOCKET_ADDRESS_NOT_SUPPORTED;
#endif
}

NS_IMETHODIMP
nsServerSocket::InitSpecialConnection(int32_t aPort, nsServerSocketFlag aFlags,
                                      int32_t aBackLog)
{
  PRNetAddrValue val;
  PRNetAddr addr;

  if (aPort < 0)
    aPort = 0;
  if (aFlags & nsIServerSocket::LoopbackOnly)
    val = PR_IpAddrLoopback;
  else
    val = PR_IpAddrAny;
  PR_SetNetAddr(val, PR_AF_INET, aPort, &addr);

  mKeepWhenOffline = ((aFlags & nsIServerSocket::KeepWhenOffline) != 0);
  return InitWithAddress(&addr, aBackLog);
}

NS_IMETHODIMP
nsServerSocket::InitWithAddress(const PRNetAddr *aAddr, int32_t aBackLog)
{
  NS_ENSURE_TRUE(mFD == nullptr, NS_ERROR_ALREADY_INITIALIZED);

  
  
  

  mFD = PR_OpenTCPSocket(aAddr->raw.family);
  if (!mFD)
  {
    NS_WARNING("unable to create server socket");
    return ErrorAccordingToNSPR(PR_GetError());
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
  nsresult rv = ErrorAccordingToNSPR(PR_GetError());
  Close();
  return rv;
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
  ~ServerSocketListenerProxy() {}

public:
  explicit ServerSocketListenerProxy(nsIServerSocketListener* aListener)
    : mListener(new nsMainThreadPtrHolder<nsIServerSocketListener>(aListener))
    , mTargetThread(do_GetCurrentThread())
  { }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISERVERSOCKETLISTENER

  class OnSocketAcceptedRunnable : public nsRunnable
  {
  public:
    OnSocketAcceptedRunnable(const nsMainThreadPtrHandle<nsIServerSocketListener>& aListener,
                             nsIServerSocket* aServ,
                             nsISocketTransport* aTransport)
      : mListener(aListener)
      , mServ(aServ)
      , mTransport(aTransport)
    { }
    
    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIServerSocketListener> mListener;
    nsCOMPtr<nsIServerSocket> mServ;
    nsCOMPtr<nsISocketTransport> mTransport;
  };

  class OnStopListeningRunnable : public nsRunnable
  {
  public:
    OnStopListeningRunnable(const nsMainThreadPtrHandle<nsIServerSocketListener>& aListener,
                            nsIServerSocket* aServ,
                            nsresult aStatus)
      : mListener(aListener)
      , mServ(aServ)
      , mStatus(aStatus)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIServerSocketListener> mListener;
    nsCOMPtr<nsIServerSocket> mServ;
    nsresult mStatus;
  };

private:
  nsMainThreadPtrHandle<nsIServerSocketListener> mListener;
  nsCOMPtr<nsIEventTarget> mTargetThread;
};

NS_IMPL_ISUPPORTS(ServerSocketListenerProxy,
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
nsServerSocket::GetPort(int32_t *aResult)
{
  
  uint16_t port;
  if (mAddr.raw.family == PR_AF_INET)
    port = mAddr.inet.port;
  else if (mAddr.raw.family == PR_AF_INET6)
    port = mAddr.ipv6.port;
  else
    return NS_ERROR_FAILURE;

  *aResult = static_cast<int32_t>(NetworkEndian::readUint16(&port));
  return NS_OK;
}

NS_IMETHODIMP
nsServerSocket::GetAddress(PRNetAddr *aResult)
{
  
  memcpy(aResult, &mAddr, sizeof(mAddr));
  return NS_OK;
}
