




































#include "private/pprio.h"
#include "prerror.h"
#include "prthread.h"
#include "prlock.h"
#include "prlog.h" 
#include "prio.h"

#include "ipcConnection.h"
#include "ipcMessageQ.h"
#include "ipcConfig.h"
#include "ipcLog.h"












#ifndef XP_OS2
#define IPC_SKIP_SECURITY_CHECKS
#endif

#ifndef IPC_SKIP_SECURITY_CHECKS
#include <unistd.h>
#include <sys/stat.h>
#endif

static PRStatus
DoSecurityCheck(PRFileDesc *fd, const char *path)
{
#ifndef IPC_SKIP_SECURITY_CHECKS
    
    
    
    
    
    
    
    
    
    int unix_fd = PR_FileDesc2NativeHandle(fd);  

    struct stat st;
    if (fstat(unix_fd, &st) == -1) {
        LOG(("stat failed"));
        return PR_FAILURE;
    }

    if (st.st_uid != getuid() && st.st_uid != geteuid()) {
        
        
        
        
        
        
        
        if (st.st_uid != 0) {
            LOG(("userid check failed"));
            return PR_FAILURE;
        }
        if (stat(path, &st) == -1) {
            LOG(("stat failed"));
            return PR_FAILURE;
        }
        if (st.st_uid != getuid() && st.st_uid != geteuid()) {
            LOG(("userid check failed"));
            return PR_FAILURE;
        }
    }
#endif
    return PR_SUCCESS;
}



struct ipcCallback : public ipcListNode<ipcCallback>
{
  ipcCallbackFunc  func;
  void            *arg;
};

typedef ipcList<ipcCallback> ipcCallbackQ;



struct ipcConnectionState
{
  PRLock      *lock;
  PRPollDesc   fds[2];
  ipcCallbackQ callback_queue;
  ipcMessageQ  send_queue;
  PRUint32     send_offset; 
  ipcMessage  *in_msg;
  PRBool       shutdown;
};

#define SOCK 0
#define POLL 1

static void
ConnDestroy(ipcConnectionState *s)
{
  if (s->lock)
    PR_DestroyLock(s->lock);  

  if (s->fds[SOCK].fd)
    PR_Close(s->fds[SOCK].fd);

  if (s->fds[POLL].fd)
    PR_DestroyPollableEvent(s->fds[POLL].fd);

  if (s->in_msg)
    delete s->in_msg;

  s->send_queue.DeleteAll();
  delete s;
}

static ipcConnectionState *
ConnCreate(PRFileDesc *fd)
{
  ipcConnectionState *s = new ipcConnectionState;
  if (!s)
    return NULL;

  s->lock = PR_NewLock();
  s->fds[SOCK].fd = NULL;
  s->fds[POLL].fd = PR_NewPollableEvent();
  s->send_offset = 0;
  s->in_msg = NULL;
  s->shutdown = PR_FALSE;

  if (!s->lock || !s->fds[1].fd)
  {
    ConnDestroy(s);
    return NULL;
  }

  
  s->fds[SOCK].fd = fd;

  return s;
}

