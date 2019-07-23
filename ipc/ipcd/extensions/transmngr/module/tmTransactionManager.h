




































#ifndef _tmTransactionManager_H_
#define _tmTransactionManager_H_

#include "plhash.h"
#include "tmUtils.h"
#include "tmVector.h"
#include "tmIPCModule.h"


class tmQueue;
class tmClient;
class tmTransaction;









class tmTransactionManager
{

public:

  
  

  


  virtual ~tmTransactionManager();

  





  PRInt32 Init();

  
  

  



  void HandleTransaction(tmTransaction *aTrans);

  



  void SendTransaction(PRUint32 aDestClientIPCID, tmTransaction *aTrans) {
    PR_ASSERT(aTrans);
    tmIPCModule::SendMsg(aDestClientIPCID, aTrans);
  }

protected:

  
  

  

  


  tmQueue* GetQueue(PRUint32 aQueueID) {
    return (tmQueue*) mQueues[aQueueID];
  }

  



  tmQueue* GetQueue(const char *aQueueName);

  













  PRInt32 AddQueue(const char *aQueueType);

  

  void RemoveQueue(PRUint32 aQueueID);

  
  

  tmVector mQueues;

private:

};

#endif
