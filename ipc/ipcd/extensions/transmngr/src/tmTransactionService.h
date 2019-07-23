




































#ifndef _tmTransactionService_H_
#define _tmTransactionService_H_

#include "ipcdclient.h"
#include "ipcILockService.h"
#include "ipcIMessageObserver.h"
#include "ipcITransactionService.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "plhash.h"
#include "tmTransaction.h"
#include "tmVector.h"

struct tm_queue_mapping;


















class tmTransactionService : public ipcITransactionService,
                             public ipcIMessageObserver
{

public:

  
  
  tmTransactionService() : mObservers(0) {};

  


  virtual ~tmTransactionService();

  
  

  
  NS_DECL_ISUPPORTS
  NS_DECL_IPCITRANSACTIONSERVICE
  NS_DECL_IPCIMESSAGEOBSERVER

protected:

  
  

  










  void SendMessage(tmTransaction *aTrans, PRBool aSync);

  

  





  void OnAttachReply(tmTransaction *aTrans);

  



  void OnDetachReply(tmTransaction *aTrans);

  


  void OnFlushReply(tmTransaction *aTrans);

  


  void OnPost(tmTransaction *aTrans);

  

  




  void DispatchStoredMessages(tm_queue_mapping *aQMapping);

  

  



  PRInt32 GetQueueID(const nsACString & aDomainName);

  




  char* GetJoinedQueueName(PRUint32 aQueueID);

  




  char* GetJoinedQueueName(const nsACString & aDomainName);

  



  tm_queue_mapping* GetQueueMap(PRUint32 aQueueID);

  


  nsresult SendDetachOrFlush(PRUint32 aQueueID,
                             PRUint32 aAction,
                             PRBool aSync);

  
  

  nsCString mNamespace;               
  PLHashTable *mObservers;            

  tmVector mQueueMaps;                
  tmVector mWaitingMessages;          

  nsCOMPtr<ipcILockService> lockService;  

private:

};

#endif
