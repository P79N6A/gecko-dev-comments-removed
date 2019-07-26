






















#ifndef _NSDEQUE
#define _NSDEQUE

#include "nscore.h"
#include "nsDebug.h"
#include "mozilla/Attributes.h"
#include "mozilla/fallible.h"
#include "mozilla/MemoryReporting.h"








class nsDequeFunctor{
public:
  virtual void* operator()(void* anObject)=0;
  virtual ~nsDequeFunctor() {}
};















class nsDequeIterator;

class NS_COM_GLUE nsDeque {
  friend class nsDequeIterator;
  typedef mozilla::fallible_t fallible_t;
  public:
   nsDeque(nsDequeFunctor* aDeallocator = nullptr);
  ~nsDeque();

  





  inline int32_t GetSize() const {return mSize;}

  




  void Push(void* aItem) {
    if (!Push(aItem, fallible_t())) {
      NS_ABORT_OOM(mSize);
    }
  }

  bool Push(void* aItem, const fallible_t&) NS_WARN_UNUSED_RESULT;

  




  void PushFront(void* aItem) {
    if (!PushFront(aItem, fallible_t())) {
      NS_ABORT_OOM(mSize);
    }
  }

  bool PushFront(void* aItem, const fallible_t&) NS_WARN_UNUSED_RESULT;

  




  void* Pop();

  




  void* PopFront();

  





  void* Peek();
  




  void* PeekFront();

  





  void* ObjectAt(int aIndex) const;

  





  void* RemoveObjectAt(int aIndex);

  


  void Empty();

  




  void Erase();

  





  nsDequeIterator Begin() const;

  





  nsDequeIterator End() const;

  void* Last() const;

  






  void ForEach(nsDequeFunctor& aFunctor) const;

  








  const void* FirstThat(nsDequeFunctor& aFunctor) const;

  void SetDeallocator(nsDequeFunctor* aDeallocator);

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
  int32_t         mSize;
  int32_t         mCapacity;
  int32_t         mOrigin;
  nsDequeFunctor* mDeallocator;
  void*           mBuffer[8];
  void**          mData;

private:

  




  nsDeque(const nsDeque& other);

  





  nsDeque& operator=(const nsDeque& anOther);

  bool GrowCapacity();
};





class NS_COM_GLUE nsDequeIterator {
public:
  
















  nsDequeIterator(const nsDeque& aQueue, int aIndex=0);

  




  nsDequeIterator(const nsDequeIterator& aCopy);

  



  nsDequeIterator& First();

  




  nsDequeIterator& operator=(const nsDequeIterator& aCopy);

  






  bool operator!=(nsDequeIterator& aIter);

  








  bool operator<(nsDequeIterator& aIter);

  





  bool operator==(nsDequeIterator& aIter);

  








  bool operator>=(nsDequeIterator& aIter);

  





  void* operator++();

  






  void* operator++(int);

  





  void* operator--();

  






  void* operator--(int);

  














  void* GetCurrent();

  







  void ForEach(nsDequeFunctor& aFunctor) const;

  








  const void* FirstThat(nsDequeFunctor& aFunctor) const;

  protected:

  int32_t         mIndex;
  const nsDeque&  mDeque;
};
#endif
