





































#include "ipcMessageWriter.h"
#include "prmem.h"
#include <string.h>





ipcMessageWriter::~ipcMessageWriter()
{
  if (mBuf)
    free(mBuf);
}
  
void ipcMessageWriter::PutInt8(PRUint8 val)
{
  if (EnsureCapacity(sizeof(PRUint8))) 
    *mBufPtr++ = val;
}




  
void ipcMessageWriter::PutInt16(PRUint16 val)
{
  if (EnsureCapacity(sizeof(PRUint16))) {
    PRUint8 temp[2];
    *(PRUint16*)temp = val;
    *mBufPtr++ = temp[0];
    *mBufPtr++ = temp[1];
  }
}
  
void ipcMessageWriter::PutInt32(PRUint32 val)
{
  if (EnsureCapacity(sizeof(PRUint32))) {
    PRUint8 temp[4];
    *(PRUint32*)temp = val;
    *mBufPtr++ = temp[0];
    *mBufPtr++ = temp[1];
    *mBufPtr++ = temp[2];
    *mBufPtr++ = temp[3];    
  }
}
  
PRUint32 ipcMessageWriter::PutBytes(const void* src, PRUint32 n)
{
  if (EnsureCapacity(n)) {
    memcpy(mBufPtr, src, n);
    mBufPtr += n;
    return n;
  }
  return 0;
}
    
PRBool ipcMessageWriter::GrowCapacity(PRInt32 sizeNeeded)
{
  if (sizeNeeded < 0)
    return PR_FALSE;
  PRInt32 newCapacity = (mBufPtr - mBuf) + sizeNeeded;
  if (mCapacity == 0)
    mCapacity = newCapacity;
  else
  {
    while (newCapacity > mCapacity && (mCapacity << 1) > 0)
      mCapacity <<= 1;
    if (newCapacity > mCapacity) 
      return PR_FALSE;
  }
    
  PRInt32 curPos = mBufPtr - mBuf;
  mBuf = (PRUint8*)realloc(mBuf, mCapacity);
  if (!mBuf) {
    mError = PR_TRUE;
    return PR_FALSE;
  }
  mBufPtr = mBuf + curPos;
  mBufEnd = mBuf + mCapacity;
  return PR_TRUE;
}