static nsresult
ConnRead(ipcConnectionState *s)
{
  char buf[1024];
  nsresult rv = NS_OK;
  PRInt32 n;

  do
  {
    n = PR_Read(s->fds[SOCK].fd, buf, sizeof(buf));
    if (n < 0)
    {
      PRErrorCode err = PR_GetError();
      if (err == PR_WOULD_BLOCK_ERROR)
      {
        
        break;
      }
      LOG(("PR_Read returned failure [err=%d]\n", err));
      rv = NS_ERROR_UNEXPECTED;
    }
    else if (n == 0)
    {
      LOG(("PR_Read returned EOF\n"));
      rv = NS_ERROR_UNEXPECTED;
    }
    else
    {
      const char *pdata = buf;
      while (n)
      {
        PRUint32 bytesRead;
        PRBool complete;

        if (!s->in_msg)
        {
          s->in_msg = new ipcMessage;
          if (!s->in_msg)
          {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        if (s->in_msg->ReadFrom(pdata, n, &bytesRead, &complete) != PR_SUCCESS)
        {
          LOG(("error reading IPC message\n"));
          rv = NS_ERROR_UNEXPECTED;
          break;
        }

        PR_ASSERT(PRUint32(n) >= bytesRead);
        n -= bytesRead;
        pdata += bytesRead;

        if (complete)
        {
          
          ipcMessage *m = s->in_msg;
          s->in_msg = NULL;

          IPC_OnMessageAvailable(m);
        }
      }
    }
  }
  while (NS_SUCCEEDED(rv));

  return rv;
}

static nsresult
ConnWrite(ipcConnectionState *s)
{
  nsresult rv = NS_OK;

  PR_Lock(s->lock);

  
  if (s->send_queue.First())
  {
    PRInt32 n = PR_Write(s->fds[SOCK].fd,
                         s->send_queue.First()->MsgBuf() + s->send_offset,
                         s->send_queue.First()->MsgLen() - s->send_offset);
    if (n <= 0)
    {
      PRErrorCode err = PR_GetError();
      if (err == PR_WOULD_BLOCK_ERROR)
      {
        
      }
      else
      {
        LOG(("error writing to socket [err=%d]\n", err));
        rv = NS_ERROR_UNEXPECTED;
      }
    }
    else
    {
      s->send_offset += n;
      if (s->send_offset == s->send_queue.First()->MsgLen())
      {
        s->send_queue.DeleteFirst();
        s->send_offset = 0;

        
        if (s->send_queue.IsEmpty())
          s->fds[SOCK].in_flags &= ~PR_POLL_WRITE;
      }
    }
  }

  PR_Unlock(s->lock);
  return rv;
}

PR_STATIC_CALLBACK(void)
ConnThread(void *arg)
{
  PRInt32 num;
  nsresult rv = NS_OK;

  ipcConnectionState *s = (ipcConnectionState *) arg;

  
  
  
  

  s->fds[SOCK].in_flags = PR_POLL_READ;
  s->fds[POLL].in_flags = PR_POLL_READ;

  while (NS_SUCCEEDED(rv))
  {
    s->fds[SOCK].out_flags = 0;
    s->fds[POLL].out_flags = 0;

    
    
    
    num = PR_Poll(s->fds, 2, PR_INTERVAL_NO_TIMEOUT);
    if (num > 0)
    {
      ipcCallbackQ cbs_to_run;

      
      
      

      if (s->fds[POLL].out_flags & PR_POLL_READ)
      {
        PR_WaitForPollableEvent(s->fds[POLL].fd);
        PR_Lock(s->lock);

        if (!s->send_queue.IsEmpty())
          s->fds[SOCK].in_flags |= PR_POLL_WRITE;

        if (!s->callback_queue.IsEmpty())
          s->callback_queue.MoveTo(cbs_to_run);

        PR_Unlock(s->lock);
      }

      
      if (s->fds[SOCK].out_flags & PR_POLL_READ)
        rv = ConnRead(s);

      
      if (s->fds[SOCK].out_flags & PR_POLL_WRITE)
        rv = ConnWrite(s);

      
      while (!cbs_to_run.IsEmpty())
      {
        ipcCallback *cb = cbs_to_run.First();
        (cb->func)(cb->arg);
        cbs_to_run.DeleteFirst();
      }

      
      
      
      PR_Lock(s->lock);
      if (s->shutdown && s->send_queue.IsEmpty() && s->callback_queue.IsEmpty())
        rv = NS_ERROR_ABORT;
      PR_Unlock(s->lock);
    }
    else
    {
      LOG(("PR_Poll returned an error\n"));
      rv = NS_ERROR_UNEXPECTED;
    }
  }

  
  if (rv == NS_ERROR_ABORT)
    rv = NS_OK;
  IPC_OnConnectionEnd(rv);

  LOG(("IPC thread exiting\n"));
}





static ipcConnectionState *gConnState = NULL;
static PRThread *gConnThread = NULL;

#ifdef DEBUG
static PRThread *gMainThread = NULL;
#endif

nsresult
TryConnect(PRFileDesc **result)
{
  PRFileDesc *fd;
  PRNetAddr addr;
  PRSocketOptionData opt;
  nsresult rv = NS_ERROR_FAILURE;

  fd = PR_OpenTCPSocket(PR_AF_LOCAL);
  if (!fd)
    goto end;

  addr.local.family = PR_AF_LOCAL;
  IPC_GetDefaultSocketPath(addr.local.path, sizeof(addr.local.path));

  
  if (PR_Connect(fd, &addr, PR_INTERVAL_NO_TIMEOUT) == PR_FAILURE)
    goto end;

  
  opt.option = PR_SockOpt_Nonblocking;
  opt.value.non_blocking = PR_TRUE;
  PR_SetSocketOption(fd, &opt);

  
  if (DoSecurityCheck(fd, addr.local.path) != PR_SUCCESS)
    goto end;
  
  *result = fd;
  return NS_OK;

end:
  if (fd)
    PR_Close(fd);

  return rv;
}

nsresult
IPC_Connect(const char *daemonPath)
{
  

  PRFileDesc *fd;
  nsresult rv = NS_ERROR_FAILURE;

  if (gConnState)
    return NS_ERROR_ALREADY_INITIALIZED;

  
  
  
  
  
  
  

  rv = TryConnect(&fd);
  if (NS_FAILED(rv))
  {
    rv = IPC_SpawnDaemon(daemonPath);
    if (NS_SUCCEEDED(rv))
      rv = TryConnect(&fd);
  }

  if (NS_FAILED(rv))
    goto end;

  
  
  

  
  gConnState = ConnCreate(fd);
  if (!gConnState)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto end;
  }
  fd = NULL; 

  gConnThread = PR_CreateThread(PR_USER_THREAD,
                                ConnThread,
                                gConnState,
                                PR_PRIORITY_NORMAL,
                                PR_GLOBAL_THREAD,
                                PR_JOINABLE_THREAD,
                                0);
  if (!gConnThread)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    goto end;
  }

#ifdef DEBUG
  gMainThread = PR_GetCurrentThread();
#endif
  return NS_OK;

end:
  if (gConnState)
  {
    ConnDestroy(gConnState);
    gConnState = NULL;
  }
  if (fd)
    PR_Close(fd);
  return rv;
}

