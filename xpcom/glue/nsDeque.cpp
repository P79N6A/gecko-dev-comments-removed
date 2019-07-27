





#include "nsDeque.h"
#include "nsISupportsImpl.h"
#include <string.h>
#ifdef DEBUG_rickg
#include <stdio.h>
#endif






























#define modasgn(x,y) if (x<0) x+=y; x%=y
#define modulus(x,y) ((x<0)?(x+y)%(y):(x)%(y))





nsDeque::nsDeque(nsDequeFunctor* aDeallocator)
{
  MOZ_COUNT_CTOR(nsDeque);
  mDeallocator = aDeallocator;
  mOrigin = mSize = 0;
  mData = mBuffer; 
  mCapacity = sizeof(mBuffer) / sizeof(mBuffer[0]);
  memset(mData, 0, mCapacity * sizeof(mBuffer[0]));
}




nsDeque::~nsDeque()
{
  MOZ_COUNT_DTOR(nsDeque);

#ifdef DEBUG_rickg
  char buffer[30];
  printf("Capacity: %i\n", mCapacity);

  static int mCaps[15] = {0};
  switch (mCapacity) {
    case 4:     mCaps[0]++; break;
    case 8:     mCaps[1]++; break;
    case 16:    mCaps[2]++; break;
    case 32:    mCaps[3]++; break;
    case 64:    mCaps[4]++; break;
    case 128:   mCaps[5]++; break;
    case 256:   mCaps[6]++; break;
    case 512:   mCaps[7]++; break;
    case 1024:  mCaps[8]++; break;
    case 2048:  mCaps[9]++; break;
    case 4096:  mCaps[10]++; break;
    default:
      break;
  }
#endif

  Erase();
  if (mData && mData != mBuffer) {
    free(mData);
  }
  mData = 0;
  SetDeallocator(0);
}

size_t
nsDeque::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t size = 0;
  if (mData != mBuffer) {
    size += aMallocSizeOf(mData);
  }

  if (mDeallocator) {
    size += aMallocSizeOf(mDeallocator);
  }

  return size;
}

size_t
nsDeque::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}







void
nsDeque::SetDeallocator(nsDequeFunctor* aDeallocator)
{
  delete mDeallocator;
  mDeallocator = aDeallocator;
}




void
nsDeque::Empty()
{
  if (mSize && mData) {
    memset(mData, 0, mCapacity * sizeof(*mData));
  }
  mSize = 0;
  mOrigin = 0;
}




void
nsDeque::Erase()
{
  if (mDeallocator && mSize) {
    ForEach(*mDeallocator);
  }
  Empty();
}








bool
nsDeque::GrowCapacity()
{
  int32_t theNewSize = mCapacity << 2;
  NS_ASSERTION(theNewSize > mCapacity, "Overflow");
  if (theNewSize <= mCapacity) {
    return false;
  }
  void** temp = (void**)malloc(theNewSize * sizeof(void*));
  if (!temp) {
    return false;
  }

  
  
  
  

  memcpy(temp, mData + mOrigin, sizeof(void*) * (mCapacity - mOrigin));
  memcpy(temp + (mCapacity - mOrigin), mData, sizeof(void*) * mOrigin);

  if (mData != mBuffer) {
    free(mData);
  }

  mCapacity = theNewSize;
  mOrigin = 0; 
  mData = temp;

  return true;
}








bool
nsDeque::Push(void* aItem, const fallible_t&)
{
  if (mSize == mCapacity && !GrowCapacity()) {
    return false;
  }
  mData[modulus(mOrigin + mSize, mCapacity)] = aItem;
  mSize++;
  return true;
}

































bool
nsDeque::PushFront(void* aItem, const fallible_t&)
{
  mOrigin--;
  modasgn(mOrigin, mCapacity);
  if (mSize == mCapacity) {
    if (!GrowCapacity()) {
      return false;
    }
    
    mData[mSize] = mData[mOrigin];
  }
  mData[mOrigin] = aItem;
  mSize++;
  return true;
}






void*
nsDeque::Pop()
{
  void* result = 0;
  if (mSize > 0) {
    --mSize;
    int32_t offset = modulus(mSize + mOrigin, mCapacity);
    result = mData[offset];
    mData[offset] = 0;
    if (!mSize) {
      mOrigin = 0;
    }
  }
  return result;
}







void*
nsDeque::PopFront()
{
  void* result = 0;
  if (mSize > 0) {
    NS_ASSERTION(mOrigin < mCapacity, "Error: Bad origin");
    result = mData[mOrigin];
    mData[mOrigin++] = 0;   
    mSize--;
    
    
    if (mCapacity == mOrigin || !mSize) {
      mOrigin = 0;
    }
  }
  return result;
}







void*
nsDeque::Peek()
{
  void* result = 0;
  if (mSize > 0) {
    result = mData[modulus(mSize - 1 + mOrigin, mCapacity)];
  }
  return result;
}







