




































#include "ipcdclient.h"
#include "ipcConnection.h"
#include "ipcConfig.h"
#include "ipcMessageQ.h"
#include "ipcMessageUtils.h"
#include "ipcLog.h"
#include "ipcm.h"

#include "nsIFile.h"
#include "nsThreadUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "nsAutoLock.h"
#include "nsProxyRelease.h"
#include "nsCOMArray.h"

#include "prio.h"
#include "prproces.h"
#include "pratom.h"



#define IPC_REQUEST_TIMEOUT PR_SecondsToInterval(30)



class ipcTargetData
{
public:
  static NS_HIDDEN_(ipcTargetData*) Create();

  
  NS_HIDDEN_(nsrefcnt) AddRef()  { return PR_AtomicIncrement(&refcnt); }
  NS_HIDDEN_(nsrefcnt) Release() { PRInt32 r = PR_AtomicDecrement(&refcnt); if (r == 0) delete this; return r; }

  NS_HIDDEN_(void) SetObserver(ipcIMessageObserver *aObserver, PRBool aOnCurrentThread);

  
  PRMonitor *monitor;

  
  nsCOMPtr<ipcIMessageObserver> observer;

  
  nsCOMPtr<nsIThread> thread;
  
  
  ipcMessageQ pendingQ;

  
  
  
  PRInt32 observerDisabled;

private:

  ipcTargetData()
    : monitor(PR_NewMonitor())
    , observerDisabled(0)
    , refcnt(0)
    {}

  ~ipcTargetData()
  {
    if (monitor)
      PR_DestroyMonitor(monitor);
  }

  PRInt32 refcnt;
};

ipcTargetData *
ipcTargetData::Create()
{
  ipcTargetData *td = new ipcTargetData;
  if (!td)
    return NULL;

  if (!td->monitor)
  {
    delete td;
    return NULL;
  }
  return td;
}

void
ipcTargetData::SetObserver(ipcIMessageObserver *aObserver, PRBool aOnCurrentThread)
{
  observer = aObserver;

  if (aOnCurrentThread)
    NS_GetCurrentThread(getter_AddRefs(thread));
  else
    thread = nsnull;
}



typedef nsRefPtrHashtable<nsIDHashKey, ipcTargetData> ipcTargetMap; 

class ipcClientState
{
public:
  static NS_HIDDEN_(ipcClientState *) Create();

  ~ipcClientState()
  {
    if (monitor)
      PR_DestroyMonitor(monitor);
  }

  
  
  
  
  
  
  
  
  PRMonitor    *monitor;
  ipcTargetMap  targetMap;
  PRBool        connected;

  
  PRUint32      selfID; 

  nsCOMArray<ipcIClientObserver> clientObservers;

private:

  ipcClientState()
    : monitor(PR_NewMonitor())
    , connected(PR_FALSE)
    , selfID(0)
  {}
};

ipcClientState *
ipcClientState::Create()
{
  ipcClientState *cs = new ipcClientState;
  if (!cs)
    return NULL;

  if (!cs->monitor || !cs->targetMap.Init())
  {
    delete cs;
    return NULL;
  }

  return cs;
}



static ipcClientState *gClientState;

static PRBool
GetTarget(const nsID &aTarget, ipcTargetData **td)
{
  nsAutoMonitor mon(gClientState->monitor);
  return gClientState->targetMap.Get(nsIDHashKey(&aTarget).GetKey(), td);
}

static PRBool
PutTarget(const nsID &aTarget, ipcTargetData *td)
{
  nsAutoMonitor mon(gClientState->monitor);
  return gClientState->targetMap.Put(nsIDHashKey(&aTarget).GetKey(), td);
}

static void
DelTarget(const nsID &aTarget)
{
  nsAutoMonitor mon(gClientState->monitor);
  gClientState->targetMap.Remove(nsIDHashKey(&aTarget).GetKey());
}



