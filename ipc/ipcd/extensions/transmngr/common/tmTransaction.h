




































#ifndef _tmTransaction_H_
#define _tmTransaction_H_

#include "tmUtils.h"



























struct tmHeader {
  PRInt32  queueID;     
  PRUint32 action;      
  PRInt32  status;      
  PRUint32 reserved;    
};





































class tmTransaction
{

public:

  
  

  tmTransaction(): mHeader(nsnull), mRawMessageLength(0), mOwnerID(0) { }

  virtual ~tmTransaction();

  
  

  

  




























  nsresult Init(PRUint32 aOwnerID,
                PRInt32 aQueueID,
                PRUint32 aAction,
                PRInt32 aStatus,
                const PRUint8 *aMessage, 
                PRUint32 aLength);

  

  


  const PRUint8* GetMessage() const { return (PRUint8*)(mHeader + 1); }

  


  PRUint32 GetMessageLength() const { 
    return (mRawMessageLength > sizeof(tmHeader)) ?
      (mRawMessageLength - sizeof(tmHeader)) : 0;
  }

  




  const PRUint8* GetRawMessage() const { return (PRUint8*) mHeader; }

  


  PRUint32 GetRawMessageLength() const { return mRawMessageLength; }

  



  PRInt32 GetQueueID() const { return mHeader->queueID; }

  


  PRUint32 GetAction() const { return mHeader->action; }

  



  PRInt32 GetStatus() const { return mHeader->status; }

  



  PRUint32 GetOwnerID() const { return mOwnerID; }

  

  



  void SetQueueID(PRInt32 aID) { mHeader->queueID = aID; }

protected:

  
  

  tmHeader* mHeader;            
  PRUint32  mRawMessageLength;  
  PRUint32  mOwnerID;           

};

#endif
