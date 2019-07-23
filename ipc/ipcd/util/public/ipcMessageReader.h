





































#ifndef ipcMessageReader_h__
#define ipcMessageReader_h__

#include "prtypes.h"









class ipcMessageReader
{
public:
                  ipcMessageReader(const PRUint8* inBuffer, PRUint32 bufferSize) :
                    mBuf(inBuffer), mBufEnd(inBuffer + bufferSize),
                    mBufPtr(mBuf),
                    mError(PR_FALSE)
                  { }
                  
                  ~ipcMessageReader()
                  { }

                  
                  
                  
  PRBool          HasError()
                  { return mError; }
  
  PRUint8         GetInt8();
  PRUint16        GetInt16();
  PRUint32        GetInt32();
  PRInt32         GetBytes(void* destBuffer, PRInt32 n);
  
                  
  const PRUint8*  GetPtr()
                  { return mBufPtr; }

                  
                  
  PRBool          AdvancePtr(PRInt32 n);
      
private:
  const PRUint8   *mBuf, *mBufEnd;
  const PRUint8   *mBufPtr;
  PRBool          mError;
};

#endif 
