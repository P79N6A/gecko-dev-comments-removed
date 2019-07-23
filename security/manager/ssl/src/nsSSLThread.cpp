




































#include "nsThreadUtils.h"
#include "nsSSLThread.h"
#include "nsAutoLock.h"
#include "nsNSSIOLayer.h"

#include "ssl.h"

nsSSLThread::nsSSLThread()
: mBusySocket(nsnull),
  mSocketScheduledToBeDestroyed(nsnull),
  mPendingHTTPRequest(nsnull)
{
  NS_ASSERTION(!ssl_thread_singleton, "nsSSLThread is a singleton, caller attempts to create another instance!");
  
  ssl_thread_singleton = this;
}

nsSSLThread::~nsSSLThread()
{
  ssl_thread_singleton = nsnull;
}

PRFileDesc *nsSSLThread::getRealSSLFD(nsNSSSocketInfo *si)
{
  if (!ssl_thread_singleton || !si || !ssl_thread_singleton->mThreadHandle)
    return nsnull;

  nsAutoLock threadLock(ssl_thread_singleton->mMutex);

  if (si->mThreadData->mReplacedSSLFileDesc)
  {
    return si->mThreadData->mReplacedSSLFileDesc;
  }
  else
  {
    return si->mFd->lower;
  }
}

PRStatus nsSSLThread::requestGetsockname(nsNSSSocketInfo *si, PRNetAddr *addr)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return PR_FAILURE;

  return fd->methods->getsockname(fd, addr);
}

PRStatus nsSSLThread::requestGetpeername(nsNSSSocketInfo *si, PRNetAddr *addr)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return PR_FAILURE;

  return fd->methods->getpeername(fd, addr);
}

PRStatus nsSSLThread::requestGetsocketoption(nsNSSSocketInfo *si, 
                                             PRSocketOptionData *data)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return PR_FAILURE;

  return fd->methods->getsocketoption(fd, data);
}

PRStatus nsSSLThread::requestSetsocketoption(nsNSSSocketInfo *si, 
                                             const PRSocketOptionData *data)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return PR_FAILURE;

  return fd->methods->setsocketoption(fd, data);
}

PRStatus nsSSLThread::requestConnectcontinue(nsNSSSocketInfo *si, 
                                             PRInt16 out_flags)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return PR_FAILURE;

  return fd->methods->connectcontinue(fd, out_flags);
}

PRInt32 nsSSLThread::requestRecvMsgPeek(nsNSSSocketInfo *si, void *buf, PRInt32 amount,
                                        PRIntn flags, PRIntervalTime timeout)
{
  if (!ssl_thread_singleton || !si || !ssl_thread_singleton->mThreadHandle)
  {
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, 0);
    return -1;
  }

  PRFileDesc *realSSLFD;

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (si == ssl_thread_singleton->mBusySocket)
    {
      PORT_SetError(PR_WOULD_BLOCK_ERROR);
      return -1;
    }

    switch (si->mThreadData->mSSLState)
    {
      case nsSSLSocketThreadData::ssl_idle:
        break;
    
      case nsSSLSocketThreadData::ssl_reading_done:
        {
          

          
          
          

          if (si->mThreadData->mSSLResultRemainingBytes < 0) {
            if (si->mThreadData->mPRErrorCode != PR_SUCCESS) {
              PR_SetError(si->mThreadData->mPRErrorCode, 0);
            }

            return si->mThreadData->mSSLResultRemainingBytes;
          }

          PRInt32 return_amount = NS_MIN(amount, si->mThreadData->mSSLResultRemainingBytes);

          memcpy(buf, si->mThreadData->mSSLRemainingReadResultData, return_amount);

          return return_amount;
        }

      case nsSSLSocketThreadData::ssl_writing_done:
      case nsSSLSocketThreadData::ssl_pending_write:
      case nsSSLSocketThreadData::ssl_pending_read:

      
      
      
      default:
        {
          PORT_SetError(PR_WOULD_BLOCK_ERROR);
          return -1;
        }
    }

    if (si->mThreadData->mReplacedSSLFileDesc)
    {
      realSSLFD = si->mThreadData->mReplacedSSLFileDesc;
    }
    else
    {
      realSSLFD = si->mFd->lower;
    }
  }

  return realSSLFD->methods->recv(realSSLFD, buf, amount, flags, timeout);
}

