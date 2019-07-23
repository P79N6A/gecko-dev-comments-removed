




































#ifndef _tmQueue_H_
#define _tmQueue_H_

#include "tmUtils.h"
#include "tmVector.h"

class tmClient;
class tmTransaction;
class tmTransactionManager;











class tmQueue
{

public:

  
  

  



  tmQueue(): mID(0), mName(nsnull), mTM(nsnull) { }

  



  virtual ~tmQueue();

  
  

  






  PRInt32 Init(const char* aName, PRUint32 aID, tmTransactionManager *aTM);

  

  

















  PRInt32 AttachClient(PRUint32 aClientID);

  











  PRInt32 DetachClient(PRUint32 aClientID);

  




  void FlushQueue(PRUint32 aClientID);

  














  PRInt32 PostTransaction(tmTransaction *aTrans);

  

  


  PRUint32 GetID() const { return mID; }

  


  const char* GetName() const { return mName; }

protected:

  





  PRBool IsAttached(PRUint32 aClientID);

  
  

  
  tmVector mTransactions;     
  tmVector mListeners;        

  
  PRUint32 mID;               
  char *mName;                
  tmTransactionManager *mTM;  

};

#endif
