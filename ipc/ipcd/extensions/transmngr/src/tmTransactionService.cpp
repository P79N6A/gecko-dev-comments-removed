




































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "plstr.h"
#include "ipcITransactionObserver.h"
#include "tmTransaction.h"
#include "tmTransactionService.h"
#include "tmUtils.h"

static const nsID kTransModuleID = TRANSACTION_MODULE_ID;

struct tm_waiting_msg {
  tmTransaction trans;      
  char*         domainName; 

  ~tm_waiting_msg();
};

tm_waiting_msg::~tm_waiting_msg() {
  if (domainName)
    PL_strfree(domainName);
}

struct tm_queue_mapping {
  PRInt32 queueID;          
  char*   domainName;       
  char*   joinedQueueName;  

  ~tm_queue_mapping();
};

tm_queue_mapping::~tm_queue_mapping() {
  if (domainName)
    PL_strfree(domainName);
  if (joinedQueueName)
    PL_strfree(joinedQueueName);
}




tmTransactionService::~tmTransactionService() {

  
  if (mObservers)
    PL_HashTableDestroy(mObservers);

  PRUint32 index = 0;
  PRUint32 size = mWaitingMessages.Size();
  tm_waiting_msg *msg = nsnull;
  for ( ; index < size; index ++) {
    msg = (tm_waiting_msg*) mWaitingMessages[index];
    delete msg;
  }

  size = mQueueMaps.Size();
  tm_queue_mapping *qmap = nsnull;
  for (index = 0; index < size; index++) {
    qmap = (tm_queue_mapping*) mQueueMaps[index];
    if (qmap)
      delete qmap;
  }
}




NS_IMPL_ISUPPORTS2(tmTransactionService,
                   ipcITransactionService,
                   ipcIMessageObserver)