nsresult nsSSLThread::requestActivateSSL(nsNSSSocketInfo *si)
{
  PRFileDesc *fd = getRealSSLFD(si);
  if (!fd)
    return NS_ERROR_FAILURE;

  if (SECSuccess != SSL_OptionSet(fd, SSL_SECURITY, PR_TRUE))
    return NS_ERROR_FAILURE;

  if (SECSuccess != SSL_ResetHandshake(fd, PR_FALSE))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

PRInt16 nsSSLThread::requestPoll(nsNSSSocketInfo *si, PRInt16 in_flags, PRInt16 *out_flags)
{
  if (!ssl_thread_singleton || !si || !ssl_thread_singleton->mThreadHandle)
    return 0;

  *out_flags = 0;

  PRBool want_sleep_and_wakeup_on_any_socket_activity = PR_FALSE;
  PRBool handshake_timeout = PR_FALSE;
  
  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (ssl_thread_singleton->mBusySocket)
    {
      
      
      
      switch (si->mThreadData->mSSLState)
      {
        case nsSSLSocketThreadData::ssl_writing_done:
        {
          if (in_flags & PR_POLL_WRITE)
          {
            *out_flags |= PR_POLL_WRITE;
          }

          return in_flags;
        }
        break;
        
        case nsSSLSocketThreadData::ssl_reading_done:
        {
          if (in_flags & PR_POLL_READ)
          {
            *out_flags |= PR_POLL_READ;
          }

          return in_flags;
        }
        break;
        
        case nsSSLSocketThreadData::ssl_pending_write:
        case nsSSLSocketThreadData::ssl_pending_read:
        {
          if (si == ssl_thread_singleton->mBusySocket)
          {
            if (nsSSLIOLayerHelpers::mSharedPollableEvent)
            {
              
              
              
              return PR_POLL_READ;
            }
            else
            {
              
              
              
              
              
              
              
              
              
              
              
              
              
              
              

              want_sleep_and_wakeup_on_any_socket_activity = PR_TRUE;
              break;
            }
          }
          else
          {
            
            
            
            
            
            NS_NOTREACHED("Socket not busy on SSL thread marked as pending");
            return 0;
          }
        }
        break;
        
        case nsSSLSocketThreadData::ssl_idle:
        {
          handshake_timeout = si->HandshakeTimeout();

          if (si != ssl_thread_singleton->mBusySocket)
          {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            in_flags &= ~(PR_POLL_READ | PR_POLL_WRITE);
          }
        }
        break;
        
        default:
          break;
      }
    }
    else
    {
      handshake_timeout = si->HandshakeTimeout();
    }

    if (handshake_timeout)
    {
      NS_ASSERTION(in_flags & PR_POLL_EXCEPT, "nsSSLThread::requestPoll handshake timeout, but caller did not poll for EXCEPT");

      *out_flags |= PR_POLL_EXCEPT;
      return in_flags;
    }
  }

  if (want_sleep_and_wakeup_on_any_socket_activity)
  {
    
    
    
    

    PR_Sleep( PR_MillisecondsToInterval(1) );
    return PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT;
  }

  return si->mFd->lower->methods->poll(si->mFd->lower, in_flags, out_flags);
}

