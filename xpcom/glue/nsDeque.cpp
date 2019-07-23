




































#include "nsDeque.h"
#include "nsCRT.h"
#ifdef DEBUG_rickg
#include <stdio.h>
#endif






























#define modasgn(x,y) if (x<0) x+=y; x%=y
#define modulus(x,y) ((x<0)?(x+y)%(y):(x)%(y))





nsDeque::nsDeque(nsDequeFunctor* aDeallocator) {
  MOZ_COUNT_CTOR(nsDeque);
  mDeallocator=aDeallocator;
  mOrigin=mSize=0;
  mData=mBuffer; 
  mCapacity=sizeof(mBuffer)/sizeof(mBuffer[0]);
  memset(mData, 0, mCapacity*sizeof(mBuffer[0]));
}




nsDeque::~nsDeque() {
  MOZ_COUNT_DTOR(nsDeque);

#ifdef DEBUG_rickg
  char buffer[30];
  printf("Capacity: %i\n", mCapacity);

  static int mCaps[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  switch(mCapacity) {
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
  if (mData && (mData!=mBuffer)) {
    free(mData);
  }
  mData=0;
  SetDeallocator(0);
}







void nsDeque::SetDeallocator(nsDequeFunctor* aDeallocator){
  if (mDeallocator) {
    delete mDeallocator;
  }
  mDeallocator=aDeallocator;
}






nsDeque& nsDeque::Empty() {
  if (mSize && mData) {
    memset(mData, 0, mCapacity*sizeof(mData));
  }
  mSize=0;
  mOrigin=0;
  return *this;
}






nsDeque& nsDeque::Erase() {
  if (mDeallocator && mSize) {
    ForEach(*mDeallocator);
  }
  return Empty();
}











PRBool nsDeque::GrowCapacity() {
  PRInt32 theNewSize=mCapacity<<2;
  NS_ASSERTION(theNewSize>mCapacity, "Overflow");
  if (theNewSize<=mCapacity)
    return PR_FALSE;
  void** temp=(void**)malloc(theNewSize * sizeof(void*));
  if (!temp)
    return PR_FALSE;

  
  
  
  

  memcpy(temp, mData + mOrigin, sizeof(void*) * (mCapacity - mOrigin));
  memcpy(temp + (mCapacity - mOrigin), mData, sizeof(void*) * mOrigin);

  if (mData != mBuffer) {
    free(mData);
  }

  mCapacity=theNewSize;
  mOrigin=0; 
  mData=temp;

  return PR_TRUE;
}









nsDeque& nsDeque::Push(void* aItem) {
  if (mSize==mCapacity && !GrowCapacity()) {
    NS_WARNING("out of memory");
    return *this;
  }
  mData[modulus(mOrigin + mSize, mCapacity)]=aItem;
  mSize++;
  return *this;
}


































nsDeque& nsDeque::PushFront(void* aItem) {
  mOrigin--;
  modasgn(mOrigin,mCapacity);
  if (mSize==mCapacity) {
    if (!GrowCapacity()) {
      NS_WARNING("out of memory");
      return *this;
    }
    
    mData[mSize]=mData[mOrigin];
  }
  mData[mOrigin]=aItem;
  mSize++;
  return *this;
}






void* nsDeque::Pop() {
  void* result=0;
  if (mSize>0) {
    --mSize;
    PRInt32 offset=modulus(mSize + mOrigin, mCapacity);
    result=mData[offset];
    mData[offset]=0;
    if (!mSize) {
      mOrigin=0;
    }
  }
  return result;
}







void* nsDeque::PopFront() {
  void* result=0;
  if (mSize>0) {
    NS_ASSERTION(mOrigin < mCapacity, "Error: Bad origin");
    result=mData[mOrigin];
    mData[mOrigin++]=0;     
    mSize--;
    
    
    if (mCapacity==mOrigin || !mSize) {
      mOrigin=0;
    }
  }
  return result;
}







void* nsDeque::Peek() {
  void* result=0;
  if (mSize>0) {
    result = mData[modulus(mSize - 1 + mOrigin, mCapacity)];
  }
  return result;
} 







void* nsDeque::PeekFront() {
  void* result=0;
  if (mSize>0) {
    result=mData[mOrigin];
  }
  return result;
}










void* nsDeque::ObjectAt(PRInt32 aIndex) const {
  void* result=0;
  if ((aIndex>=0) && (aIndex<mSize)) {
    result=mData[modulus(mOrigin + aIndex, mCapacity)];
  }
  return result;
}








nsDequeIterator nsDeque::Begin() const{
  return nsDequeIterator(*this, 0);
}









nsDequeIterator nsDeque::End() const{
  return nsDequeIterator(*this, mSize - 1);
}

void* nsDeque::Last() const {
  return End().GetCurrent();
}









void nsDeque::ForEach(nsDequeFunctor& aFunctor) const{
  for (PRInt32 i=0; i<mSize; i++) {
    aFunctor(ObjectAt(i));
  }
}










const void* nsDeque::FirstThat(nsDequeFunctor& aFunctor) const{
  for (PRInt32 i=0; i<mSize; i++) {
    void* obj=aFunctor(ObjectAt(i));
    if (obj) {
      return obj;
    }
  }
  return 0;
}















nsDequeIterator::nsDequeIterator(const nsDeque& aQueue, int aIndex)
: mIndex(aIndex),
  mDeque(aQueue)
{
}






nsDequeIterator::nsDequeIterator(const nsDequeIterator& aCopy)
: mIndex(aCopy.mIndex),
  mDeque(aCopy.mDeque)
{
}





nsDequeIterator& nsDequeIterator::First(){
  mIndex=0;
  return *this;
}







nsDequeIterator& nsDequeIterator::operator=(const nsDequeIterator& aCopy) {
  NS_ASSERTION(&mDeque==&aCopy.mDeque,"you can't change the deque that an interator is iterating over, sorry.");
  mIndex=aCopy.mIndex;
  return *this;
}








PRBool nsDequeIterator::operator!=(nsDequeIterator& aIter) {
  return PRBool(!this->operator==(aIter));
}









PRBool nsDequeIterator::operator<(nsDequeIterator& aIter) {
  return PRBool(((mIndex<aIter.mIndex) && (&mDeque==&aIter.mDeque)));
}







PRBool nsDequeIterator::operator==(nsDequeIterator& aIter) {
  return PRBool(((mIndex==aIter.mIndex) && (&mDeque==&aIter.mDeque)));
}









PRBool nsDequeIterator::operator>=(nsDequeIterator& aIter) {
  return PRBool(((mIndex>=aIter.mIndex) && (&mDeque==&aIter.mDeque)));
}






void* nsDequeIterator::operator++() {
  NS_ASSERTION(mIndex<mDeque.mSize,
    "You have reached the end of the Internet."\
    "You have seen everything there is to see. Please go back. Now."
  );
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex>=mDeque.mSize) return 0;
#endif
  return mDeque.ObjectAt(++mIndex);
}







void* nsDequeIterator::operator++(int) {
  NS_ASSERTION(mIndex<=mDeque.mSize,
    "You have already reached the end of the Internet."\
    "You have seen everything there is to see. Please go back. Now."
  );
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex>mDeque.mSize) return 0;
#endif
  return mDeque.ObjectAt(mIndex++);
}






void* nsDequeIterator::operator--() {
  NS_ASSERTION(mIndex>=0,
    "You have reached the beginning of the Internet."\
    "You have seen everything there is to see. Please go forward. Now."
  );
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex<0) return 0;
#endif
  return mDeque.ObjectAt(--mIndex);
}







void* nsDequeIterator::operator--(int) {
  NS_ASSERTION(mIndex>=0,
    "You have already reached the beginning of the Internet."\
    "You have seen everything there is to see. Please go forward. Now."
  );
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex<0) return 0;
#endif
  return mDeque.ObjectAt(mIndex--);
}














void* nsDequeIterator::GetCurrent() {
  NS_ASSERTION(mIndex<mDeque.mSize&&mIndex>=0,"Current is out of bounds");
#ifndef TIMELESS_LIGHTWEIGHT
  if (mIndex>=mDeque.mSize||mIndex<0) return 0;
#endif
  return mDeque.ObjectAt(mIndex);
}









void nsDequeIterator::ForEach(nsDequeFunctor& aFunctor) const{
  mDeque.ForEach(aFunctor);
}










const void* nsDequeIterator::FirstThat(nsDequeFunctor& aFunctor) const{
  return mDeque.FirstThat(aFunctor);
}