NS_IMETHODIMP
tmTransactionService::Init(const nsACString & aNamespace) {

  nsresult rv;
  
  rv = IPC_DefineTarget(kTransModuleID, this, PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  lockService = do_GetService("@mozilla.org/ipc/lock-service;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  
  mObservers = PL_NewHashTable(20, 
                               PL_HashString, 
                               PL_CompareStrings, 
                               PL_CompareValues, 0, 0);
  if (!mObservers)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mQueueMaps.Init();
  mWaitingMessages.Init();

  
  mNamespace.Assign(aNamespace);
  return NS_OK;
}

NS_IMETHODIMP
tmTransactionService::Attach(const nsACString & aDomainName, 
                             ipcITransactionObserver *aObserver,
                             PRBool aLockingCall) {

  
  
  if (GetQueueID(aDomainName) != TM_NO_ID)
    return TM_ERROR_QUEUE_EXISTS;
  if (!mObservers)
    return NS_ERROR_NOT_INITIALIZED;

  
  nsCString jQName;
  jQName.Assign(mNamespace);
  jQName.Append(aDomainName);

  
  char* joinedQueueName = ToNewCString(jQName);
  if (!joinedQueueName)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  PL_HashTableAdd(mObservers, joinedQueueName, aObserver);

  
  tm_queue_mapping *qm = new tm_queue_mapping();
  if (!qm)
    return NS_ERROR_OUT_OF_MEMORY;
  qm->queueID = TM_NO_ID;                   
  qm->joinedQueueName = joinedQueueName;    
  qm->domainName = ToNewCString(aDomainName);
  if (!qm->domainName) {
    PL_HashTableRemove(mObservers, joinedQueueName);
    delete qm;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mQueueMaps.Append(qm);

  nsresult rv = NS_ERROR_FAILURE;
  tmTransaction trans;

  
  if (aLockingCall)
    lockService->AcquireLock(joinedQueueName, PR_TRUE);
  

  if (NS_SUCCEEDED(trans.Init(0,                             
                              TM_NO_ID,                      
                              TM_ATTACH,                     
                              NS_OK,                         
                              (PRUint8 *)joinedQueueName,    
                              PL_strlen(joinedQueueName)+1))) { 
    
    SendMessage(&trans, PR_TRUE);  
    rv = NS_OK;
  }

  
  if (aLockingCall)
    lockService->ReleaseLock(joinedQueueName);

  return rv;
}


NS_IMETHODIMP
tmTransactionService::Detach(const nsACString & aDomainName) {

  
  return SendDetachOrFlush(GetQueueID(aDomainName), TM_DETACH, PR_FALSE);

}

NS_IMETHODIMP
tmTransactionService::Flush(const nsACString & aDomainName,
                            PRBool aLockingCall) {
  
  if (aLockingCall)
    lockService->AcquireLock(GetJoinedQueueName(aDomainName), PR_TRUE);

  
  nsresult rv = SendDetachOrFlush(GetQueueID(aDomainName), TM_FLUSH, PR_TRUE);

  
  if (aLockingCall)
    lockService->ReleaseLock(GetJoinedQueueName(aDomainName));

  return rv;

}

NS_IMETHODIMP
tmTransactionService::PostTransaction(const nsACString & aDomainName, 
                                      const PRUint8 *aData, 
                                      PRUint32 aDataLen) {

  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(0,                       
                              GetQueueID(aDomainName), 
                              TM_POST,                 
                              NS_OK,                   
                              aData,                   
                              aDataLen))) {             
    if (trans.GetQueueID() == TM_NO_ID) {
      
      tm_waiting_msg *msg = new tm_waiting_msg(); 
      if (!msg)
        return NS_ERROR_OUT_OF_MEMORY;
      msg->trans = trans;
      msg->domainName = ToNewCString(aDomainName);
      if (!msg->domainName) {
        delete msg;
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mWaitingMessages.Append(msg);
    }
    else {
      
      SendMessage(&trans, PR_FALSE);
    }
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP
tmTransactionService::OnMessageAvailable(const PRUint32 aSenderID,
                                         const nsID & aTarget, 
                                         const PRUint8 *aData, 
                                         PRUint32 aDataLength) {

  nsresult rv = NS_ERROR_OUT_OF_MEMORY; 

  tmTransaction *trans = new tmTransaction();
  if (trans) {
    rv = trans->Init(0,                      
                     TM_INVALID_ID,          
                     TM_INVALID_ID,          
                     TM_INVALID_ID,          
                     aData,                  
                     aDataLength);           

    if (NS_SUCCEEDED(rv)) {
      switch(trans->GetAction()) {
      case TM_ATTACH_REPLY:
        OnAttachReply(trans);
        break;
      case TM_POST_REPLY:
        
        
        break;
      case TM_DETACH_REPLY:
        OnDetachReply(trans);
        break;
      case TM_FLUSH_REPLY:
        OnFlushReply(trans);
        break;
      case TM_POST:
        OnPost(trans);
        break;
      default: 
        NS_NOTREACHED("Received a TM reply outside of mapped messages");
        break;
      }
    }
    delete trans;
  }
  return rv;
}




void
tmTransactionService::SendMessage(tmTransaction *aTrans, PRBool aSync) {

  NS_ASSERTION(aTrans, "tmTransactionService::SendMessage called with null transaction");

  IPC_SendMessage(0, kTransModuleID, 
                  aTrans->GetRawMessage(), 
                  aTrans->GetRawMessageLength());
  if (aSync)
    IPC_WaitMessage(0, kTransModuleID, nsnull, PR_INTERVAL_NO_TIMEOUT);
}

void
tmTransactionService::OnAttachReply(tmTransaction *aTrans) {

  
  if (aTrans->GetStatus() >= 0) {

    PRUint32 size = mQueueMaps.Size();
    tm_queue_mapping *qmap = nsnull;
    for (PRUint32 index = 0; index < size; index++) {
      qmap = (tm_queue_mapping*) mQueueMaps[index];
      if (qmap && 
          PL_strcmp(qmap->joinedQueueName, (char*) aTrans->GetMessage()) == 0) {

        
        qmap->queueID = aTrans->GetQueueID();
        
        DispatchStoredMessages(qmap);
      }
    }
  }

  
  ipcITransactionObserver *observer = 
    (ipcITransactionObserver *)PL_HashTableLookup(mObservers, 
                                                 (char*)aTrans->GetMessage());
  if (observer)
    observer->OnAttachReply(aTrans->GetQueueID(), aTrans->GetStatus());
}

void
tmTransactionService::OnDetachReply(tmTransaction *aTrans) {

  tm_queue_mapping *qmap = GetQueueMap(aTrans->GetQueueID());

  
  ipcITransactionObserver *observer = 
    (ipcITransactionObserver *)PL_HashTableLookup(mObservers, 
                                                 qmap->joinedQueueName);

  
  if (aTrans->GetStatus() >= 0) {

    
    PL_HashTableRemove(mObservers, qmap->joinedQueueName);

    
    mQueueMaps.Remove(qmap);
    delete qmap;
  }


  
  if (observer)
    observer->OnDetachReply(aTrans->GetQueueID(), aTrans->GetStatus());
}

void
tmTransactionService::OnFlushReply(tmTransaction *aTrans) {

  ipcITransactionObserver *observer = 
    (ipcITransactionObserver *)PL_HashTableLookup(mObservers, 
                              GetJoinedQueueName(aTrans->GetQueueID()));
  if (observer)
    observer->OnFlushReply(aTrans->GetQueueID(), aTrans->GetStatus());
}

void
tmTransactionService::OnPost(tmTransaction *aTrans) {

  ipcITransactionObserver *observer = 
    (ipcITransactionObserver*) PL_HashTableLookup(mObservers, 
                              GetJoinedQueueName(aTrans->GetQueueID()));
  if (observer)
    observer->OnTransactionAvailable(aTrans->GetQueueID(), 
                                     aTrans->GetMessage(), 
                                     aTrans->GetMessageLength());
}

void
tmTransactionService::DispatchStoredMessages(tm_queue_mapping *aQMapping) {

  PRUint32 size = mWaitingMessages.Size();
  tm_waiting_msg *msg = nsnull;
  for (PRUint32 index = 0; index < size; index ++) {
    msg = (tm_waiting_msg*) mWaitingMessages[index];
    
    if (msg && strcmp(aQMapping->domainName, msg->domainName) == 0) {

      
      msg->trans.SetQueueID(aQMapping->queueID);
      SendMessage(&(msg->trans), PR_FALSE);

      
      mWaitingMessages.Remove(msg);
      delete msg;
    }
  }
}


PRInt32
tmTransactionService::GetQueueID(const nsACString & aDomainName) {

  PRUint32 size = mQueueMaps.Size();
  tm_queue_mapping *qmap = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
    qmap = (tm_queue_mapping*) mQueueMaps[index];
    if (qmap && aDomainName.Equals(qmap->domainName))
      return qmap->queueID;
  }
  return TM_NO_ID;
}

char*
tmTransactionService::GetJoinedQueueName(PRUint32 aQueueID) {

  PRUint32 size = mQueueMaps.Size();
  tm_queue_mapping *qmap = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
    qmap = (tm_queue_mapping*) mQueueMaps[index];
    if (qmap && qmap->queueID == aQueueID)
      return qmap->joinedQueueName;
  }
  return nsnull;
}

char*
tmTransactionService::GetJoinedQueueName(const nsACString & aDomainName) {

  PRUint32 size = mQueueMaps.Size();
  tm_queue_mapping *qmap = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
    qmap = (tm_queue_mapping*) mQueueMaps[index];
    if (qmap && aDomainName.Equals(qmap->domainName))
      return qmap->joinedQueueName;
  }
  return nsnull;
}

tm_queue_mapping*
tmTransactionService::GetQueueMap(PRUint32 aQueueID) {

  PRUint32 size = mQueueMaps.Size();
  tm_queue_mapping *qmap = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
    qmap = (tm_queue_mapping*) mQueueMaps[index];
    if (qmap && qmap->queueID == aQueueID)
      return qmap;
  }
  return nsnull;
}

nsresult
tmTransactionService::SendDetachOrFlush(PRUint32 aQueueID,
                                        PRUint32 aAction, 
                                        PRBool aSync) {

  
  if (aQueueID == TM_NO_ID)
    return NS_ERROR_UNEXPECTED;

  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(0,         
                              aQueueID,  
                              aAction,   
                              NS_OK,     
                              nsnull,    
                              0))) {      
    
    SendMessage(&trans, aSync);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}
