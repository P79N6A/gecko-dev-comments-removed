




































#include <stdlib.h>
#include "tmTransaction.h"




tmTransaction::~tmTransaction() {
  if (mHeader)
    free(mHeader);
}



nsresult
tmTransaction::Init(PRUint32 aOwnerID,
                    PRInt32  aQueueID, 
                    PRUint32 aAction, 
                    PRInt32  aStatus, 
                    const PRUint8 *aMessage, 
                    PRUint32 aLength) {
  nsresult rv = NS_OK;
  tmHeader *header = nsnull;

  
  if (aQueueID == TM_INVALID_ID) {
    header = (tmHeader*) malloc(aLength);
    if (header) {
      mRawMessageLength = aLength;
      memcpy(header, aMessage, aLength);
    }
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }
  else {   
    header = (tmHeader*) malloc (sizeof(tmHeader) + aLength);
    if (header) {
      mRawMessageLength = sizeof(tmHeader) + aLength;
      header->action = aAction;
      header->queueID = aQueueID;
      header->status = aStatus;
      header->reserved = 0x00000000;
      if (aLength > 0)     
        memcpy((header + 1), aMessage, aLength);
    }
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }

  if (NS_SUCCEEDED(rv)) {
    mOwnerID = aOwnerID;
    mHeader = header;
  }

  return rv;
}