PRStatus nsSSLThread::requestClose(nsNSSSocketInfo *si)
{
  if (!ssl_thread_singleton || !si)
    return PR_FAILURE;

  PRBool close_later = PR_FALSE;
  nsIRequest* requestToCancel = nsnull;

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (ssl_thread_singleton->mBusySocket == si) {
    
      
      
      
      
      
      
      
      if (ssl_thread_singleton->mPendingHTTPRequest)
      {
        requestToCancel = ssl_thread_singleton->mPendingHTTPRequest;
        ssl_thread_singleton->mPendingHTTPRequest = nsnull;
      }
      
      close_later = PR_TRUE;
      ssl_thread_singleton->mSocketScheduledToBeDestroyed = si;

      PR_NotifyAllCondVar(ssl_thread_singleton->mCond);
    }
  }

  if (requestToCancel)
  {
    if (NS_IsMainThread())
    {
      requestToCancel->Cancel(NS_ERROR_ABORT);
    }
    else
    {
      NS_WARNING("Attempt to close SSL socket from a thread that is not the main thread. Can not cancel pending HTTP request from NSS");
    }
  
    NS_RELEASE(requestToCancel);
  }
  
  if (!close_later)
  {
    return si->CloseSocketAndDestroy();
  }
  
  return PR_SUCCESS;
}

void nsSSLThread::restoreOriginalSocket_locked(nsNSSSocketInfo *si)
{
  if (si->mThreadData->mReplacedSSLFileDesc)
  {
    if (nsSSLIOLayerHelpers::mPollableEventCurrentlySet)
    {
      nsSSLIOLayerHelpers::mPollableEventCurrentlySet = PR_FALSE;
      if (nsSSLIOLayerHelpers::mSharedPollableEvent)
      {
        PR_WaitForPollableEvent(nsSSLIOLayerHelpers::mSharedPollableEvent);
      }
    }

    if (nsSSLIOLayerHelpers::mSharedPollableEvent)
    {
      
      si->mFd->lower = si->mThreadData->mReplacedSSLFileDesc;
      si->mThreadData->mReplacedSSLFileDesc = nsnull;
    }

    nsSSLIOLayerHelpers::mSocketOwningPollableEvent = nsnull;
  }
}

PRStatus nsSSLThread::getRealFDIfBlockingSocket_locked(nsNSSSocketInfo *si, 
                                                       PRFileDesc *&out_fd)
{
  out_fd = nsnull;

  PRFileDesc *realFD = 
    (si->mThreadData->mReplacedSSLFileDesc) ?
      si->mThreadData->mReplacedSSLFileDesc : si->mFd->lower;

  if (si->mBlockingState == nsNSSSocketInfo::blocking_state_unknown)
  {
    PRSocketOptionData sod;
    sod.option = PR_SockOpt_Nonblocking;
    if (PR_GetSocketOption(realFD, &sod) == PR_FAILURE)
      return PR_FAILURE;

    si->mBlockingState = sod.value.non_blocking ?
      nsNSSSocketInfo::is_nonblocking_socket : nsNSSSocketInfo::is_blocking_socket;
  }

  if (si->mBlockingState == nsNSSSocketInfo::is_blocking_socket)
  {
    out_fd = realFD;
  }

  return PR_SUCCESS;
}