static nsresult
GetDaemonPath(nsCString &dpath)
{
  nsCOMPtr<nsIFile> file;

  nsresult rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                       getter_AddRefs(file));
  if (NS_SUCCEEDED(rv))
  {
    rv = file->AppendNative(NS_LITERAL_CSTRING(IPC_DAEMON_APP_NAME));
    if (NS_SUCCEEDED(rv))
      rv = file->GetNativePath(dpath);
  }

  return rv;
}



static void
ProcessPendingQ(const nsID &aTarget)
{
  ipcMessageQ tempQ;

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);

    
    

    if (!td->observerDisabled)
      td->pendingQ.MoveTo(tempQ);
  }

  
  while (!tempQ.IsEmpty())
  {
    ipcMessage *msg = tempQ.First();

    if (td->observer)
      td->observer->OnMessageAvailable(msg->mMetaData,
                                       msg->Target(),
                                       (const PRUint8 *) msg->Data(),
                                       msg->DataLen());
    else
    {
      
      
      NS_ASSERTION(aTarget.Equals(IPCM_TARGET), "unexpected target");
      LOG(("dropping IPCM message: type=%x\n", IPCM_GetType(msg)));
    }
    tempQ.DeleteFirst();
  }
}






typedef PRBool (* ipcMessageSelector)(
  void *arg,
  ipcTargetData *td,
  const ipcMessage *msg
);


static PRBool
DefaultSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
  return PR_TRUE;
}

static nsresult
WaitTarget(const nsID           &aTarget,
           PRIntervalTime        aTimeout,
           ipcMessage          **aMsg,
           ipcMessageSelector    aSelector = nsnull,
           void                 *aArg = nsnull)
{
  *aMsg = nsnull;

  if (!aSelector)
    aSelector = DefaultSelector;

  nsRefPtr<ipcTargetData> td;
  if (!GetTarget(aTarget, getter_AddRefs(td)))
    return NS_ERROR_INVALID_ARG; 

  PRIntervalTime timeStart = PR_IntervalNow();
  PRIntervalTime timeEnd;
  if (aTimeout == PR_INTERVAL_NO_TIMEOUT)
    timeEnd = aTimeout;
  else if (aTimeout == PR_INTERVAL_NO_WAIT)
    timeEnd = timeStart;
  else
  {
    timeEnd = timeStart + aTimeout;

    
    if (timeEnd < timeStart)
      timeEnd = PR_INTERVAL_NO_TIMEOUT;
  }

  ipcMessage *lastChecked = nsnull, *beforeLastChecked = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  nsAutoMonitor mon(td->monitor);

  while (gClientState->connected)
  {
    NS_ASSERTION(!lastChecked, "oops");

    
    
    
    
    
    
    
    
    
    
    
    

    lastChecked = td->pendingQ.First();
    beforeLastChecked = nsnull;

    
    while (lastChecked)
    {
      if ((aSelector)(aArg, td, lastChecked))
      {
        
        if (beforeLastChecked)
          td->pendingQ.RemoveAfter(beforeLastChecked);
        else
          td->pendingQ.RemoveFirst();
        lastChecked->mNext = nsnull;

        *aMsg = lastChecked;
        break;
      }

      beforeLastChecked = lastChecked;
      lastChecked = lastChecked->mNext;
    }
      
    if (*aMsg)
    {
      rv = NS_OK;
      break;
    }

    
    if (!gClientState->connected)
    {
      rv = NS_ERROR_ABORT;
      break;
    }

    PRIntervalTime t = PR_IntervalNow();
    if (t > timeEnd) 
    {
      rv = IPC_ERROR_WOULD_BLOCK;
      break;
    }
    mon.Wait(timeEnd - t);

    LOG(("woke up from sleep [pendingQempty=%d connected=%d]\n",
          td->pendingQ.IsEmpty(), gClientState->connected));
  }

  return rv;
}



