





































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
      nsMemory::Free(mBufferPtr);
  }

  PRBool EnsureElemCapacity(PRInt32 inElemCapacity)
  {
    if (inElemCapacity <= mCurElemCapacity)
      return PR_TRUE;
    
    T* newBuffer;

    if (mBufferPtr != mStackBuffer)
      newBuffer = (T*)nsMemory::Realloc((void *)mBufferPtr, inElemCapacity * sizeof(T));
    else 
      newBuffer = (T*)nsMemory::Alloc(inElemCapacity * sizeof(T));

    if (!newBuffer)
      return PR_FALSE;

    if (mBufferPtr != mStackBuffer)
      nsMemory::Free(mBufferPtr);

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

protected:

  T             *mBufferPtr;
  T             mStackBuffer[sz];
  PRInt32       mCurElemCapacity;
};

#endif 