PRInt32 nsSSLThread::requestRead(nsNSSSocketInfo *si, void *buf, PRInt32 amount, 
                                 PRIntervalTime timeout)
{
  if (!ssl_thread_singleton || !si || !buf || !amount || !ssl_thread_singleton->mThreadHandle)
  {
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return -1;
  }

  PRBool this_socket_is_busy = PR_FALSE;
  PRBool some_other_socket_is_busy = PR_FALSE;
  nsSSLSocketThreadData::ssl_state my_ssl_state;
  PRFileDesc *blockingFD = nsnull;

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (ssl_thread_singleton->mExitRequested) {
      PR_SetError(PR_UNKNOWN_ERROR, 0);
      return -1;
    }

    if (getRealFDIfBlockingSocket_locked(si, blockingFD) == PR_FAILURE) {
      return -1;
    }

    if (!blockingFD)
    {
      my_ssl_state = si->mThreadData->mSSLState;
  
      if (ssl_thread_singleton->mBusySocket == si)
      {
        this_socket_is_busy = PR_TRUE;
  
        if (my_ssl_state == nsSSLSocketThreadData::ssl_reading_done)
        {
          
          
          
          restoreOriginalSocket_locked(si);
  
          ssl_thread_singleton->mBusySocket = nsnull;
          
          
          
        }
      }
      else if (ssl_thread_singleton->mBusySocket)
      {
        some_other_socket_is_busy = PR_TRUE;
      }
  
      if (!this_socket_is_busy && si->HandshakeTimeout())
      {
        restoreOriginalSocket_locked(si);
        PR_SetError(PR_CONNECT_RESET_ERROR, 0);
        checkHandshake(-1, PR_TRUE, si->mFd->lower, si);
        return -1;
      }
    }
    
  }

  if (blockingFD)
  {
    
    
    return blockingFD->methods->recv(blockingFD, buf, amount, 0, timeout);
  }

  switch (my_ssl_state)
  {
    case nsSSLSocketThreadData::ssl_idle:
      {
        NS_ASSERTION(!this_socket_is_busy, "oops, unexpected incosistency");
        
        if (some_other_socket_is_busy)
        {
          PORT_SetError(PR_WOULD_BLOCK_ERROR);
          return -1;
        }
        
        
      }
      break;

    case nsSSLSocketThreadData::ssl_reading_done:
      
      {
        
        if (si->mThreadData->mSSLResultRemainingBytes < 0) {
          if (si->mThreadData->mPRErrorCode != PR_SUCCESS) {
            PR_SetError(si->mThreadData->mPRErrorCode, 0);
            si->mThreadData->mPRErrorCode = PR_SUCCESS;
          }

          si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_idle;
          return si->mThreadData->mSSLResultRemainingBytes;
        }

        PRInt32 return_amount = NS_MIN(amount, si->mThreadData->mSSLResultRemainingBytes);

        memcpy(buf, si->mThreadData->mSSLRemainingReadResultData, return_amount);

        si->mThreadData->mSSLResultRemainingBytes -= return_amount;

        if (!si->mThreadData->mSSLResultRemainingBytes) {
          si->mThreadData->mSSLRemainingReadResultData = nsnull;
          si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_idle;
        }
        else {
          si->mThreadData->mSSLRemainingReadResultData += return_amount;
        }

        return return_amount;
      }
      
      break;


    
    
    
    
    
    case nsSSLSocketThreadData::ssl_pending_write:
    case nsSSLSocketThreadData::ssl_pending_read:

    
    
    
    
    
    case nsSSLSocketThreadData::ssl_writing_done:

    
    
    
    default:
      {
        PORT_SetError(PR_WOULD_BLOCK_ERROR);
        return -1;
      }
      
      break;
  }

  if (si->isPK11LoggedOut() || si->isAlreadyShutDown()) {
    PR_SetError(PR_SOCKET_SHUTDOWN_ERROR, 0);
    return -1;
  }

  if (si->GetCanceled()) {
    return PR_FAILURE;
  }

  
  

  if (!si->mThreadData->ensure_buffer_size(amount))
  {
    PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    return -1;
  }
  
  si->mThreadData->mSSLRequestedTransferAmount = amount;
  si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_pending_read;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (nsSSLIOLayerHelpers::mSharedPollableEvent)
    {
      NS_ASSERTION(!nsSSLIOLayerHelpers::mSocketOwningPollableEvent, 
                   "oops, some other socket still owns our shared pollable event");
  
      NS_ASSERTION(!si->mThreadData->mReplacedSSLFileDesc, "oops");
  
      si->mThreadData->mReplacedSSLFileDesc = si->mFd->lower;
      si->mFd->lower = nsSSLIOLayerHelpers::mSharedPollableEvent;
    }

    nsSSLIOLayerHelpers::mSocketOwningPollableEvent = si;
    ssl_thread_singleton->mBusySocket = si;

    
    PR_NotifyAllCondVar(ssl_thread_singleton->mCond);
  }

  PORT_SetError(PR_WOULD_BLOCK_ERROR);
  return -1;
}