class ipcEvent_ClientState : public nsRunnable
{
public:
  ipcEvent_ClientState(PRUint32 aClientID, PRUint32 aClientState)
    : mClientID(aClientID)
    , mClientState(aClientState)
  {
  }

  NS_IMETHOD Run()
  {
    
    if (!gClientState)
      return nsnull;

    for (PRInt32 i=0; i<gClientState->clientObservers.Count(); ++i)
      gClientState->clientObservers[i]->OnClientStateChange(mClientID,
                                                            mClientState);
    return nsnull;
  }

private:
  PRUint32 mClientID;
  PRUint32 mClientState;
};



class ipcEvent_ProcessPendingQ : public nsRunnable
{
public:
  ipcEvent_ProcessPendingQ(const nsID &aTarget)
    : mTarget(aTarget)
  {
  }

  NS_IMETHOD Run()
  {
    ProcessPendingQ(mTarget);
    return NS_OK;
  }

private:
  const nsID mTarget;
};

static void
RunEvent(void *arg)
{
  nsIRunnable *ev = NS_STATIC_CAST(nsIRunnable *, arg);
  ev->Run();
  NS_RELEASE(ev);
}

static void
CallProcessPendingQ(const nsID &target, ipcTargetData *td)
{
  

  nsIRunnable *ev = new ipcEvent_ProcessPendingQ(target);
  if (!ev)
    return;
  NS_ADDREF(ev);

  nsresult rv;

  if (td->thread)
  {
    rv = td->thread->Dispatch(ev, NS_DISPATCH_NORMAL);
    NS_RELEASE(ev);
  }
  else 
  {
    rv = IPC_DoCallback(RunEvent, ev);
  }

  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to process pending queue");
}



static void
DisableMessageObserver(const nsID &aTarget)
{
  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);
    ++td->observerDisabled;
  }
}

static void
EnableMessageObserver(const nsID &aTarget)
{
  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);
    if (td->observerDisabled > 0 && --td->observerDisabled == 0)
      if (!td->pendingQ.IsEmpty())
        CallProcessPendingQ(aTarget, td);
  }
}




static PRBool
WaitIPCMResponseSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
  PRUint32 requestIndex = *(PRUint32 *) arg;
  return IPCM_GetRequestIndex(msg) == requestIndex;
}





static nsresult
WaitIPCMResponse(PRUint32 requestIndex, ipcMessage **responseMsg = nsnull)
{
  ipcMessage *msg;

  nsresult rv = WaitTarget(IPCM_TARGET, IPC_REQUEST_TIMEOUT, &msg,
                           WaitIPCMResponseSelector, &requestIndex);
  if (NS_FAILED(rv))
    return rv;

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_RESULT)
  {
    ipcMessageCast<ipcmMessageResult> result(msg);
    if (result->Status() < 0)
      rv = NS_ERROR_FAILURE; 
    else
      rv = NS_OK;
  }

  if (responseMsg)
    *responseMsg = msg;
  else
    delete msg;

  return rv;
}


static nsresult
MakeIPCMRequest(ipcMessage *msg, ipcMessage **responseMsg = nsnull)
{
  if (!msg)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 requestIndex = IPCM_GetRequestIndex(msg);

  
  
  
  
  
  DisableMessageObserver(IPCM_TARGET);

  nsresult rv = IPC_SendMsg(msg);
  if (NS_SUCCEEDED(rv))
    rv = WaitIPCMResponse(requestIndex, responseMsg);

  EnableMessageObserver(IPCM_TARGET);
  return rv;
}



static void
RemoveTarget(const nsID &aTarget, PRBool aNotifyDaemon)
{
  DelTarget(aTarget);

  if (aNotifyDaemon)
  {
    nsresult rv = MakeIPCMRequest(new ipcmMessageClientDelTarget(aTarget));
    if (NS_FAILED(rv))
      LOG(("failed to delete target: rv=%x\n", rv));
  }
}

