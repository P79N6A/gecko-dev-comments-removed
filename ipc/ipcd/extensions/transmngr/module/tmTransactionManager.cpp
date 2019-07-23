




































#include "plstr.h"
#include <stdlib.h>
#include "tmQueue.h"
#include "tmTransactionManager.h"
#include "tmTransaction.h"
#include "tmUtils.h"




tmTransactionManager::~tmTransactionManager() {

  PRUint32 size = mQueues.Size();
  tmQueue *queue = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
     queue = (tmQueue *)mQueues[index];
     if (queue) {
       delete queue;
     }
  }
}

PRInt32
tmTransactionManager::Init() {
  return mQueues.Init();
}




void
tmTransactionManager::HandleTransaction(tmTransaction *aTrans) {

  PRUint32 action = aTrans->GetAction();
  PRUint32 ownerID = aTrans->GetOwnerID();
  tmQueue *queue = nsnull;

  
  if (action == TM_ATTACH) {
    const char *name = (char*) aTrans->GetMessage(); 
    queue = GetQueue(name);  
    if (!queue) {
      PRInt32 index = AddQueue(name);
      if (index >= 0)
        queue = GetQueue(index); 
    }
  }
  else  
    queue = GetQueue(aTrans->GetQueueID());

  if (queue) {
    
    
    
    PRInt32 result = 0;
    switch (action) {
    case TM_ATTACH:
      queue->AttachClient(ownerID);
      break;
    case TM_POST:
      result = queue->PostTransaction(aTrans);
      if (result >= 0) 
        return;
      break;
    case TM_FLUSH:
      queue->FlushQueue(ownerID);
      break;
    case TM_DETACH:
      if (queue->DetachClient(ownerID) == TM_SUCCESS_DELETE_QUEUE) {
        
        RemoveQueue(aTrans->GetQueueID()); 
      }
      break;
    default:
      PR_NOT_REACHED("bad action in the transaction");
    }
  }
  delete aTrans;
}








tmQueue*
tmTransactionManager::GetQueue(const char *aQueueName) {

  PRUint32 size = mQueues.Size();
  tmQueue *queue = nsnull;
  for (PRUint32 index = 0; index < size; index++) {
    queue = (tmQueue*) mQueues[index];
    if (queue && strcmp(queue->GetName(), aQueueName) == 0)
      return queue;
  }
  return nsnull;
}


PRInt32
tmTransactionManager::AddQueue(const char *aQueueName) {

  tmQueue* queue = new tmQueue();
  if (!queue)
    return -1;
  PRInt32 index = mQueues.Append(queue);
  if (index < 0)
    delete queue;
  else
    queue->Init(aQueueName, index, this);
  return index;
}

void
tmTransactionManager::RemoveQueue(PRUint32 aQueueID) {
  PR_ASSERT(aQueueID <= mQueues.Size());

  tmQueue *queue = (tmQueue*)mQueues[aQueueID];
  if (queue) {
    mQueues.RemoveAt(aQueueID);
    delete queue;
  }
}