void*
nsDeque::PeekFront()
{
  void* result = 0;
  if (mSize > 0) {
    result = mData[mOrigin];
  }
  return result;
}










void*
nsDeque::ObjectAt(int32_t aIndex) const
{
  void* result = 0;
  if (aIndex >= 0 && aIndex < mSize) {
    result = mData[modulus(mOrigin + aIndex, mCapacity)];
  }
  return result;
}

void*
nsDeque::RemoveObjectAt(int32_t aIndex)
{
  if (aIndex < 0 || aIndex >= mSize) {
    return 0;
  }
  void* result = mData[modulus(mOrigin + aIndex, mCapacity)];

  
  
  for (int32_t i = aIndex; i < mSize; ++i) {
    mData[modulus(mOrigin + i, mCapacity)] =
      mData[modulus(mOrigin + i + 1, mCapacity)];
  }
  mSize--;

  return result;
}








nsDequeIterator
nsDeque::Begin() const
{
  return nsDequeIterator(*this, 0);
}









nsDequeIterator
nsDeque::End() const
{
  return nsDequeIterator(*this, mSize - 1);
}

void*
nsDeque::Last() const
{
  return End().GetCurrent();
}









void
nsDeque::ForEach(nsDequeFunctor& aFunctor) const
{
  for (int32_t i = 0; i < mSize; ++i) {
    aFunctor(ObjectAt(i));
  }
}










const void*
nsDeque::FirstThat(nsDequeFunctor& aFunctor) const
{
  for (int32_t i = 0; i < mSize; ++i) {
    void* obj = aFunctor(ObjectAt(i));
    if (obj) {
      return obj;
    }
  }
  return 0;
}















nsDequeIterator::nsDequeIterator(const nsDeque& aQueue, int aIndex)
  : mIndex(aIndex)
  , mDeque(aQueue)
{
}






nsDequeIterator::nsDequeIterator(const nsDequeIterator& aCopy)
  : mIndex(aCopy.mIndex)
  , mDeque(aCopy.mDeque)
{
}





nsDequeIterator&
nsDequeIterator::First()
{
  mIndex = 0;
  return *this;
}







nsDequeIterator&
nsDequeIterator::operator=(const nsDequeIterator& aCopy)
{
  NS_ASSERTION(&mDeque == &aCopy.mDeque,
               "you can't change the deque that an interator is iterating over, sorry.");
  mIndex = aCopy.mIndex;
  return *this;
}








bool
nsDequeIterator::operator!=(nsDequeIterator& aIter)
{
  return !this->operator==(aIter);
}









bool
nsDequeIterator::operator<(nsDequeIterator& aIter)
{
  return mIndex < aIter.mIndex && &mDeque == &aIter.mDeque;
}







bool
nsDequeIterator::operator==(nsDequeIterator& aIter)
{
  return mIndex == aIter.mIndex && &mDeque == &aIter.mDeque;
}









bool
nsDequeIterator::operator>=(nsDequeIterator& aIter)
{
  return mIndex >= aIter.mIndex && &mDeque == &aIter.mDeque;
}






void*
nsDequeIterator::operator++()
{
  NS_ASSERTION(mIndex < mDeque.mSize,
               "You have reached the end of the Internet. You have seen "
               "everything there is to see. Please go back. Now.");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex >= mDeque.mSize) {
    return 0;
  }
#endif
  return mDeque.ObjectAt(++mIndex);
}







void*
nsDequeIterator::operator++(int)
{
  NS_ASSERTION(mIndex <= mDeque.mSize,
               "You have reached the end of the Internet. You have seen "
               "everything there is to see. Please go back. Now.");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex > mDeque.mSize) {
    return 0;
  }
#endif
  return mDeque.ObjectAt(mIndex++);
}






void*
nsDequeIterator::operator--()
{
  NS_ASSERTION(mIndex >= 0,
               "You have reached the end of the Internet. You have seen "
               "everything there is to see. Please go forward. Now.");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex < 0) {
    return 0;
  }
#endif
  return mDeque.ObjectAt(--mIndex);
}







void*
nsDequeIterator::operator--(int)
{
  NS_ASSERTION(mIndex >= 0,
               "You have reached the end of the Internet. You have seen "
               "everything there is to see. Please go forward. Now.");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex < 0) {
    return 0;
  }
#endif
  return mDeque.ObjectAt(mIndex--);
}














void*
nsDequeIterator::GetCurrent()
{
  NS_ASSERTION(mIndex < mDeque.mSize && mIndex >= 0, "Current is out of bounds");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex >= mDeque.mSize || mIndex < 0) {
    return 0;
  }
#endif
  return mDeque.ObjectAt(mIndex);
}









void
nsDequeIterator::ForEach(nsDequeFunctor& aFunctor) const
{
  mDeque.ForEach(aFunctor);
}










const void*
nsDequeIterator::FirstThat(nsDequeFunctor& aFunctor) const
{
  return mDeque.FirstThat(aFunctor);
}