static nsresult
DefineTarget(const nsID           &aTarget,
             ipcIMessageObserver  *aObserver,
             PRBool                aOnCurrentThread,
             PRBool                aNotifyDaemon,
             ipcTargetData       **aResult)
{
  nsresult rv;

  nsRefPtr<ipcTargetData> td( ipcTargetData::Create() );
  if (!td)
    return NS_ERROR_OUT_OF_MEMORY;
  td->SetObserver(aObserver, aOnCurrentThread);

  if (!PutTarget(aTarget, td))
    return NS_ERROR_OUT_OF_MEMORY;

  if (aNotifyDaemon)
  {
    rv = MakeIPCMRequest(new ipcmMessageClientAddTarget(aTarget));
    if (NS_FAILED(rv))
    {
      LOG(("failed to add target: rv=%x\n", rv));
      RemoveTarget(aTarget, PR_FALSE);
      return rv;
    }
  }

  if (aResult)
    NS_ADDREF(*aResult = td);
  return NS_OK;
}



static nsresult
TryConnect()
{
  nsCAutoString dpath;
  nsresult rv = GetDaemonPath(dpath);
  if (NS_FAILED(rv))
    return rv;
  
  rv = IPC_Connect(dpath.get());
  if (NS_FAILED(rv))
    return rv;

  gClientState->connected = PR_TRUE;

  rv = DefineTarget(IPCM_TARGET, nsnull, PR_FALSE, PR_FALSE, nsnull);
  if (NS_FAILED(rv))
    return rv;

  ipcMessage *msg;

  
  rv = MakeIPCMRequest(new ipcmMessageClientHello(), &msg);
  if (NS_FAILED(rv))
    return rv;

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_CLIENT_ID)
    gClientState->selfID = ipcMessageCast<ipcmMessageClientID>(msg)->ClientID();
  else
  {
    LOG(("unexpected response from CLIENT_HELLO message: type=%x!\n",
        IPCM_GetType(msg)));
    rv = NS_ERROR_UNEXPECTED;
  }

  delete msg;
  return rv;
}

nsresult
IPC_Init()
{
  NS_ENSURE_TRUE(!gClientState, NS_ERROR_ALREADY_INITIALIZED);

  IPC_InitLog(">>>");

  gClientState = ipcClientState::Create();
  if (!gClientState)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = TryConnect();
  if (NS_FAILED(rv))
    IPC_Shutdown();

  return rv;
}

nsresult
IPC_Shutdown()
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  if (gClientState->connected)
    IPC_Disconnect();

  delete gClientState;
  gClientState = nsnull;

  return NS_OK;
}



nsresult
IPC_DefineTarget(const nsID          &aTarget,
                 ipcIMessageObserver *aObserver,
                 PRBool               aOnCurrentThread)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  nsresult rv;

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(aTarget, getter_AddRefs(td)))
  {
    
    
    {
      nsAutoMonitor mon(td->monitor);
      td->SetObserver(aObserver, aOnCurrentThread);
    }

    
    
    if (!aObserver)
      RemoveTarget(aTarget, PR_TRUE);

    rv = NS_OK;
  }
  else
  {
    if (aObserver)
      rv = DefineTarget(aTarget, aObserver, aOnCurrentThread, PR_TRUE, nsnull);
    else
      rv = NS_ERROR_INVALID_ARG; 
  }

  return rv;
}

nsresult
IPC_DisableMessageObserver(const nsID &aTarget)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  DisableMessageObserver(aTarget);
  return NS_OK;
}

nsresult
IPC_EnableMessageObserver(const nsID &aTarget)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  EnableMessageObserver(aTarget);
  return NS_OK;
}

