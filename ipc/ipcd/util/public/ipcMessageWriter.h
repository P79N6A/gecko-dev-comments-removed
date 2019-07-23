





































#ifndef ipcMessageWriter_h__
#define ipcMessageWriter_h__

#include "prtypes.h"









class ipcMessageWriter
{
public:
                  ipcMessageWriter(PRUint32 initialCapacity) :
                    mBuf(NULL),
                    mBufPtr(NULL), mBufEnd(NULL),
                    mCapacity(0),
                    mError(PR_FALSE)
                  {
                    EnsureCapacity(initialCapacity);
                  }
                  
                  ~ipcMessageWriter();

                  
                  
                  
  PRBool          HasError()
                  { return mError; }

  void            PutInt8(PRUint8 val);
  void            PutInt16(PRUint16 val);
  void            PutInt32(PRUint32 val);  
  PRUint32        PutBytes(const void* src, PRUint32 n);

                  
  PRUint8*        GetBuffer()
                  { return mBuf; }

  PRInt32         GetSize()
                  { return mBufPtr - mBuf; }

private:
  PRBool          EnsureCapacity(PRInt32 sizeNeeded)
                  {
                    return (mBuf && ((mBufPtr + sizeNeeded) <= mBufEnd)) ?
                      PR_TRUE : GrowCapacity(sizeNeeded);
                  }
  PRBool          GrowCapacity(PRInt32 sizeNeeded);

private:
  PRUint8         *mBuf;
  PRUint8         *mBufPtr, *mBufEnd;
  PRInt32         mCapacity;
  PRBool          mError;
};

#endif 
