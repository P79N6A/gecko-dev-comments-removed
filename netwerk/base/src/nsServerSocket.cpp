




































#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsSocketTransport2.h"
#include "nsServerSocket.h"
#include "nsProxyRelease.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsNetError.h"
#include "nsNetCID.h"
#include "prnetdb.h"
#include "prio.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);



typedef void (nsServerSocket:: *nsServerSocketFunc)(void);

static nsresult
PostEvent(nsServerSocket *s, nsServerSocketFunc func)
{
  nsCOMPtr<nsIRunnable> ev = new nsRunnableMethod<nsServerSocket>(s, func);
  if (!ev)
    return NS_ERROR_OUT_OF_MEMORY;

  return gSocketTransportService->Dispatch(ev, NS_DISPATCH_NORMAL);
}





nsServerSocket::nsServerSocket()
  : mLock(nsnull)
  , mFD(nsnull)
  , mAttached(PR_FALSE)
{
  
  
  if (!gSocketTransportService)
  {
    nsCOMPtr<nsISocketTransportService> sts =
        do_GetService(kSocketTransportServiceCID);
    NS_ASSERTION(sts, "no socket transport service");
  }
  
  NS_ADDREF(gSocketTransportService);
}

nsServerSocket::~nsServerSocket()
{
  Close(); 

  if (mLock)
    PR_DestroyLock(mLock);

  
  nsSocketTransportService *serv = gSocketTransportService;
  NS_RELEASE(serv);
}

void
nsServerSocket::OnMsgClose()
{
  LOG(("nsServerSocket::OnMsgClose [this=%p]\n", this));

  if (NS_FAILED(mCondition))
    return;

  
  mCondition = NS_BINDING_ABORTED;

  
  
  if (!mAttached)
    OnSocketDetached(mFD);
}

void
nsServerSocket::OnMsgAttach()
{
  LOG(("nsServerSocket::OnMsgAttach [this=%p]\n", this));

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

  
  
  
  
  
  
  
  
  
  
  
  
  if (!gSocketTransportService->CanAttachSocket())
  {
    nsCOMPtr<nsIRunnable> event =
        NS_NEW_RUNNABLE_METHOD(nsServerSocket, this, OnMsgAttach);
    if (!event)
      return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = gSocketTransportService->NotifyWhenCanAttachSocket(event);
    if (NS_FAILED(rv))
      return rv;
  }

  
  
  
  rv = gSocketTransportService->AttachSocket(mFD, this);
  if (NS_FAILED(rv))
    return rv;

  mAttached = PR_TRUE;

  
  
  
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
    mFD = nsnull;
  }

  if (mListener)
  {
    mListener->OnStopListening(this, mCondition);

    
    nsIServerSocketListener *listener = nsnull;
    {
      nsAutoLock lock(mLock);
      mListener.swap(listener);
    }
    
    
    if (listener)
      NS_ProxyRelease(mListenerTarget, listener);
  }
}






NS_IMPL_THREADSAFE_ISUPPORTS1(nsServerSocket, nsIServerSocket)






NS_IMETHODIMP
nsServerSocket::Init(PRInt32 aPort, PRBool aLoopbackOnly, PRInt32 aBackLog)
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
  NS_ENSURE_TRUE(mFD == nsnull, NS_ERROR_ALREADY_INITIALIZED);

  if (!mLock)
  {
    mLock = PR_NewLock();
    if (!mLock)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  

  mFD = PR_OpenTCPSocket(aAddr->raw.family);
  if (!mFD)
  {
    NS_WARNING("unable to create server socket");
    return NS_ERROR_FAILURE;
  }

  PRSocketOptionData opt;

  opt.option = PR_SockOpt_Reuseaddr;
  opt.value.reuse_addr = PR_TRUE;
  PR_SetSocketOption(mFD, &opt);

  opt.option = PR_SockOpt_Nonblocking;
  opt.value.non_blocking = PR_TRUE;
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
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  {
    nsAutoLock lock(mLock);
    
    
    if (!mListener)
    {
      if (mFD)
      {
        PR_Close(mFD);
        mFD = nsnull;
      }
      return NS_OK;
    }
  }
  return PostEvent(this, &nsServerSocket::OnMsgClose);
}

NS_IMETHODIMP
nsServerSocket::AsyncListen(nsIServerSocketListener *aListener)
{
  
  NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mListener == nsnull, NS_ERROR_IN_PROGRESS);
  {
    nsAutoLock lock(mLock);
    nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                       NS_GET_IID(nsIServerSocketListener),
                                       aListener,
                                       NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(mListener));
    if (NS_FAILED(rv))
      return rv;
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