nsresult
IPC_SendMessage(PRUint32       aReceiverID,
                const nsID    &aTarget,
                const PRUint8 *aData,
                PRUint32       aDataLen)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  nsresult rv;
  if (aReceiverID == 0)
  {
    ipcMessage *msg = new ipcMessage(aTarget, (const char *) aData, aDataLen);
    if (!msg)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = IPC_SendMsg(msg);
  }
  else
    rv = MakeIPCMRequest(new ipcmMessageForward(IPCM_MSG_REQ_FORWARD,
                                                aReceiverID,
                                                aTarget,
                                                (const char *) aData,
                                                aDataLen));

  return rv;
}

struct WaitMessageSelectorData
{
  PRUint32             senderID;
  ipcIMessageObserver *observer;
};

static PRBool WaitMessageSelector(void *arg, ipcTargetData *td, const ipcMessage *msg)
{
  WaitMessageSelectorData *data = (WaitMessageSelectorData *) arg;

  nsresult rv = IPC_WAIT_NEXT_MESSAGE;

  if (msg->mMetaData == data->senderID)
  {
    ipcIMessageObserver *obs = data->observer;
    if (!obs)
      obs = td->observer;
    NS_ASSERTION(obs, "must at least have a default observer");

    rv = obs->OnMessageAvailable(msg->mMetaData,
                                 msg->Target(),
                                 (const PRUint8 *) msg->Data(),
                                 msg->DataLen());
  }

  
  return rv != IPC_WAIT_NEXT_MESSAGE;
}

nsresult
IPC_WaitMessage(PRUint32             aSenderID,
                const nsID          &aTarget,
                ipcIMessageObserver *aObserver,
                PRIntervalTime       aTimeout)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  
  if (aTarget.Equals(IPCM_TARGET))
    return NS_ERROR_INVALID_ARG;

  WaitMessageSelectorData data = { aSenderID, aObserver };

  ipcMessage *msg;
  nsresult rv = WaitTarget(aTarget, aTimeout, &msg, WaitMessageSelector, &data);
  if (NS_FAILED(rv))
    return rv;
  delete msg;

  return NS_OK;
}



nsresult
IPC_GetID(PRUint32 *aClientID)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  *aClientID = gClientState->selfID;
  return NS_OK;
}

nsresult
IPC_AddName(const char *aName)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return MakeIPCMRequest(new ipcmMessageClientAddName(aName));
}

nsresult
IPC_RemoveName(const char *aName)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return MakeIPCMRequest(new ipcmMessageClientDelName(aName));
}



nsresult
IPC_AddClientObserver(ipcIClientObserver *aObserver)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  return gClientState->clientObservers.AppendObject(aObserver)
      ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
IPC_RemoveClientObserver(ipcIClientObserver *aObserver)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  for (PRInt32 i = 0; i < gClientState->clientObservers.Count(); ++i)
  {
    if (gClientState->clientObservers[i] == aObserver)
      gClientState->clientObservers.RemoveObjectAt(i);
  }

  return NS_OK;
}




nsresult
IPC_ResolveClientName(const char *aName, PRUint32 *aClientID)
{
  NS_ENSURE_TRUE(gClientState, NS_ERROR_NOT_INITIALIZED);

  ipcMessage *msg;

  nsresult rv = MakeIPCMRequest(new ipcmMessageQueryClientByName(aName), &msg);
  if (NS_FAILED(rv))
    return rv;

  if (IPCM_GetType(msg) == IPCM_MSG_ACK_CLIENT_ID)
    *aClientID = ipcMessageCast<ipcmMessageClientID>(msg)->ClientID();
  else
  {
    LOG(("unexpected IPCM response: type=%x\n", IPCM_GetType(msg)));
    rv = NS_ERROR_UNEXPECTED;
  }
    
  delete msg;
  return rv;
}



nsresult
IPC_ClientExists(PRUint32 aClientID, PRBool *aResult)
{
  
  
  
  
  

  ipcmMessagePing ping;

  return MakeIPCMRequest(new ipcmMessageForward(IPCM_MSG_REQ_FORWARD,
                                                aClientID,
                                                IPCM_TARGET,
                                                ping.Data(),
                                                ping.DataLen()));
}