PRInt32 nsSSLThread::requestWrite(nsNSSSocketInfo *si, const void *buf, PRInt32 amount,
                                  PRIntervalTime timeout)
{
  if (!ssl_thread_singleton || !si || !buf || !amount || !ssl_thread_singleton->mThreadHandle)
  {
    PR_SetError(PR_UNKNOWN_ERROR, 0);
    return -1;
  }

  PRBool this_socket_is_busy = PR_FALSE;
  PRBool some_other_socket_is_busy = PR_FALSE;
  nsSSLSocketThreadData::ssl_state my_ssl_state;
  PRFileDesc *blockingFD = nsnull;
  
  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);
    
    if (ssl_thread_singleton->mExitRequested) {
      PR_SetError(PR_UNKNOWN_ERROR, 0);
      return -1;
    }

    if (getRealFDIfBlockingSocket_locked(si, blockingFD) == PR_FAILURE) {
      return -1;
    }

    if (!blockingFD)
    {
      my_ssl_state = si->mThreadData->mSSLState;
  
      if (ssl_thread_singleton->mBusySocket == si)
      {
        this_socket_is_busy = PR_TRUE;
        
        if (my_ssl_state == nsSSLSocketThreadData::ssl_writing_done)
        {
          
          
          
          restoreOriginalSocket_locked(si);
  
          ssl_thread_singleton->mBusySocket = nsnull;
          
          
          
        }
      }
      else if (ssl_thread_singleton->mBusySocket)
      {
        some_other_socket_is_busy = PR_TRUE;
      }
  
      if (!this_socket_is_busy && si->HandshakeTimeout())
      {
        restoreOriginalSocket_locked(si);
        PR_SetError(PR_CONNECT_RESET_ERROR, 0);
        checkHandshake(-1, PR_FALSE, si->mFd->lower, si);
        return -1;
      }
    }
    
  }

  if (blockingFD)
  {
    
    
    return blockingFD->methods->send(blockingFD, buf, amount, 0, timeout);
  }

  switch (my_ssl_state)
  {
    case nsSSLSocketThreadData::ssl_idle:
      {
        NS_ASSERTION(!this_socket_is_busy, "oops, unexpected incosistency");
        
        if (some_other_socket_is_busy)
        {
          PORT_SetError(PR_WOULD_BLOCK_ERROR);
          return -1;
        }
        
        
      }
      break;

    case nsSSLSocketThreadData::ssl_writing_done:
      
      {
        
        if (si->mThreadData->mSSLResultRemainingBytes < 0) {
          if (si->mThreadData->mPRErrorCode != PR_SUCCESS) {
            PR_SetError(si->mThreadData->mPRErrorCode, 0);
            si->mThreadData->mPRErrorCode = PR_SUCCESS;
          }

          si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_idle;
          return si->mThreadData->mSSLResultRemainingBytes;
        }

        PRInt32 return_amount = NS_MIN(amount, si->mThreadData->mSSLResultRemainingBytes);

        si->mThreadData->mSSLResultRemainingBytes -= return_amount;

        if (!si->mThreadData->mSSLResultRemainingBytes) {
          si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_idle;
        }

        return return_amount;
      }
      break;
      
    
    
    
    
    
    case nsSSLSocketThreadData::ssl_pending_write:
    case nsSSLSocketThreadData::ssl_pending_read:

    
    
    
    
    
    case nsSSLSocketThreadData::ssl_reading_done:

    
    
    
    default:
      {
        PORT_SetError(PR_WOULD_BLOCK_ERROR);
        return -1;
      }
      
      break;
  }

  if (si->isPK11LoggedOut() || si->isAlreadyShutDown()) {
    PR_SetError(PR_SOCKET_SHUTDOWN_ERROR, 0);
    return -1;
  }

  if (si->GetCanceled()) {
    return PR_FAILURE;
  }

  
  

  if (!si->mThreadData->ensure_buffer_size(amount))
  {
    PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    return -1;
  }

  memcpy(si->mThreadData->mSSLDataBuffer, buf, amount);
  si->mThreadData->mSSLRequestedTransferAmount = amount;
  si->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_pending_write;

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);

    if (nsSSLIOLayerHelpers::mSharedPollableEvent)
    {
      NS_ASSERTION(!nsSSLIOLayerHelpers::mSocketOwningPollableEvent, 
                   "oops, some other socket still owns our shared pollable event");
  
      NS_ASSERTION(!si->mThreadData->mReplacedSSLFileDesc, "oops");
  
      si->mThreadData->mReplacedSSLFileDesc = si->mFd->lower;
      si->mFd->lower = nsSSLIOLayerHelpers::mSharedPollableEvent;
    }

    nsSSLIOLayerHelpers::mSocketOwningPollableEvent = si;
    ssl_thread_singleton->mBusySocket = si;

    PR_NotifyAllCondVar(ssl_thread_singleton->mCond);
  }

  PORT_SetError(PR_WOULD_BLOCK_ERROR);
  return -1;
}

