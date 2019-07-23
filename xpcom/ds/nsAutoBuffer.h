





































#ifndef nsAutoBuffer_h__
#define nsAutoBuffer_h__

#ifndef nsMemory_h__
#include "nsMemory.h"
#endif

































template <class T, PRInt32 sz>
class nsAutoBuffer
{
public:
  nsAutoBuffer() :
    mBufferPtr(mStackBuffer),
    mCurElemCapacity(sz)
  {
  }

  ~nsAutoBuffer()
  {
    if (mBufferPtr != mStackBuffer)
      NS_Free(mBufferPtr);
  }

  
  
  PRBool EnsureElemCapacity(PRInt32 inElemCapacity)
  {
    if (inElemCapacity <= mCurElemCapacity)
      return PR_TRUE;

    T* newBuffer;

    if (mBufferPtr != mStackBuffer) {
      newBuffer = static_cast<T*>(NS_Realloc(mBufferPtr, inElemCapacity * sizeof(T)));
    } else {
      newBuffer = static_cast<T*>(NS_Alloc(inElemCapacity * sizeof(T)));
    }

    if (!newBuffer)
      return PR_FALSE;

    mBufferPtr = newBuffer; 
    mCurElemCapacity = inElemCapacity;
    return PR_TRUE;
  }

  PRBool AddElemCapacity(PRInt32 inElemCapacity)
  {
    return EnsureElemCapacity(mCurElemCapacity + inElemCapacity);
  }

  T*          get()             const  { return mBufferPtr; }
  PRInt32     GetElemCapacity() const  { return mCurElemCapacity;  }

  T& operator[](PRUint32 i)
  {
    return mBufferPtr[i];
  }

  const T& operator[](PRUint32 i) const
  {
    return mBufferPtr[i];
  }

protected:

  T             *mBufferPtr;
  T             mStackBuffer[sz];
  PRInt32       mCurElemCapacity;
};

#endif 