nsresult
IPC_SpawnDaemon(const char *path)
{
  PRFileDesc *readable = nsnull, *writable = nsnull;
  PRProcessAttr *attr = nsnull;
  nsresult rv = NS_ERROR_FAILURE;
  char *const argv[] = { (char *const) path, nsnull };
  char c;

  
  
  
  

  if (PR_CreatePipe(&readable, &writable) != PR_SUCCESS)
    goto end;
  PR_SetFDInheritable(writable, PR_TRUE);

  attr = PR_NewProcessAttr();
  if (!attr)
    goto end;

  if (PR_ProcessAttrSetInheritableFD(attr, writable, IPC_STARTUP_PIPE_NAME) != PR_SUCCESS)
    goto end;

  if (PR_CreateProcessDetached(path, argv, nsnull, attr) != PR_SUCCESS)
    goto end;

  if ((PR_Read(readable, &c, 1) != 1) && (c != IPC_STARTUP_PIPE_MAGIC))
    goto end;

  rv = NS_OK;
end:
  if (readable)
    PR_Close(readable);
  if (writable)
    PR_Close(writable);
  if (attr)
    PR_DestroyProcessAttr(attr);
  return rv;
}



PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateTargetMapAndNotify(const nsID    &aKey,
                            ipcTargetData *aData,
                            void          *aClosure)
{
  nsAutoMonitor mon(aData->monitor);

  
  mon.NotifyAll();

  return PL_DHASH_NEXT;
}


void
IPC_OnConnectionEnd(nsresult error)
{
  
  

  nsAutoMonitor mon(gClientState->monitor);
  gClientState->connected = PR_FALSE;
  gClientState->targetMap.EnumerateRead(EnumerateTargetMapAndNotify, nsnull);
}



#ifdef IPC_LOGGING
#include "prprf.h"
#include <ctype.h>
#endif


void
IPC_OnMessageAvailable(ipcMessage *msg)
{
#ifdef IPC_LOGGING
  if (LOG_ENABLED())
  {
    char *targetStr = msg->Target().ToString();
    LOG(("got message for target: %s\n", targetStr));
    nsMemory::Free(targetStr);

    IPC_LogBinary((const PRUint8 *) msg->Data(), msg->DataLen());
  }
#endif

  if (msg->Target().Equals(IPCM_TARGET))
  {
    switch (IPCM_GetType(msg))
    {
      
      case IPCM_MSG_PSH_FORWARD:
      {
        ipcMessageCast<ipcmMessageForward> fwd(msg);
        ipcMessage *innerMsg = new ipcMessage(fwd->InnerTarget(),
                                              fwd->InnerData(),
                                              fwd->InnerDataLen());
        
        innerMsg->mMetaData = fwd->ClientID();

        delete msg;

        
        IPC_OnMessageAvailable(innerMsg);
        return;
      }
      case IPCM_MSG_PSH_CLIENT_STATE:
      {
        ipcMessageCast<ipcmMessageClientState> status(msg);
        nsCOMPtr<nsIRunnable> ev =
            new ipcEvent_ClientState(status->ClientID(),
                                     status->ClientState());
        NS_DispatchToMainThread(ev);
        return;
      }
    }
  }

  nsRefPtr<ipcTargetData> td;
  if (GetTarget(msg->Target(), getter_AddRefs(td)))
  {
    nsAutoMonitor mon(td->monitor);

    
    
    PRBool dispatchEvent = td->pendingQ.IsEmpty();

    
    td->pendingQ.Append(msg);

    
    
    const nsID target = msg->Target();

    LOG(("placed message on pending queue for target and notifying all...\n"));

    
    mon.NotifyAll();

    
    if (dispatchEvent)
      CallProcessPendingQ(target, td);
  }
  else
  {
    NS_WARNING("message target is undefined");
  }
}