nsresult
IPC_Disconnect()
{
  
  PR_ASSERT(gMainThread == PR_GetCurrentThread());

  if (!gConnState || !gConnThread)
    return NS_ERROR_NOT_INITIALIZED;

  PR_Lock(gConnState->lock);
  gConnState->shutdown = PR_TRUE;
  PR_SetPollableEvent(gConnState->fds[POLL].fd);
  PR_Unlock(gConnState->lock);

  PR_JoinThread(gConnThread);

  ConnDestroy(gConnState);

  gConnState = NULL;
  gConnThread = NULL;
  return NS_OK;
}

nsresult
IPC_SendMsg(ipcMessage *msg)
{
  if (!gConnState || !gConnThread)
    return NS_ERROR_NOT_INITIALIZED;

  PR_Lock(gConnState->lock);
  gConnState->send_queue.Append(msg);
  PR_SetPollableEvent(gConnState->fds[POLL].fd);
  PR_Unlock(gConnState->lock);

  return NS_OK;
}

nsresult
IPC_DoCallback(ipcCallbackFunc func, void *arg)
{
  if (!gConnState || !gConnThread)
    return NS_ERROR_NOT_INITIALIZED;
  
  ipcCallback *callback = new ipcCallback;
  if (!callback)
    return NS_ERROR_OUT_OF_MEMORY;
  callback->func = func;
  callback->arg = arg;

  PR_Lock(gConnState->lock);
  gConnState->callback_queue.Append(callback);
  PR_SetPollableEvent(gConnState->fds[POLL].fd);
  PR_Unlock(gConnState->lock);
  return NS_OK;
}



#ifdef TEST_STANDALONE

void IPC_OnConnectionFault(nsresult rv)
{
  LOG(("IPC_OnConnectionFault [rv=%x]\n", rv));
}

void IPC_OnMessageAvailable(ipcMessage *msg)
{
  LOG(("IPC_OnMessageAvailable\n"));
  delete msg;
}

int main()
{
  IPC_InitLog(">>>");
  IPC_Connect("/builds/moz-trunk/seamonkey-debug-build/dist/bin/mozilla-ipcd");
  IPC_Disconnect();
  return 0;
}

#endif