void nsSSLThread::Run(void)
{
  
  
  nsNSSSocketInfo *socketToDestroy = nsnull;

  while (PR_TRUE)
  {
    if (socketToDestroy)
    {
      socketToDestroy->CloseSocketAndDestroy();
      socketToDestroy = nsnull;
    }

    
    nsSSLSocketThreadData::ssl_state busy_socket_ssl_state;
  
    {
      
      
      
      nsAutoLock threadLock(ssl_thread_singleton->mMutex);
      
      if (mSocketScheduledToBeDestroyed)
      {
        if (mBusySocket == mSocketScheduledToBeDestroyed)
        {
          
          
          

          restoreOriginalSocket_locked(mBusySocket);

          mBusySocket->mThreadData->mSSLState = nsSSLSocketThreadData::ssl_idle;
          mBusySocket = nsnull;
        }
      
        socketToDestroy = mSocketScheduledToBeDestroyed;
        mSocketScheduledToBeDestroyed = nsnull;
        continue; 
      }

      if (mExitRequested)
        break;

      PRBool pending_work = PR_FALSE;

      do
      {
        if (mBusySocket
            &&
              (mBusySocket->mThreadData->mSSLState == nsSSLSocketThreadData::ssl_pending_read
              ||
              mBusySocket->mThreadData->mSSLState == nsSSLSocketThreadData::ssl_pending_write))
        {
          pending_work = PR_TRUE;
        }

        if (!pending_work)
        {
          

          PR_WaitCondVar(mCond, PR_INTERVAL_NO_TIMEOUT);
        }
        
      } while (!pending_work && !mExitRequested && !mSocketScheduledToBeDestroyed);
      
      if (mSocketScheduledToBeDestroyed)
        continue;
      
      if (mExitRequested)
        break;
      
      if (!pending_work)
        continue;
      
      busy_socket_ssl_state = mBusySocket->mThreadData->mSSLState;
    }

    {
      
      
    
      nsNSSShutDownPreventionLock locker;

      PRFileDesc *realFileDesc = mBusySocket->mThreadData->mReplacedSSLFileDesc;
      if (!realFileDesc)
      {
        realFileDesc = mBusySocket->mFd->lower;
      }

      if (nsSSLSocketThreadData::ssl_pending_write == busy_socket_ssl_state)
      {
        PRInt32 bytesWritten = realFileDesc->methods
          ->write(realFileDesc, 
                  mBusySocket->mThreadData->mSSLDataBuffer, 
                  mBusySocket->mThreadData->mSSLRequestedTransferAmount);

#ifdef DEBUG_SSL_VERBOSE
        PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] wrote %d bytes\n", (void*)fd, bytesWritten));
#endif
        
        bytesWritten = checkHandshake(bytesWritten, PR_FALSE, realFileDesc, mBusySocket);
        if (bytesWritten < 0) {
          
          mBusySocket->mThreadData->mPRErrorCode = PR_GetError();
        }

        mBusySocket->mThreadData->mSSLResultRemainingBytes = bytesWritten;
        busy_socket_ssl_state = nsSSLSocketThreadData::ssl_writing_done;
      }
      else if (nsSSLSocketThreadData::ssl_pending_read == busy_socket_ssl_state)
      {
        PRInt32 bytesRead = realFileDesc->methods
           ->read(realFileDesc, 
                  mBusySocket->mThreadData->mSSLDataBuffer, 
                  mBusySocket->mThreadData->mSSLRequestedTransferAmount);

#ifdef DEBUG_SSL_VERBOSE
        PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("[%p] read %d bytes\n", (void*)fd, bytesRead));
        DEBUG_DUMP_BUFFER((unsigned char*)buf, bytesRead);
#endif
        bytesRead = checkHandshake(bytesRead, PR_TRUE, realFileDesc, mBusySocket);
        if (bytesRead < 0) {
          
          mBusySocket->mThreadData->mPRErrorCode = PR_GetError();
        }

        mBusySocket->mThreadData->mSSLResultRemainingBytes = bytesRead;
        mBusySocket->mThreadData->mSSLRemainingReadResultData = 
          mBusySocket->mThreadData->mSSLDataBuffer;
        busy_socket_ssl_state = nsSSLSocketThreadData::ssl_reading_done;
      }
    }

    
    PRBool needToSetPollableEvent = PR_FALSE;

    {
      nsAutoLock threadLock(ssl_thread_singleton->mMutex);
      
      mBusySocket->mThreadData->mSSLState = busy_socket_ssl_state;
      
      if (!nsSSLIOLayerHelpers::mPollableEventCurrentlySet)
      {
        needToSetPollableEvent = PR_TRUE;
        nsSSLIOLayerHelpers::mPollableEventCurrentlySet = PR_TRUE;
      }
    }

    if (needToSetPollableEvent && nsSSLIOLayerHelpers::mSharedPollableEvent)
    {
      
      
      
      PR_SetPollableEvent(nsSSLIOLayerHelpers::mSharedPollableEvent);

      
      
    }
  }

  {
    nsAutoLock threadLock(ssl_thread_singleton->mMutex);
    if (mBusySocket)
    {
      restoreOriginalSocket_locked(mBusySocket);
      mBusySocket = nsnull;
    }
    if (!nsSSLIOLayerHelpers::mPollableEventCurrentlySet)
    {
      nsSSLIOLayerHelpers::mPollableEventCurrentlySet = PR_TRUE;
      if (nsSSLIOLayerHelpers::mSharedPollableEvent)
      {
        PR_SetPollableEvent(nsSSLIOLayerHelpers::mSharedPollableEvent);
      }
    }
  }
}

void nsSSLThread::rememberPendingHTTPRequest(nsIRequest *aRequest)
{
  if (!ssl_thread_singleton)
    return;

  nsAutoLock threadLock(ssl_thread_singleton->mMutex);

  NS_IF_ADDREF(aRequest);
  ssl_thread_singleton->mPendingHTTPRequest = aRequest;
}

void nsSSLThread::cancelPendingHTTPRequest()
{
  if (!ssl_thread_singleton)
    return;

  nsAutoLock threadLock(ssl_thread_singleton->mMutex);

  if (ssl_thread_singleton->mPendingHTTPRequest)
  {
    ssl_thread_singleton->mPendingHTTPRequest->Cancel(NS_ERROR_ABORT);

    NS_RELEASE(ssl_thread_singleton->mPendingHTTPRequest);

    ssl_thread_singleton->mPendingHTTPRequest = nsnull;
  }
}

nsSSLThread *nsSSLThread::ssl_thread_singleton = nsnull;
