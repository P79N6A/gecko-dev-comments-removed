






















#ifndef _NSDEQUE
#define _NSDEQUE

#include "nscore.h"








class nsDequeFunctor{
public:
  virtual void* operator()(void* anObject)=0;
};















class nsDequeIterator;

class NS_COM_GLUE nsDeque {
  friend class nsDequeIterator;
  public:
   nsDeque(nsDequeFunctor* aDeallocator = nullptr);
  ~nsDeque();

  





  inline int32_t GetSize() const {return mSize;}

  





  nsDeque& Push(void* aItem);

  





  nsDeque& PushFront(void* aItem);

  




  void* Pop();

  




  void* PopFront();

  





  void* Peek();
  




  void* PeekFront();

  





  void* ObjectAt(int aIndex) const;

  





  void* RemoveObjectAt(int aIndex);

  




  nsDeque& Empty();

  






  nsDeque& Erase();

  





  nsDequeIterator Begin() const;

  





  nsDequeIterator End() const;

  void* Last() const;
  







  void ForEach(nsDequeFunctor& aFunctor) const;

  








  const void* FirstThat(nsDequeFunctor& aFunctor) const;

  void SetDeallocator(nsDequeFunctor* aDeallocator);

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
