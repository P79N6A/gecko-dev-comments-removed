




































#include "plstr.h"
#include "tmQueue.h"
#include "tmTransaction.h"
#include "tmTransactionManager.h"
#include "tmUtils.h"




tmQueue::~tmQueue() {

  
  PRUint32 index = 0;
  PRUint32 size = mTransactions.Size();
  for ( ; index < size ; index++) {
    if (mTransactions[index])
      delete (tmTransaction *)mTransactions[index];
  }

  
  

  mTM = nsnull;
  mID = 0;
  if (mName)
    PL_strfree(mName);
}




PRInt32
tmQueue::Init(const char* aName, PRUint32 aID, tmTransactionManager *aTM) {
  PR_ASSERT(mTM == nsnull);

  if (NS_SUCCEEDED(mTransactions.Init()) &&
      NS_SUCCEEDED(mListeners.Init()) &&
      ((mName = PL_strdup(aName)) != nsnull) ) {
    mTM = aTM;
    mID = aID;
    return NS_OK;
  }
  return -1;
}

PRInt32
tmQueue::AttachClient(PRUint32 aClientID) {

  PRInt32 status = NS_OK;                 

  if (!IsAttached(aClientID)) {
    
    status = mListeners.Append((void*) aClientID);
  }
  else
    status = -2;

  
  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(aClientID,        
                              mID,              
                              TM_ATTACH_REPLY,  
                              status,           
                              (PRUint8*)mName,  
                              PL_strlen(mName)+1))) {
    
    mTM->SendTransaction(aClientID, &trans);
  }

  
  if (status >= 0) { 
    
    PRUint32 size = mTransactions.Size();
    for (PRUint32 index = 0; index < size; index++) {
      if (mTransactions[index])
        mTM->SendTransaction(aClientID, (tmTransaction*) mTransactions[index]);
    }
  }
  return status;
}

PRInt32
tmQueue::DetachClient(PRUint32 aClientID) {

  PRUint32 size = mListeners.Size();
  PRUint32 id = 0;
  PRInt32 status = -1;

  for (PRUint32 index = 0; index < size; index++) {
    id = (PRUint32)NS_PTR_TO_INT32(mListeners[index]);
    if(id == aClientID) {
      mListeners.RemoveAt(index);
      status = NS_OK;
      break;
    }
  }

  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(aClientID,
                               mID,
                               TM_DETACH_REPLY,
                               status,
                               nsnull,
                               0))) {
    
    mTM->SendTransaction(aClientID, &trans);
  }

  
  if (mListeners.Size() == 0)
    return TM_SUCCESS_DELETE_QUEUE;
  return status;
}

void
tmQueue::FlushQueue(PRUint32 aClientID) {

  if(!IsAttached(aClientID))
    return;

  PRUint32 size = mTransactions.Size();
  for (PRUint32 index = 0; index < size; index++)
    if (mTransactions[index])
      delete (tmTransaction*)mTransactions[index];

  mTransactions.Clear();

  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(aClientID,
                               mID, 
                               TM_FLUSH_REPLY, 
                               NS_OK,
                               nsnull,
                               0))) {
    mTM->SendTransaction(aClientID, &trans);
  }
}

PRInt32
tmQueue::PostTransaction(tmTransaction *aTrans) {

  PRInt32 status = -1;
  PRUint32 ownerID = aTrans->GetOwnerID();

  
  
  

  if (IsAttached(ownerID) && aTrans->GetQueueID() == mID)
    status = mTransactions.Append(aTrans);

  if (status >= 0) {
    
    PRUint32 size = mListeners.Size();
    PRUint32 id = 0;
    for (PRUint32 index = 0; index < size; index++) {
      id = (PRUint32)NS_PTR_TO_INT32(mListeners[index]);
      if (ownerID != id)
        mTM->SendTransaction(id, aTrans);
    }
  }

  tmTransaction trans;
  if (NS_SUCCEEDED(trans.Init(ownerID,
                              mID,
                              TM_POST_REPLY,
                              status,
                              nsnull,
                              0))) {
    
    mTM->SendTransaction(ownerID, &trans);
  }
  return status;
}

PRBool
tmQueue::IsAttached(PRUint32 aClientID) {
  
  
  PRUint32 size = mListeners.Size();
  for (PRUint32 index = 0; index < size; index++) {
    if (aClientID == (PRUint32)NS_PTR_TO_INT32(mListeners[index]))
      return PR_TRUE;
  }
  return PR_FALSE;
}
