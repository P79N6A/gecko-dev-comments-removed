





































#include "ipcMessageReader.h"
#include <string.h>





PRUint8 ipcMessageReader::GetInt8()
{
  if (mBufPtr < mBufEnd)
    return *mBufPtr++;
  mError = PR_TRUE;
  return 0;
}





PRUint16 ipcMessageReader::GetInt16()
{
  if (mBufPtr + sizeof(PRUint16) <= mBufEnd) {
    PRUint8 temp[2] = { mBufPtr[0], mBufPtr[1] };
    mBufPtr += sizeof(PRUint16);
    return *(PRUint16*)temp;
  }
  mError = PR_TRUE;
  return 0;
}

PRUint32 ipcMessageReader::GetInt32()
{
  if (mBufPtr + sizeof(PRUint32) <= mBufEnd) {
    PRUint8 temp[4] = { mBufPtr[0], mBufPtr[1], mBufPtr[2], mBufPtr[3] };
    mBufPtr += sizeof(PRUint32);
    return *(PRUint32*)temp;
  }
  mError = PR_TRUE;
  return 0;
}

PRInt32 ipcMessageReader::GetBytes(void* destBuffer, PRInt32 n)
{
  if (mBufPtr + n <= mBufEnd) {
    memcpy(destBuffer, mBufPtr, n);
    mBufPtr += n;
    return n;
  }
  mError = PR_TRUE;
  return 0;
}

PRBool ipcMessageReader::AdvancePtr(PRInt32 n)
{
  const PRUint8 *newPtr = mBufPtr + n;
  if (newPtr >= mBuf && newPtr <= mBufEnd) {
    mBufPtr = newPtr;
    return PR_TRUE;
  }
  mError = PR_TRUE;
  return PR_FALSE;
}
